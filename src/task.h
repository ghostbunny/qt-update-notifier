﻿/*
 * copyright: 2014-2017
 * name : Francis Banyikwa
 * email: mhogomchungu@gmail.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __TASK_H_INCLUDED__
#define __TASK_H_INCLUDED__

#include <vector>
#include <utility>
#include <future>
#include <functional>
#include <QThread>
#include <QEventLoop>
#include <QMutex>

/*
 *
 * Examples on how to use the library are at the end of this file.
 *
 */

/*
 * This library wraps a function into a future where the result of the function
 * can be retrieved through the future's below public methods:
 *
 * 1. .get().  This method runs the wrapped function on the current thread
 *             and could block the thread and hang GUI.
 *
 * 2. .then(). This method does three things:
 *
 *             1. Registers a method to be called when a wrapped function finish running.
 *
 *             2. Runs the wrapped function on a background thread.
 *
 *             3. Runs the registered method on the current thread when the wrapped function finish
 *                running.
 *
 * 3. .await(). This method does three things:
 *
 *             1. Suspends the current thread at a point where this method is called.
 *
 *             2. Creates a background thread and then runs the wrapped function in the background
 *                thread.
 *
 *             3. Unsuspends the current thread when the wrapped function finish and let the
 *                current thread continue normally.
 *
 *             The suspension at step 1 is done without blocking the thread and hence the suspension
 *             can be done in the GUI thread and the GUI will remain responsive.
 *
 * 4. .queue(). This method runs tasks in a future sequentially and a passed in function will be called
 *              when all tasks finish running. This method behaves like .then( [](){} ) if the future is
 *              managing only one task.
 *
 * 5. .cancel(). This method can be used to cancel a future. It is important to know
 *               that this method does not terminate a running thread that is powering a future, it just
 *               releases memory used by a future and this method should be used if a future is to be discarded
 *               after it it is acquired but never used.
 *
 * 6. .all_threads(). This method returns a vector of QThreads that are powering futures.
 *                    The vector will contain a single entry if this future powers its own task. If this future
 *                    manages other futures,then the returned vector will contain QThread pointers that are in
 *                    the same order as tasks/futures passed to Task::run().
 *
 * 7. .start(). This method is to be used if a future is to be run without caring about
 *              its result. Use this method if you want a future to run but dont want to use any of the above mentioned
 *              methods.
 *
 * 8. .manages_multiple_futures(). This method can be used to check if a future powers
 *    its own task or manages other futures.
 *
 *
 * The future is of type "Task::future<T>&" and "std::reference_wrapper"[1]
 * class can be used if they are to be managed in a container that can not handle references.
 *
 * [1] http://en.cppreference.com/w/cpp/utility/functional/reference_wrapper
 */

namespace Task
{
	template< typename T >
	using pair = std::pair< std::function< T() >,std::function< void( T ) > > ;

	using void_pair = std::pair< std::function< void() >,std::function< void() > > ;

	class Thread : public QThread
	{
		Q_OBJECT
	public:
		Thread()
		{
			#if QT_VERSION < QT_VERSION_CHECK( 5,0,0 )
				connect( this,SIGNAL( finished() ),this,SLOT( deleteLater() ) ) ;
			#else
				connect( this,&QThread::finished,this,&QThread::deleteLater ) ;
			#endif
		}
	protected:
		virtual ~Thread()
		{
		}
	private:
		virtual void run()
		{
		}
	};

