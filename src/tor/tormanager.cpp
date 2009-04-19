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
#include <QXmlStreamReader>
#include <QDesktopServices>
#include <QMessageBox>
#include <QMenu>

#include "appcheck.h"
#include "ui_passworddialog.h"
#include "torcontrol.h"
#include "tormanager.h"

#include "browserapplication.h"
#include "browsermainwindow.h"
#include "tabwidget.h"
#include "toolbarsearch.h"
#include "webview.h"
#include "networkaccessmanager.h"

#define TOR_CHECK 1
#define USING_TOR 2
#define INSTALLATION_BROKEN 3
#define NOT_USING_TOR 4

#define PRIVOXY 8118
#define POLIPO  8123

TorManager::TorManager()
{
    tor=0L;
    privoxy = 0L;
    polipo = 0L;
    userProxy = 0L;
    torcontrol = 0L;
    m_checkTorSilently = false;
    m_proxyConfigured = false;

#ifndef Q_OS_WIN
    privoxyConfigFiles << QLatin1String("/etc/privoxy/config");
    privoxyConfigFiles << QLatin1String("/usr/etc/privoxy/config");
    privoxyConfigFiles << QLatin1String("/usr/local/etc/privoxy/config");
#else
    privoxyConfigFiles << QString(QLatin1String("c:\\program files\\Vidalia Bundle\\Privoxy\\config.txt"));
    privoxyConfigFiles << QString(QLatin1String("c:\\program files\\Privoxy\\config.txt"));
    privoxyConfigFiles << QString(QLatin1String("%1\\Vidalia Bundle\\Privoxy\\config.txt"))
                        .arg(QDesktopServices::storageLocation(QDesktopServices::ApplicationsLocation));
    privoxyConfigFiles << QString(QLatin1String("%1\\Privoxy\\config.txt"))
                        .arg(QDesktopServices::storageLocation(QDesktopServices::ApplicationsLocation));
    qDebug() << privoxyConfigFiles << endl;
#endif

#ifndef Q_OS_WIN
    polipoConfigFiles << QLatin1String("/etc/polipo/config");
    polipoConfigFiles << QLatin1String("/usr/etc/polipo/config");
    polipoConfigFiles << QLatin1String("/usr/local/etc/polipo/config");
#else
    polipoConfigFiles << QString(QLatin1String("c:\\program files\\Polipo\\config.txt"));
    polipoConfigFiles << QString(QLatin1String("c:\\program files\\Vidalia Bundle\\Polipo\\config.txt"));
    polipoConfigFiles << QString(QLatin1String("%1\\Vidalia Bundle\\Polipo\\config.txt"))
                        .arg(QDesktopServices::storageLocation(QDesktopServices::ApplicationsLocation));
    polipoConfigFiles << QString(QLatin1String("%1\\Polipo\\config.txt"))
                        .arg(QDesktopServices::storageLocation(QDesktopServices::ApplicationsLocation));
    qDebug() << polipoConfigFiles << endl;
#endif

    checkApps();
    connectToTor();
    m_countries = new Countries();
}

void TorManager::checkApps()
{
    QNetworkProxy proxy = BrowserApplication::instance()->networkAccessManager()->currentProxy();
    QRegExp polipoTorConf(QLatin1String("^([^#]+|)socksParentProxy[^#]+=[^#]+\"(localhost|127.0.0.1):9050\""));
    QRegExp privoxyTorConf(QLatin1String("^([^#]+|)forward-socks4a[^#]+/[^#]+(localhost|127.0.0.1):9050 \\."));

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

    if (proxy.port() != PRIVOXY && proxy.port() != POLIPO) {
        if (userProxy)
            delete userProxy;
        userProxy = new AppCheck( QLatin1String("localhost"), proxy.port() );
        connect(userProxy, SIGNAL(connectedToApp(bool)),
                this, SLOT(updateUserProxyStatus(bool)));
    }

    m_proxyConfigured = validProxyConfiguration((proxy.port()==POLIPO) ?
                                                  polipoConfigFiles : privoxyConfigFiles,
                                                (proxy.port()==POLIPO) ?
                                                  polipoTorConf : privoxyTorConf);
    qDebug() << m_proxyConfigured << endl;
}


bool TorManager::validProxyConfiguration(const QStringList &proxyConfigFiles, QRegExp &rx)
{
    for ( QStringList::ConstIterator it = proxyConfigFiles.begin(); it != proxyConfigFiles.end(); ++it ) {
        QFile file((*it));
        if (!file.open(QFile::ReadOnly))
            continue;
        QTextStream stream(&file);

        do {
            if (rx.indexIn(stream.readLine()) != -1)
                return true;
        } while (!stream.atEnd());
    }

    return false; 
}

TorManager::~TorManager()
{

}

void TorManager::torCheckComplete(bool error)
{
    int result = INSTALLATION_BROKEN;
    if (!error) {
      QString target;
      QByteArray data;
      data = http->readAll();
      if (data.isNull())
          return;

      QXmlStreamReader xml(data);
      while (!xml.atEnd()) {
          if (!xml.attributes().value(QLatin1String("target")).isNull()) {
              target = xml.attributes().value(QLatin1String("target")).toString();
              break;
          }
          xml.readNext();
      }
      if (target == QLatin1String("success")) {
          setBrowsingEnabled(true);
          result = USING_TOR;
      } else if (target == QLatin1String("failure")){
          result = NOT_USING_TOR;
      }
    }
    reportTorCheckResults(result);
}

