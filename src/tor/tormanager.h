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

#ifndef _TORMANAGER_H_
#define _TORMANAGER_H_

#include <QObject>
#include <qicon.h>
#include <qpointer.h>
#include <qurl.h>
#include <qhttp.h>

class AppCheck;
class TorControl;

class TorManager : public QObject
{
    Q_OBJECT
public:
    TorManager();

    virtual ~TorManager();

    bool torIsRunning(){ return m_torIsRunning;};
    bool privoxyIsRunning(){ return m_privoxyIsRunning;};
    bool polipoIsRunning(){ return m_polipoIsRunning;};
    void checkApps();
    void checkTorInstallation(bool checkTorSilently);

private slots:
    void updateTorStatus(bool connected) { m_torIsRunning = connected; };
    void updatePrivoxyStatus(bool connected) { m_privoxyIsRunning = connected; };
    void updatePolipoStatus(bool connected) { m_polipoIsRunning = connected; };
    void torCheckComplete(bool error);
    void reportTorCheckResults(int page);
    void displayStatusResult();
    void checkTorSilently();
    void checkTorExplicitly();

private:
    AppCheck *tor;
    AppCheck *privoxy;
    AppCheck *polipo;
    AppCheck *userProxy;
    TorControl *torcontrol;
    bool m_torIsRunning;
    bool m_privoxyIsRunning;
    bool m_polipoIsRunning;
    bool m_checkTorSilently;
    QHttp *http;
    QString m_statusbar;

};

#endif //