	template< typename T >
	class future : private QObject
	{
	public:
		/*
		 * ----------------Start of public API----------------
		 */
		/*
		 * Use this API if you care about the result
		 */
		void then( std::function< void( T ) > function )
		{
			m_function = std::move( function ) ;
			this->start() ;
		}
		/*
		 * Use this API if you DO NOT care about the result
		 */
		void then( std::function< void() > function )
		{
			m_function_1 = std::move( function ) ;
			this->start() ;
		}
		void queue( std::function< void() > function = [](){} )
		{
			if( this->manages_multiple_futures() ){

				m_function_1 = std::move( function ) ;

				this->_queue() ;
			}else{
				this->then( std::move( function ) ) ;
			}
		}
		T get()
		{
			if( this->manages_multiple_futures() ){

				for( auto& it : m_tasks ){

					it.second( it.first->get() ) ;
				}

				this->deleteLater() ;

				return T() ;
			}else{
				return m_get() ;
			}
		}
		T await()
		{
			QEventLoop p ;

			T q ;

			m_function = [ & ]( T&& r ){ q = std::move( r ) ; p.exit() ; } ;

			this->start() ;

			p.exec() ;

			return q ;
		}
		bool manages_multiple_futures()
		{
			return m_tasks.size() > 0 ;
		}
		const std::vector< QThread * >& all_threads()
		{
			return m_threads ;
		}
		QThread * first_thread()
		{
			return m_threads[ 0 ] ;
		}
		QThread * thread_at( std::vector< QThread * >::size_type s )
		{
			return m_threads[ s ] ;
		}
		void start()
		{
			if( this->manages_multiple_futures() ){

				this->_start() ;
			}else{
				m_start() ;
			}
		}
		void cancel()
		{
			if( this->manages_multiple_futures() ){

				for( auto& it : m_tasks ){

					it.first->cancel() ;
				}

				this->deleteLater() ;
			}else{
				m_cancel() ;
			}
		}
		/*
		 * ----------------End of public API----------------
		 */
		future() = delete ;
		future( const future& ) = delete ;
		future( future&& ) = delete ;
		future& operator=( const future& ) = delete ;
		future& operator=( future&& ) = delete ;

		future( QThread * e,
			std::function< void() >&& start,
			std::function< void() >&& cancel,
			std::function< T() >&& get ) :
			m_thread( e ),
			m_start ( std::move( start ) ),
			m_cancel( std::move( cancel ) ),
			m_get   ( std::move( get ) )
		{
			if( m_thread ){

				m_threads.push_back( m_thread ) ;
			}else{
				/*
				 * This object was created by "_private_future< T >()" class.
				 * It has no QThread of its own because it only manages other futures.
				 *
				 */
			}
		}
		void run( T&& r )
		{
			if( m_function_1 != nullptr ){

				m_function_1() ;

			}else if( m_function != nullptr ){

				m_function( std::move( r ) ) ;
			}
		}

		template< typename E >
		friend void _private_add( Task::future< E >&,
					  Task::future< E >&,
					  std::function< void( E ) >&& ) ;
	private:
		void _queue()
		{
			m_tasks[ m_counter ].first->then( [ this ]( T&& e ){

				m_tasks[ m_counter ].second( std::move( e ) ) ;

				m_counter++ ;

				if( m_counter == m_tasks.size() ){

					m_function_1() ;

					this->deleteLater() ;
				}else{
					this->_queue() ;
				}
			} ) ;
		}
		void _start()
		{
			for( auto& it : m_tasks ){

				it.first->then( [ & ]( T&& e ){

					QMutexLocker m( &m_mutex ) ;

					Q_UNUSED( m ) ;

					m_counter++ ;

					it.second( std::move( e ) ) ;

					if( m_counter == m_tasks.size() ){

						if( m_function_1 != nullptr ){

							m_function_1() ;

						}else if( m_function != nullptr ){

							m_function( T() ) ;
						}

						this->deleteLater() ;
					}
				} ) ;
			}
		}

		QThread * m_thread = nullptr ;
		std::function< void( T ) > m_function = nullptr ;
		std::function< void() > m_function_1  = nullptr ;
		std::function< void() > m_start       = [](){} ;
		std::function< void() > m_cancel      = [](){} ;
		std::function< T() > m_get            = [](){ return T() ; } ;

		QMutex m_mutex ;
		std::vector< std::pair< Task::future< T > *,std::function< void( T ) > > > m_tasks ;
		std::vector< QThread * > m_threads ;
		decltype( m_tasks.size() ) m_counter = 0 ;
	};

