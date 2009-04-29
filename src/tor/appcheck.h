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

#ifndef _APPCHECK_H_
#define _APPCHECK_H_

#include <QTcpSocket>
#include <qtextstream.h>
#include <qlistview.h>
#include <qdebug.h>


class AppCheck : public QObject
{
    Q_OBJECT
public:
    AppCheck( const QString &host, int port );

    virtual ~AppCheck();


signals:

    void fatalError();
    void serverError();
    void torConnectionClosed();
    void connectedToApp( bool connected);
    void appShutDownUnexpectedly();

public slots:
    void socketReadyRead();

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
       emit connectedToApp(true);
    }


    void socketClosed()
    {
    }

    void socketError( QAbstractSocket::SocketError e )
    {
         if ( e == QAbstractSocket::ConnectionRefusedError ||
              e == QAbstractSocket::HostNotFoundError )
              emit connectedToApp(false);
         if ( e == QAbstractSocket::RemoteHostClosedError)
              emit appShutDownUnexpectedly();
    }

private:
    QTcpSocket *socket;
};

#endif //


