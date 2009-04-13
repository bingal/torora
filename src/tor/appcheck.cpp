/****************************************************************************
 *   Copyright (C) 2006 - 2008 Robert Hogan                                *
 *   robert@roberthogan.net                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/


#include <QTcpSocket>
#include <QDesktopServices>
#include <qtextstream.h>
#include <qstringlist.h>
#include "appcheck.h"


AppCheck::AppCheck( const QString &host, int port )
{
    // create the socket and connect various of its signals
    socket = new QTcpSocket( this );
    connect( socket, SIGNAL(connected()),
            SLOT(socketConnected()) );
    connect( socket, SIGNAL(readyRead()),
            SLOT(socketReadyRead()) );
    connect( socket, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(socketError(QAbstractSocket::SocketError)) );

    // connect to the server
    socket->connectToHost( host, port );

}


void AppCheck::socketReadyRead()
{
    QString line;
    // read from the server
    while ( socket->canReadLine() ) {

        line = QLatin1String(socket->readLine().trimmed());

        qDebug() << line << endl;

    }
}

AppCheck::~AppCheck()
{

}