	template<>
	class future< void > : private QObject
	{
	public:
		/*
		 * ----------------Start of public API----------------
		 */
		void then( std::function< void() > function )
		{
			m_function = std::move( function ) ;
			this->start() ;
		}
		void get()
		{
			if( this->manages_multiple_futures() ){

				for( auto& it : m_tasks ){

					it.first->get() ;
					it.second() ;
				}

				this->deleteLater() ;
			}else{
				m_get() ;
			}
		}
		void await()
		{
			QEventLoop p ;

			m_function = [ & ](){ p.exit() ; } ;

			this->start() ;

			p.exec() ;
		}
		bool manages_multiple_futures()
		{
			return m_tasks.size() > 0 ;
		}
		const std::vector< QThread * >& threads()
		{
			return m_threads ;
		}
		QThread * first_thread()
		{
			return m_threads[ 0 ] ;
		}
		QThread * thread_at( std::vector< QThread * >::size_type s )
		{
			return m_threads[ s ] ;
		}
		void start()
		{
			if( this->manages_multiple_futures() ){

				this->_start() ;
			}else{
				m_start() ;
			}
		}
		void cancel()
		{
			if( this->manages_multiple_futures() ){

				for( auto& it : m_tasks ){

					it.first->cancel() ;
				}

				this->deleteLater() ;
			}else{
				m_cancel() ;
			}
		}
		void queue( std::function< void() > function = [](){} )
		{
			if( this->manages_multiple_futures() ){

				m_function = std::move( function ) ;

				this->_queue() ;
			}else{
				this->then( std::move( function ) ) ;
			}
		}
		/*
		 * ----------------End of public API----------------
		 */
		future() = delete ;
		future( const future& ) = delete ;
		future( future&& ) = delete ;
		future& operator=( const future& ) = delete ;
		future& operator=( future&& ) = delete ;

		future(	QThread * e ,
			std::function< void() >&& start,
			std::function< void() >&& cancel,
			std::function< void() >&& get ) :
			m_thread( e ),
			m_start ( std::move( start ) ),
			m_cancel( std::move( cancel ) ),
			m_get   ( std::move( get ) )
		{
			if( m_thread ){

				m_threads.push_back( m_thread ) ;
			}else{
				/*
				 * This object was created by "_private_future< T >()" class.
				 * It has no QThread of its own because it only manages other futures.
				 *
				 */
			}
		}

		template< typename T >
		friend void _private_add_void( Task::future< T >&,
					       Task::future< T >&,
					       std::function< T() >&& ) ;
		void run()
		{
			m_function() ;
		}
	private:
		void _queue()
		{
			m_tasks[ m_counter ].first->then( [ this ](){

				m_tasks[ m_counter ].second() ;

				m_counter++ ;

				if( m_counter == m_tasks.size() ){

					m_function() ;

					this->deleteLater() ;
				}else{
					this->_queue() ;
				}
			} ) ;
		}

		void _start()
		{
			for( auto& it : m_tasks ){

				it.first->then( [ & ](){

					QMutexLocker m( &m_mutex ) ;

					Q_UNUSED( m ) ;

					m_counter++ ;

					it.second() ;

					if( m_counter == m_tasks.size() ){

						m_function() ;

						this->deleteLater() ;
					}
				} ) ;
			}
		}

		QThread * m_thread = nullptr ;

		std::function< void() > m_function = [](){} ;
		std::function< void() > m_start    = [](){} ;
		std::function< void() > m_cancel   = [](){} ;
		std::function< void() > m_get      = [](){} ;

		QMutex m_mutex ;
		std::vector< std::pair< Task::future< void > *,std::function< void() > > > m_tasks ;
		std::vector< QThread * > m_threads ;
		decltype( m_tasks.size() ) m_counter = 0 ;
	};

