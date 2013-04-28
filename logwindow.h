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


#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QWidget>
#include <QFile>
#include <QString>
#include <QCloseEvent>

namespace Ui {
class logWindow;
}

class logWindow : public QWidget
{
	Q_OBJECT
public:
	explicit logWindow( QString = QString(),QWidget * parent = 0 );
	~logWindow();
	void showLogWindow( QString ) ;
	void showAptGetWindow( QString ) ;
private slots:
	void pbClearLog( void ) ;
	void pbQuit( void ) ;
	void updateLogWindow( void ) ;
private:
	QString getLogContents( void ) ;
	void closeEvent( QCloseEvent * ) ;
	Ui::logWindow * m_ui;
	QString m_logFile ;
};

#endif // LOGWINDOW_H
