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
#include <qregexp.h>
#include "torcontrol.h"

#include <qtimer.h>
#include <assert.h>
#include <qfile.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <qdir.h>

/* Linux-specific includes */
#include <dirent.h>
#include <unistd.h>


TorControl::TorControl( const QString &host, int port )
{
    // create the socket and connect various of its signals
    socket = new QTcpSocket( this );
    connect( socket, SIGNAL(connected()),
            SLOT(socketConnected()) );
    connect( socket, SIGNAL(connectionClosed()),
            SLOT(socketConnectionClosed()) );
    connect( socket, SIGNAL(readyRead()),
            SLOT(socketReadyRead()) );
    connect( socket, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(socketError(QAbstractSocket::SocketError)) );

    // connect to the server
    qDebug() << "connecting to tor" << endl;
    socket->connectToHost( host, port );

}


void TorControl::setExitCountry(const QString &cc)
{
    sendToServer(QString(QLatin1String("SETCONF ExitNodes={%1}")).arg(cc));
    sendToServer(QLatin1String("signal newnym"));

}

void TorControl::strictExitNodes( bool strict )
{
    if (strict)
        sendToServer(QLatin1String("SETCONF StrictExitNodes=1"));
    else
        sendToServer(QLatin1String("SETCONF StrictExitNodes=0"));

}


void TorControl::authenticate()
{
    if (!readCookie())
        sendToServer(QLatin1String("AUTHENTICATE"));
}


bool TorControl::readCookie()
{

    QStringList cookieCandidates;
    cookieCandidates << QString(QLatin1String("%1/.tor/control_auth_cookie"))
                        .arg(QDesktopServices::HomeLocation);
    cookieCandidates << QLatin1String("/var/lib/tor/control_auth_cookie");

    for ( QStringList::Iterator it = cookieCandidates.begin(); it != cookieCandidates.end(); ++it ) {
        QFile inf((*it));
        if ( inf.open(QIODevice::ReadOnly) ) {
            QByteArray array = inf.readAll();
            inf.close();
            if (array.size() != 32)
                continue;
            sendToServer(QString(QLatin1String("AUTHENTICATE %1")).arg(QLatin1String(array.toHex())));
            return true;
        }
    }

    return false; 

}

void TorControl::newIdentity()
{
    sendToServer(QLatin1String("signal newnym"));
}


void TorControl::socketReadyRead()
{
    QString line;
    // read from the server
    while ( socket->canReadLine() ) {

        line = QLatin1String(socket->readLine().trimmed());

        qDebug() << line << endl;
        if (line.contains(QLatin1String("250 OK"))){
            if (!m_controllerWorking){
//                 emit authenticated();
                m_controllerWorking = true;
            }
            continue;
        }


        if (line.contains(QLatin1String("552 Unrecognized key \"ns/all\""))){
//             emit needAlphaVersion();
            continue;
        }

        QString code = line.left(3);

        if (code == QLatin1String("514")){
            QString eventInfo = line.section(QLatin1String(" "),1);
/*            emit processWarning("authenticationrequired", eventInfo);
            emit fatalError();*/
        }else if (code == QLatin1String("515")){
            QString eventInfo = line.section(QLatin1String(" "),1);
//             emit authenticationFailed();
        }


    }
}

TorControl::~TorControl()
{

}

bool TorControl::isControllerWorking()
{
    return m_controllerWorking;

}