	template< typename T >
	class ThreadHelper : public Thread
	{
	public:
		ThreadHelper( std::function< T() >&& function ) :
			m_function( std::move( function ) ),
			m_future( this,
				  [ this ](){ this->start() ; },
				  [ this ](){ this->deleteLater() ; },
				  [ this ](){ this->deleteLater() ; return m_function() ; } )
		{
		}
		future<T>& Future()
		{
			return m_future ;
		}
	private:
		~ThreadHelper()
		{
			m_future.run( std::move( m_result ) ) ;
		}
		void run()
		{
			m_result = m_function() ;
		}
		std::function< T() > m_function ;
		future<T> m_future ;
		T m_result ;
	};

	template<>
	class ThreadHelper< void > : public Thread
	{
	public:
		ThreadHelper( std::function< void() >&& function ) :
			m_function( std::move( function ) ),
			m_future( this,
				  [ this ](){ this->start() ; },
				  [ this ](){ this->deleteLater() ; },
				  [ this ](){ m_function() ; this->deleteLater() ; } )
		{
		}
		future< void >& Future()
		{
			return m_future ;
		}
	private:
		~ThreadHelper()
		{
			m_future.run() ;
		}
		void run()
		{
			m_function() ;
		}
		std::function< void() > m_function ;
		future< void > m_future ;
	};

	/*
	 *
	 * Below APIs wrappes a function around a future and then returns the future.
	 *
	 */

	template< typename T >
	future<T>& run( std::function< T() > function )
	{
		return ( new ThreadHelper<T>( std::move( function ) ) )->Future() ;
	}

	template< typename T,typename ... Args >
	future<T>& run( std::function< T( Args ... ) > function,Args ... args )
	{
		return Task::run<T>( std::bind( std::move( function ),std::move( args ) ... ) ) ;
	}

	static inline future< void >& run( std::function< void() > function )
	{
		return Task::run< void >( std::move( function ) ) ;
	}

	template< typename ... Args >
	future< void >& run( std::function< void( Args ... ) > function,Args ... args )
	{
		return Task::run< void >( std::bind( std::move( function ),std::move( args ) ... ) ) ;
	}

	/*
	 * -------------------------Start of internal helper functions-------------------------
	 */

	template< typename T >
	void _private_add( Task::future< T >& a,Task::future< T >& b,
			   std::function< void( T ) >&& c )
	{
		a.m_tasks.emplace_back( std::addressof( b ),std::move( c ) ) ;
		a.m_threads.push_back( b.m_thread ) ;
	}

	template< typename T >
	void _private_add_void( Task::future< T >& a,Task::future< T >& b,
				std::function< T() >&& c )
	{
		a.m_tasks.emplace_back( std::addressof( b ),std::move( c ) ) ;
		a.m_threads.push_back( b.m_thread ) ;
	}

	template< typename T >
	void _private_add_task( Task::future< T >& f )
	{
		Q_UNUSED( f ) ;
	}

	template< typename T >
	void _private_add_future( Task::future< T >& f )
	{
		Q_UNUSED( f ) ;
	}

	template< typename T >
	void _private_add_pair( Task::future< T >& f )
	{
		Q_UNUSED( f ) ;
	}

	template< typename T >
	void _private_add_pair_void( Task::future< T >& f )
	{
		Q_UNUSED( f ) ;
	}

	template< typename ... T >
	void _private_add_task( Task::future< void >& f,std::function< void() >&& e,T&& ... t )
	{
		_private_add_void( f,Task::run< void >( std::move( e ) ),std::function< void() >( [](){} ) ) ;
		_private_add_task( f,std::move( t ) ... ) ;
	}

	template< typename ... T >
	void _private_add_future( Task::future< void >& f,Task::future< void >& e,T&& ... t )
	{
		_private_add_void( f,e,std::function< void() >( [](){} ) ) ;
		_private_add_future( f,std::forward<T>( t ) ... ) ;
	}

