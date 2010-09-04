/****************************************************************************
 ** $Id: torclient.h,v 1.76 2009/01/17 15:49:08 hoganrobert Exp $
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
 ***************************************************************************
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#ifndef _TORCONTROL_H_
#define _TORCONTROL_H_

#include <QTcpSocket>
#include <qtextstream.h>
#include <qlistview.h>

#define PREAUTHENTICATING 1
#define AUTHENTICATING 2
#define AUTHENTICATED 3
#define LISTING_CIRCUITS 4

class TorControl : public QObject
{
    Q_OBJECT
public:
    TorControl( const QString &host, int port );

    virtual ~TorControl();


    void sendToServer(const QString &string)
    {
        if (!socket)
            return;
        QTextStream os(socket);
        os << string << "\r\n";
    }
    bool connectedToTor(){ return m_controllerWorking; };
    void newIdentity();
    bool readyToUse(){return (m_state > AUTHENTICATING)?true:false;}
    bool geoBrowsingCapable();
    void setCountries(const QStringList &countrycodes) { m_countrycodes = countrycodes; }
    bool runningServer() { return m_serverRunning; }
signals:

    void fatalError();
    void serverError();
    void torConnectionClosed();
    void authenticated();
	void authenticationFailed();
    void requestPassword(const QString &);
    void showGeoBrowsingMenu();
    void geoBrowsingUpdate(int);

public slots:
    void socketReadyRead();
    void authenticate();
    void authenticateWithPassword(const QString &password);
    void setExitCountry(const QString &cc );
    void strictExitNodes( bool strict );
    void enableRelay();

private slots:
    void closeConnection()
    {
        socket->close();
        if ( socket->state() == QAbstractSocket::ClosingState ) {
            // We have a delayed close.
            connect( socket, SIGNAL(delayedCloseFinished()),
                    SLOT(socketClosed()) );
        } else {
            // The socket is closed.
            socketClosed();
        }
    }

    void socketConnected()
    {
        protocolInfo();
    }

    void socketConnectionClosed()
    {
        emit torConnectionClosed();
    }

    void socketClosed()
    {
    }

    void socketError( QAbstractSocket::SocketError e )
    {
         if ( e == QAbstractSocket::ConnectionRefusedError ||
              e == QAbstractSocket::HostNotFoundError )
            emit fatalError();
    }


private:
    void reconnect();
    void protocolInfo();
    void closeCircuit(const QString &circid);
    void getExitCountry();
    void checkForServer();
    void parseServerStatus(const QString &line);
    void parseExitNodes(const QString &line);
    void clearSavedPassword();
    void storePassword();

    QTcpSocket *socket;
    bool readCookie();
    bool m_controllerWorking;
    bool m_serverRunning;
    QString m_host;
    int m_port;
    QString m_password;
    QString m_versionTor;
    QStringList m_authMethods;
    int m_state;
    QStringList m_countrycodes;
};

#endif //