void TorManager::setBrowsingEnabled(bool enabled)
{
    BrowserApplication::instance()->mainWindow()->tabWidget()->setLocationBarEnabled(enabled);
    BrowserApplication::instance()->mainWindow()->toolbarSearch()->setEnabled(enabled);
    BrowserApplication::instance()->mainWindow()->enableBookmarksToolbar(enabled);
    BrowserApplication::instance()->mainWindow()->tabWidget()->setEnabled(enabled);
    BrowserApplication::instance()->mainWindow()->geoBrowsingAction()->setEnabled(enabled);
    BrowserApplication::instance()->mainWindow()->stopReloadAction()->setEnabled(enabled);
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
      case USING_TOR:
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
        setBrowsingEnabled(false);
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
        } else if (page == NOT_USING_TOR) {
            bulletone = tr("Testing at https://check.torproject.org indicated that you are not using Tor.");
            if (!m_proxyConfigured) {
                headline = tr("Torora Is By-Passing Tor!");
                bullettwo = tr("<b>A primitive check of your configuration suggests that %1 is not configured to use Tor.</b>")
                          .arg((proxy.port()==PRIVOXY)?QLatin1String("Privoxy"):QLatin1String("Polipo"));
                bulletfour = tr("<li>Check your configuration..</li>");
            } else {
                headline = tr("Torora May Be By-Passing Tor!");
                bullettwo = tr("Your set-up seems OK, Tor and %1 are running and seem to be correctly configured.").
                            arg((proxy.port()==PRIVOXY)?QLatin1String("Privoxy"):QLatin1String("Polipo"));
                bulletfour = tr("<li>Click 'Change Identity' in Vidalia or TorK and try again. The exit node used for the test may not be listed with the checking service yet.</li>");
            }
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

    if (page == USING_TOR) {
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
      setBrowsingEnabled(false);
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

bool TorManager::readyToUse()
{
    if (torcontrol && torcontrol->readyToUse())
        return true;
    return false;
}

void TorManager::authenticate()
{
    torcontrol->authenticate();
}

void TorManager::connectToTor()
{
    if (torcontrol)
        return;
    torcontrol = new TorControl(QLatin1String("localhost"), 9051 );
    connect(torcontrol, SIGNAL(requestPassword(const QString &)),
            this, SLOT(requestPassword(const QString &)));
    connect(torcontrol, SIGNAL(showGeoBrowsingMenu()),
            this, SLOT(showGeoBrowsingMenu()));
}

void TorManager::setGeoBrowsingLocation(int offset)
{
    if (!torcontrol->geoBrowsingCapable()) {
        QMessageBox::information(BrowserApplication::instance()->mainWindow(),
            tr("Your Version of Tor Does Not Support This Feature."),
            tr("Your version of Tor does not support GeoBrowsing. <br> Install Tor 0.2.1.X or "
               "later if you want to browse the internet from specific countries."));
        return;
    }
    torcontrol->setExitCountry(m_countries->country(offset)->cc());
    emit geoBrowsingUpdate(offset);
}

void TorManager::showGeoBrowsingMenu()
{
    BrowserApplication::instance()->mainWindow()->geoBrowsingMenu()->hide();
    BrowserApplication::instance()->mainWindow()->aboutToShowGeoBrowsingMenu();
    BrowserApplication::instance()->mainWindow()->geoBrowsingMenu()->show();
}

void TorManager::requestPassword(const QString &message)
{
    BrowserMainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    Ui::PasswordDialog passwordDialog;
    passwordDialog.setupUi(&dialog);

    passwordDialog.iconLabel->setText(QLatin1String("Tor Password"));
    passwordDialog.iconLabel->setPixmap(QPixmap(QLatin1String(":tor-logo.png")));

    QString introMessage = message;
    passwordDialog.introLabel->setText(introMessage);
    passwordDialog.introLabel->setWordWrap(true);
    passwordDialog.userNameLineEdit->setVisible(false);
    passwordDialog.label->setVisible(false);
    if (dialog.exec() == QDialog::Accepted) {
        torcontrol->authenticateWithPassword(passwordDialog.passwordLineEdit->text());
    } else {
        passwordHelp();
    }
}

void TorManager::passwordHelp()
{
    QFile file(QLatin1String(":/passwordhelp.html"));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "WebPage::handleUnsupportedContent" << "Unable to open torcheck.html";
        return;
    }
    QString tororaIssues;
    QString title, headline, intro, bullettwo, bulletthree, bulletfour, img;
    headline = tr("To Enable Geo-Browsing You Need To Set Tor's Password.");
    intro = tr("Open up the Vidalia 'Advanced' settings dialog.");
    bullettwo = tr("Clear the 'Randomly Generate Password' check-box.");
    bulletthree = tr("Type in a password of your choosing and click 'OK'.");
    bulletfour = tr("Enter this password when Torora requests it.");
    img = QLatin1String(":vidalia-password.png");

    QString html = QString(QLatin1String(file.readAll()))
                        .arg(title)
                        .arg(QString())
                        .arg(headline)
                        .arg(intro)
                        .arg(bullettwo)
                        .arg(bulletthree)
                        .arg(bulletfour);

    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    QIcon icon = QIcon(img);
    QPixmap pixmap = icon.pixmap(QSize(546, 219));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("IMAGE_BINARY_DATA_HERE"),
                     QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    imageBuffer.open(QBuffer::ReadWrite);
    icon = QIcon(QLatin1String(":help.png"));
    pixmap = icon.pixmap(QSize(32, 32));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("INFO_BINARY_DATA_HERE"),
                    QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    BrowserApplication::instance()->mainWindow()->currentTab()->page()->mainFrame()->setHtml(html, QUrl());

}