	template< typename E,typename F,typename ... T >
	void _private_add_pair( Task::future< E >& f,F&& s,T&& ... t )
	{
		_private_add( f,Task::run< E >( std::move( s.first ) ),std::move( s.second ) ) ;
		_private_add_pair( f,std::forward<T>( t ) ... ) ;
	}

	template< typename F,typename ... T >
	void _private_add_pair_void( Task::future< void >& f,F&& s,T&& ... t )
	{
		_private_add_void( f,Task::run< void >( std::move( s.first ) ),std::move( s.second ) ) ;
		_private_add_pair_void( f,std::forward<T>( t ) ... ) ;
	}

	template< typename T >
	struct _private_future
	{
		Task::future< T >& operator()()
		{
			return *( new Task::future< T >( nullptr,
							 [](){},
							 [](){},
							 [](){ return T() ; } ) ) ;
		}
	};

	template<>
	struct _private_future< void >
	{
		Task::future< void >& operator()()
		{
			return *( new Task::future< void >( nullptr,
							    [](){},
							    [](){},
							    [](){} ) ) ;
		}
	};

	/*
	 * -------------------------End of internal helper functions-------------------------
	 */

	template< typename ... T >
	Task::future< void >& run( std::function< void() > f,T ... t )
	{
		auto& e = _private_future< void >()() ;

		_private_add_task( e,std::move( f ),std::move( t ) ... ) ;

		return e ;
	}

	template< typename ... T >
	Task::future< void >& run( Task::future< void >& s,T&& ... t )
	{
		auto& e = _private_future< void >()() ;

		_private_add_future( e,s,std::forward<T>( t ) ... ) ;

		return e ;
	}

	template< typename ... T >
	Task::future< void >& run( void_pair s,T ... t )
	{
		auto& e = _private_future< void >()() ;

		_private_add_pair_void( e,std::move( s ),std::move( t ) ... ) ;

		return e ;
	}

	template< typename E,typename ... T >
	Task::future< E >& run( pair< E > s,T ... t )
	{
		auto& e = _private_future< E >()() ;

		_private_add_pair( e,std::move( s ),std::move( t ) ... ) ;

		return e ;
	}

	/*
	 *
	 * A few useful helper functions
	 *
	 */

	template< typename T >
	T await( std::function< T() > function )
	{
		return Task::run<T>( std::move( function ) ).await() ;
	}

	template< typename T,typename ... Args >
	T await( std::function< T( Args ... ) > function,Args ... args )
	{
		return Task::await<T>( std::bind( std::move( function ),std::move( args ) ... ) ) ;
	}

	static inline void await( std::function< void() > function )
	{
		Task::await< void >( std::move( function ) ) ;
	}

	template< typename T >
	T await( Task::future<T>& e )
	{
		return e.await() ;
	}

	template< typename T >
	T await( std::future<T> t )
	{
		return Task::await<T>( [ & ](){ return t.get() ; } ) ;
	}

	/*
	 * These methods run their arguments in a separate thread and does not offer
	 * continuation feature.Useful when wanting to just run a function in a
	 * different thread.
	 */
	static inline void exec( std::function< void() > function )
	{
		Task::run( std::move( function ) ).start() ;
	}

	template< typename T,typename ... Args >
	void exec( std::function< T( Args ... ) > function,Args ... args )
	{
		Task::exec( std::bind( std::move( function ),std::move( args ) ... ) ) ;
	}

	template< typename T >
	void exec( Task::future<T>& e )
	{
		e.start() ;
	}
}

#if 0 // start example block

Examples on how to use the library

**********************************************************
* Example use cases on how to use Task::run().then() API
**********************************************************

templated version that passes a return value of one function to another function
---------------------------------------------------------------------------------

int foo
{
	/*
	 * This task will run on a different thread
	 * This tasks returns a result
	 */
	return 0 ;
}

void bar( int r )
{
	/*
	 * This task will run on the original thread.
	 * This tasks takes an argument returned by task _a
	 */
}

Task::run<int>( foo ).then( bar ) ;

alternatively,

Task::future<int>& e = Task::run<int>( foo ) ;

