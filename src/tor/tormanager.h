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
#include "countries.h"

#include <QObject>
#include <qicon.h>
#include <qpointer.h>
#include <qurl.h>
#include <qhttp.h>

class AppCheck;
class TorControl;
class Countries;
class Country;

class TorManager : public QObject
{
    Q_OBJECT
public:
    TorManager();

    virtual ~TorManager();

    bool torIsRunning(){ return m_torIsRunning;}
    void checkApps();
    void checkTorInstallation(bool checkTorSilently);
    void setGeoBrowsingLocation(int offset);
    Countries* countries(){ return m_countries; }
    bool readyToUse();

signals:
    void geoBrowsingUpdate(int offset);

private slots:
    void updateTorStatus(bool connected) { m_torIsRunning = connected; }
    void torCheckComplete(bool error);
    void reportTorCheckResults(int page);
    void displayStatusResult();
    void torShutDownUnexpectedly();

public slots:
    void checkTorSilently();
    void checkTorExplicitly();
    void requestPassword(const QString &);
    void showGeoBrowsingMenu();
    void runServer();
    void enableRelay();
    void authenticate();
    
private:
    void setBrowsingEnabled(bool enabled);
    void passwordHelp();
    void connectToTor();
    void serverRunning();

    AppCheck *tor;
    TorControl *torcontrol;
    bool m_torIsRunning;
    bool m_proxyConfigured;
    bool m_checkTorSilently;
    QHttp *http;
    QString m_statusbar;
    Countries *m_countries;
    QTimer *m_timer;
    bool m_displayedAlready;
    QString m_country;
};

#endif //


