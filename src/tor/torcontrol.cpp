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
    m_host = host;
    m_port = port;
    m_password = QString();
    m_authMethods = QStringList();

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

    reconnect();
}

void TorControl::reconnect()
{
    // connect to the server
    m_state = PREAUTHENTICATING;
    socket->disconnectFromHost();
    socket->connectToHost( m_host, m_port );

}
void TorControl::setExitCountry(const QString &cc)
{
    if (m_state != AUTHENTICATED) {
        return;
    }
    strictExitNodes(true);
    if (!cc.isEmpty())
      sendToServer(QString(QLatin1String("SETCONF ExitNodes={%1}")).arg(cc));
    else
      sendToServer(QString(QLatin1String("SETCONF ExitNodes=")));
    sendToServer(QLatin1String("signal newnym"));
}

void TorControl::strictExitNodes( bool strict )
{
    if (strict)
        sendToServer(QLatin1String("SETCONF StrictExitNodes=1"));
    else
        sendToServer(QLatin1String("SETCONF StrictExitNodes=0"));

}


void TorControl::authenticateWithPassword(const QString &password)
{
    qDebug() << "auth with pass" << endl;
    m_password = password;
    authenticate();
}

void TorControl::protocolInfo()
{
    m_state = PREAUTHENTICATING;
    sendToServer(QLatin1String("PROTOCOLINFO"));
}

void TorControl::authenticate()
{
    qDebug() << "authenticate" << endl;
    if (m_authMethods.contains(QLatin1String("HASHEDPASSWORD"))) {
        if (!m_password.isEmpty())
            sendToServer(QString(QLatin1String("AUTHENTICATE \"%1\"")).arg(m_password));
        else {
            emit requestPassword(tr("<qt>Tor Requires A Password for GeoBrowsing Access. <br>"
                                    "Enter it or click 'Cancel' for help.</qt>"));
        }
    } else if (m_authMethods.contains(QLatin1String("COOKIE"))) {
        readCookie();
    } else
        sendToServer(QLatin1String("AUTHENTICATE"));
}


bool TorControl::readCookie()
{

    QStringList cookieCandidates;
#ifndef Q_OS_WIN
    cookieCandidates << QString(QLatin1String("%1/.tor/control_auth_cookie"))
                        .arg(QDesktopServices::HomeLocation);
    cookieCandidates << QLatin1String("/var/lib/tor/control_auth_cookie");
#else
    cookieCandidates << QString(QLatin1String("%1\.tor\control_auth_cookie"))
                        .arg(QDesktopServices::HomeLocation);
#endif
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
    while ( socket->canReadLine() ) {

          QString line = QLatin1String(socket->readLine().trimmed());
          qDebug() << line << endl;
          qDebug() << m_state << endl;
          QString code;
          switch (m_state) {
              case AUTHENTICATING:
                  if (line.contains(QLatin1String("250 OK"))){
                      m_state = AUTHENTICATED;
                      continue;
                  }
                  code = line.left(3);
                  /*Incorrect password*/
                  if (code == QLatin1String("515")){
                      qDebug() << "failed" << line << endl;
                      reconnect();
                      emit requestPassword(tr("<qt>The password you entered was incorrect. <br>"
                                              "Try entering it again or click 'Cancel' for help:</qt>"));
                  }
                  break;
              case PREAUTHENTICATING:
                  if (line.contains(QLatin1String("250 OK"))){
                      m_state=AUTHENTICATING;
                      continue;
                  }
                  if (line.contains(QLatin1String("250-AUTH METHODS="))){
                      line.remove(QLatin1String("250-AUTH METHODS="));
                      m_authMethods = line.split(QLatin1String(","));
                      /* If there's no auth method we can just make the control
                         session ready for use now. Otherwise, we wait until it's
                         required and prompt for a password then.*/
                      if (m_authMethods.contains(QLatin1String("NULL")))
                          authenticate();
                  }
                  break;
          }
    }
}

TorControl::~TorControl()
{

}