e.then( bar ) ;


Non templated version that does not pass around return value
----------------------------------------------------------------
void foo_1()
{
	/*
	 * This task will run on a different thread
	 * This tasks returns with no result
	 */
}

void bar_1()
{
	/*
	 * This task will run on the original thread.
	 * This tasks takes no argument
	 */
}

Task::run( foo_1 ).then( bar_1 ) ;

alternatively,

Task::future<void>& e = Task::run( foo_1 ) ;

e.then( bar_1 ) ;

**********************************************************
* Example use cases on how to use Task::run().await() API
**********************************************************

int r = Task::await<int>( foo ) ;

alternatively,

Task::future<int>& e = Task::run<int>( foo ) ;

int r = e.await() ;

alternatively,

int r = Task::run<int>( foo ).await() ;

*******************************************************************
* Example use cases on how to use lambda that requires an argument
*******************************************************************

/*
 * declaring "foo_2" with an auto keyword will not be sufficient here
 * and the full std::function<blablabla> is required.
 *
 * For the same reason,just plugging in a lambda that requires arguments
 * into Task::run() will not be sufficent and the plugged in lambda must
 * be casted to std::function<blablabla> for it to compile.
 *
 * Why the above restriction? No idea but i suspect it has to do with
 * variadic template type deduction failing to see something.
 */

std::function< int( int ) > foo_2 = []( int x ){

	return x + 1 ;
} ;

Task::run( foo_2,6 ).then( []( int r ){

	qDebug() << r ;
} ) ;

alternatively,

r = Task::await( foo_2,6 ) ;

*******************************************************************
* Example use cases on how to run multiple tasks and wait for all to
* to finish before continuing
*******************************************************************

Task::future<void>& e = Task::run( fn1,fn2,fn3 ) ;

or alternatively,

Task::future<void>& f1 = Task::run( fn1 ) ;
Task::future<void>& f2 = Task::run( fn2 ) ;
Task::future<void & f3 = Task::run( fn3 ) ;

Task::future<void>& e = Task::run( f1,f2,f3 ) ;

1.0 .await() can then be called on the future to wait for all tasks to finish before continuing.
2.0 .then()  can then be called on the future to invoke a callback on the current thread when all tasks finish.

*******************************************************************
* Example use cases on how to run multiple tasks and their continuations
* and wait for all to finish before continuing
*******************************************************************

std::cout<< "Testing multiple tasks without continuation arguments" << std::endl ;

auto fna1 = [](){ _printThreadID(); } ;
auto fna2 = [](){ _printThreadID(); } ;
auto fna3 = [](){ _printThreadID(); } ;

auto ra1 = [](){ std::cout << "r1" << std::endl ; } ;
auto ra2 = [](){ std::cout << "r2" << std::endl ; } ;
auto ra3 = [](){ std::cout << "r3" << std::endl ; } ;

Task::future<void>& e = Task::run( Task::void_pair{ fna1,ra1 },
				   Task::void_pair{ fna2,ra2 },
				   Task::void_pair{ fna3,ra3 } ) ;

e.await() ;

std::cout<< "Testing multiple tasks with continuation arguments" << std::endl ;

auto fn1 = [](){ _printThreadID(); return 0 ;} ;
auto fn2 = [](){ _printThreadID(); return 0 ;} ;
auto fn3 = [](){ _printThreadID(); return 0 ; } ;

auto r1 = []( int ){ std::cout << "r1" << std::endl ; } ;
auto r2 = []( int ){ std::cout << "r2" << std::endl ; } ;
auto r3 = []( int ){ std::cout << "r3" << std::endl ; } ;

Task::future<int>& s = Task::run(  Task::pair<int>{ fn1,r1 },
				   Task::pair<int>{ fn2,r2 },
				   Task::pair<int>{ fn3,r3 } ) ;

s.then( [](){ QCoreApplication::quit() ;} ) ;

#endif //end example block

#endif //__TASK_H_INCLUDED__
