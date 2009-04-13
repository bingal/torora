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


#include <QHttp>
#include <QTimer>
#include <QFile>
#include <QBuffer>
#include <QStatusBar>
#include <qwebframe.h>

#include "tormanager.h"
#include "torcontrol.h"
#include "appcheck.h"
#include "browserapplication.h"
#include "browsermainwindow.h"
#include "tabwidget.h"
#include "toolbarsearch.h"
#include "webview.h"
#include "networkaccessmanager.h"

#define TOR_CHECK 1
#define TOR_SUCCESS 2
#define TOR_FAIL 3
#define PRIVOXY 8118
#define POLIPO  8123

TorManager::TorManager()
{
    tor=0L;
    privoxy = 0L;
    polipo = 0L;
    userProxy = 0L;
    m_checkTorSilently = false;

    checkApps();
    torcontrol = new TorControl(QLatin1String("localhost"), 9051 );

}

void TorManager::checkApps()
{
    if (tor)
        delete tor;
    tor = new AppCheck( QLatin1String("localhost"), 9050 );
    connect(tor, SIGNAL(connectedToApp(bool)), this, SLOT(updateTorStatus(bool)));

    if (privoxy)
        delete privoxy;
    privoxy = new AppCheck( QLatin1String("localhost"), PRIVOXY );
    connect(privoxy, SIGNAL(connectedToApp(bool)), this, SLOT(updatePrivoxyStatus(bool)));

    if (polipo)
        delete polipo;
    polipo = new AppCheck( QLatin1String("localhost"), POLIPO );
    connect(polipo, SIGNAL(connectedToApp(bool)), this, SLOT(updatePolipoStatus(bool)));

    QNetworkProxy proxy = BrowserApplication::instance()->networkAccessManager()->currentProxy();
    if (proxy.port() != PRIVOXY && proxy.port() != POLIPO) {
        if (userProxy)
            delete userProxy;
        userProxy = new AppCheck( QLatin1String("localhost"), proxy.port() );
        connect(userProxy, SIGNAL(connectedToApp(bool)),
                this, SLOT(updateUserProxyStatus(bool)));
    }
}

TorManager::~TorManager()
{

}

void TorManager::torCheckComplete(bool error)
{
    int success = TOR_FAIL;
    if (!error) {
      QByteArray data;
      data = http->readAll();
      if (data.isNull())
          return;
      QByteArray pass("<html xmlns=\"http://www.w3.org/1999/xhtml\"><body>"
                      "<a id=\"TorCheckResult\" target=\"success\" href=\"/\""
                      "></a></body></html>");
      qDebug() << data << endl;
      if (pass == data) {
          BrowserApplication::instance()->mainWindow()->tabWidget()->setLocationBarEnabled(true);
          BrowserApplication::instance()->mainWindow()->toolbarSearch()->setEnabled(true);
          BrowserApplication::instance()->mainWindow()->enableBookmarksToolbar(true);
          BrowserApplication::instance()->mainWindow()->tabWidget()->setEnabled(true);
          success = TOR_SUCCESS;
      }
    }
    reportTorCheckResults(success);
}

void TorManager::reportTorCheckResults(int page)
{
    QString statusbar;
    QNetworkProxy proxy = BrowserApplication::instance()->networkAccessManager()->currentProxy();

    QFile file(QLatin1String(":/torcheck.html"));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "WebPage::handleUnsupportedContent" << "Unable to open torcheck.html";
        return;
    }
    QString tororaIssues;
    QString issuesfile;
#if defined(TORORA)
#if defined(TORORA_WEBKIT_BUILD)
    issuesfile = QLatin1String(":/TORORA_WEBKIT_BUILD_ISSUES");
#else
    issuesfile = QLatin1String(":/TORORA_ISSUES");
#endif
#else
    issuesfile = QLatin1String(":/ARORA_ISSUES");
