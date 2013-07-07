/*
 *
 *  Copyright (c) 2013
 *  name : mhogo mchungu
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CHECKOLDPACKAGES_H
#define CHECKOLDPACKAGES_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QThreadPool>
#include <QRunnable>
#include <QDebug>
#include <QDir>

class checkoldpackages : public QObject,public QRunnable
{
	Q_OBJECT
public:
	explicit checkoldpackages( QByteArray packageList = QByteArray(),QObject * parent = 0 ) ;
	~checkoldpackages() ;
	void start( void ) ;
signals:
	void outdatedPackages( QStringList ) ;
public slots:
private:
	void run( void ) ;
	void checkKernelVersion( void ) ;
	void checkLibreOfficeVersion( void ) ;
	void checkVirtualBoxVersion( void ) ;
	void checkCallibeVersion( void ) ;
	bool updateAvailable( QString ) ;
	QString m_iv ;
	QString m_nv ;
	QStringList m_package ;
	QByteArray m_packageList ;
};

#endif // CHECKOLDPACKAGES_H
