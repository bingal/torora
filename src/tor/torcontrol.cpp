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
#include <qdir.h>

/*FIXME: Check for use of exitnodes when connecting, so locationbar icon is always accurate */
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
    strictExitNodes(!cc.isEmpty());
    if (!cc.isEmpty())
      sendToServer(QString(QLatin1String("SETCONF ExitNodes={%1}")).arg(cc));
    else
      sendToServer(QString(QLatin1String("SETCONF ExitNodes=")));
    newIdentity();
}

void TorControl::getExitCountry()
{
    if (m_state != AUTHENTICATED) {
        return;
    }
    sendToServer(QString(QLatin1String("GETCONF ExitNodes")));
}

bool TorControl::geoBrowsingCapable()
{
    /* If Tor version < 0.2.1.X then not supported */
    qDebug() << m_versionTor << endl;
    if ((m_versionTor.mid(0,1).toInt() < 1) &&
       (m_versionTor.mid(2,1).toInt() < 3) &&
       (m_versionTor.mid(4,1).toInt() < 1))
        return false;
    return true;
}

void TorControl::strictExitNodes( bool strict )
{
    if (strict)
        sendToServer(QLatin1String("SETCONF StrictExitNodes=1"));
    else
        sendToServer(QLatin1String("SETCONF StrictExitNodes=0"));

}

void TorControl::parseExitNodes(const QString &line)
{
    QStringList nodes = line.split(QLatin1String(","));
    QStringList countriesInUse;
    for ( QStringList::Iterator it = nodes.begin(); it != nodes.end(); ++it ) {
        if (m_countrycodes.contains((*it).remove(QLatin1String("{")).remove(QLatin1String("}"))))
            countriesInUse << (*it);
    }
    if (countriesInUse.isEmpty() || (countriesInUse.count() > 1))
        emit geoBrowsingUpdate(6);
    else
        emit geoBrowsingUpdate(m_countrycodes.indexOf(countriesInUse.first()));
}

void TorControl::authenticateWithPassword(const QString &password)
{
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
    cookieCandidates << QString(QLatin1String("%1\\.tor\\control_auth_cookie"))
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
    /* Get the list of open circuits and close them. This will
       force Tor to build circuits obeying the new rules. */
    sendToServer(QLatin1String("GETINFO circuit-status"));
    /* Tell Tor to use a new circuit for the next connection. */
    sendToServer(QLatin1String("signal newnym"));
}

void TorControl::closeCircuit(const QString &circid)
{
    sendToServer(QString(QLatin1String("CLOSECIRCUIT %1")).arg(circid));
}

void TorControl::socketReadyRead()
{
    while ( socket->canReadLine() ) {

          QString line = QLatin1String(socket->readLine().trimmed());
          QString code;
          QStringList tokens;
          qDebug() << line << endl;
          switch (m_state) {
              case AUTHENTICATING:
                  if (line.contains(QLatin1String("250 OK"))){
                      m_state = AUTHENTICATED;
                      getExitCountry();
                      /* If the user had to enter a password then they're expecting to see the
                         geobrowsing menu pop up. */
                      if (m_authMethods.contains(QLatin1String("HASHEDPASSWORD")))
                          emit showGeoBrowsingMenu();
                      continue;
                  }
                  code = line.left(3);
                  /*Incorrect password*/
                  if (code == QLatin1String("515")){
                      reconnect();
                      emit requestPassword(tr("<qt>The password you entered was incorrect. <br>"
                                              "Try entering it again or click 'Cancel' for help:</qt>"));
                  }
                  break;
              case AUTHENTICATED:
                  if (line.contains(QLatin1String("250+circuit-status="))){
                      m_state=LISTING_CIRCUITS;
                      continue;
                  }
                  if (line.contains(QLatin1String("250 ExitNodes="))){
                      line.remove(QLatin1String("250 ExitNodes="));
                      if (!line.isEmpty())
                          parseExitNodes(line);
                      continue;
                  }
                  break;
              case LISTING_CIRCUITS:
                  if (line.contains(QLatin1String("250 OK"))){
                      m_state=AUTHENTICATED;
                      continue;
                  }
                  tokens = line.split(QLatin1String(" "));
                  if (tokens.first().toInt() != 0)
                      closeCircuit(tokens.first());
                  break;
              case PREAUTHENTICATING:
                  if (line.contains(QLatin1String("250 OK"))){
                      m_state=AUTHENTICATING;
                      continue;
                  }
                  if (line.contains(QLatin1String("250-VERSION Tor="))){
                      line.remove(QLatin1String("250-VERSION Tor="));
                      line.remove(QLatin1String("\""));
                      m_versionTor = line;
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
    delete socket;
}