#endif
    QFile issues(issuesfile);
    QString title, headline, bulletone, bullettwo, bulletthree, bulletfour, img;
    switch (page) {
      case TOR_CHECK:
        if (m_checkTorSilently)
            return;
        title = tr("Checking Tor..");
        headline = tr("Checking Tor..");
        bulletone = tr("Torora is checking https://check.torproject.org.");
        bullettwo = tr("Once Torora is sure you can browse anonymously, browsing will be enabled.");
        bulletthree = tr("This check may take a few seconds, so please be patient.");
        img = QLatin1String(":tor-checking.png");
        statusbar = QLatin1String("Checking Tor...");
        break;
      case TOR_SUCCESS:
        if (m_checkTorSilently)
            return;
        if (!issues.open(QIODevice::ReadOnly)) {
            qWarning() << "WebPage::handleUnsupportedContent" << "Unable to open " << issuesfile;
            return;
        }
        tororaIssues = QString(QLatin1String(issues.readAll()));
        title = tr("Torora Ready For Use..");
        headline = tr("Tor is Working Properly. You Can Browse Anonymously.");
        bulletone = tr("You can confirm this yourself by visiting <a href='https://check.torproject.org'>https://check.torproject.org</a>");
        bullettwo = tr("The bookmark toolbar contains some well known hidden services you can check out.");
        bulletthree = tr("You can check Tor at any time by pressing F12 or clicking <b>Tools->Check Tor.</b>");
        img = QLatin1String(":tor-on.png");
        statusbar = QLatin1String("Tor Check Successful");
       break;
      default:
        title = tr("Check Your Tor Installation");
        if (!m_torIsRunning) {
            headline = tr("Tor Is Not Running On Your Computer!");
            bulletone = tr("Check that you have installed Tor.");
            bullettwo = tr("Check that you have started Tor.");
        } else if (proxy.port() == POLIPO && !m_polipoIsRunning) {
            headline = tr("Polipo Privacy Proxy Is Not Running On Your Computer!");
            bulletone = tr("Check that you have installed Polipo.");
            bullettwo = tr("Check that you have started Polipo.");
        } else if (proxy.port() == PRIVOXY && !m_privoxyIsRunning) {
            headline = tr("Privoxy Is Not Running On Your Computer!");
            bulletone = tr("Check that you have installed Privoxy.");
            bullettwo = tr("Check that you have started Privoxy.");
        } else {
            headline = tr("The Tor Check Website May Be Down!");
            bulletone = tr("Check that https://check.torproject.org is available using another browser.");
            bullettwo = tr("There may be a temporary issue with the website.");
            bulletfour = tr("<li>Click 'Change Identity' in Vidalia or TorK and try again. The exit node used for the test may not be listed with the checking service yet.</li>");
        }
        bulletthree = tr("Press F12 or <b>Tools->Check Tor</b> to test Tor again.");
        img = QLatin1String(":tor-off.png");
        statusbar = QLatin1String("Tor Check Failed");
        break;
    }
    QString html = QString(QLatin1String(file.readAll()))
                        .arg(title)
                        .arg(QString())
                        .arg(headline)
                        .arg(bulletone)
                        .arg(bullettwo)
                        .arg(bulletthree)
                        .arg(bulletfour)
                        .arg(tororaIssues);

    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    QIcon icon = QIcon(img);
    QPixmap pixmap = icon.pixmap(QSize(32, 32));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("IMAGE_BINARY_DATA_HERE"),
                     QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    if (page == TOR_SUCCESS) {
        /*FIXME: Pseudorandom intervals may not be enough to prevent an attacker guessing who
                is testing */
        #define TOR_CHECK_PERIOD 60 * 1000 * (qrand() % 10)
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(checkTorSilently()));
        timer->start(TOR_CHECK_PERIOD);

        imageBuffer.open(QBuffer::ReadWrite);
        icon = QIcon(QLatin1String(":info.png"));
        pixmap = icon.pixmap(QSize(32, 32));
        if (pixmap.save(&imageBuffer, "PNG")) {
            html.replace(QLatin1String("INFO_IMAGE_HERE"),
                        QString(QLatin1String(imageBuffer.buffer().toBase64())));
        }

        imageBuffer.open(QBuffer::ReadWrite);
        icon = QIcon(QLatin1String(":important.png"));
        pixmap = icon.pixmap(QSize(32, 32));
        if (pixmap.save(&imageBuffer, "PNG")) {
            html.replace(QLatin1String("WARNING_IMAGE_HERE"),
                        QString(QLatin1String(imageBuffer.buffer().toBase64())));
        }

        imageBuffer.open(QBuffer::ReadWrite);
        icon = QIcon(QLatin1String(":help.png"));
        pixmap = icon.pixmap(QSize(32, 32));
        if (pixmap.save(&imageBuffer, "PNG")) {
            html.replace(QLatin1String("QUESTION_IMAGE_HERE"),
                        QString(QLatin1String(imageBuffer.buffer().toBase64())));
        }
    }
    BrowserApplication::instance()->mainWindow()->currentTab()->page()->mainFrame()->setHtml(html, QUrl());

    if (!m_checkTorSilently) {
        m_statusbar=statusbar;
        QTimer::singleShot(1000, this, SLOT(displayStatusResult()));
        if (page != TOR_CHECK)
            BrowserApplication::instance()->mainWindow()->setStatusBarMessagesEnabled(true);
    }
}

void TorManager::checkTorInstallation(bool checkTorSilently)
{
    checkApps();
    m_checkTorSilently = checkTorSilently;
    if (!m_checkTorSilently) {
      BrowserApplication::instance()->mainWindow()->toolbarSearch()->setEnabled(false);
      BrowserApplication::instance()->mainWindow()->tabWidget()->setLocationBarEnabled(false);
      BrowserApplication::instance()->mainWindow()->enableBookmarksToolbar(false);
      BrowserApplication::instance()->mainWindow()->tabWidget()->setEnabled(false);
      BrowserApplication::instance()->mainWindow()->setStatusBarMessagesEnabled(false);
      reportTorCheckResults(TOR_CHECK);
    }
    http = new QHttp(QLatin1String("check.torproject.org"),
                     QHttp::ConnectionModeHttps, 443, this);
    QNetworkProxy proxy = BrowserApplication::instance()->networkAccessManager()->currentProxy();
    http->setProxy(proxy);
    http->get(QLatin1String("/?TorButton=True"));

    connect(http, SIGNAL(done(bool)),
            this, SLOT(torCheckComplete(bool)));
}

void TorManager::checkTorSilently()
{
    checkTorInstallation(true);
}

void TorManager::checkTorExplicitly()
{
    checkTorInstallation(false);
}

void TorManager::displayStatusResult()
{
     BrowserApplication::instance()->mainWindow()->statusBar()->showMessage(m_statusbar);
}
