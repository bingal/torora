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
#include "networkproxyfactory.h"

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
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkTorSilently()));

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
    m_countries = new Countries();
}

void TorManager::checkApps()
{
    QNetworkProxy proxy = BrowserApplication::instance()->networkAccessManager()->currentProxy();
    QRegExp polipoTorConf(QLatin1String("^([^#]+|)socksParentProxy[^#]+=[^#]+\"(localhost|127.0.0.1):9050\""));
    QRegExp privoxyTorConf(QLatin1String("^([^#]+|)forward-socks4a[^#]+/[^#]+(localhost|127.0.0.1):9050 \\."));

    if (tor)
        delete tor;
    tor = new AppCheck( QLatin1String("localhost"), 9051, true );
    connect(tor, SIGNAL(connectedToApp(bool)), this, SLOT(updateTorStatus(bool)));
    connect(tor, SIGNAL(appShutDownUnexpectedly()), this, SLOT(torShutDownUnexpectedly()));

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
//    qDebug() << m_proxyConfigured << endl;
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

void TorManager::torShutDownUnexpectedly()
{
    m_torIsRunning = false;
    reportTorCheckResults(INSTALLATION_BROKEN);
}

void TorManager::privoxyShutDownUnexpectedly()
{
    m_privoxyIsRunning = false;
    reportTorCheckResults(INSTALLATION_BROKEN);
}

void TorManager::polipoShutDownUnexpectedly()
{
    m_polipoIsRunning = false;
    reportTorCheckResults(INSTALLATION_BROKEN);
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
    //BrowserApplication::instance()->mainWindow()->tabWidget()->setEnabled(enabled);
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
        img = QLatin1String(":graphics/tor-checking.png");
        statusbar = QLatin1String("Checking Tor...");
        break;
      case USING_TOR:
        /*FIXME: Pseudorandom intervals may not be enough to prevent an attacker guessing who
                is testing */
        #define TOR_CHECK_PERIOD (60 * 1000 * ((qrand() % 10) + 1))
        m_timer->start(TOR_CHECK_PERIOD);
//        qDebug() << "TOR_CHECK_PERIOD " << TOR_CHECK_PERIOD << endl;

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
        img = QLatin1String(":graphics/tor-on.png");
        statusbar = QLatin1String("Tor Check Successful");
       break;
      default:
        setBrowsingEnabled(false);
        /* Stop the periodic tor checks until we're back up */
        m_timer->stop();
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
        img = QLatin1String(":graphics/tor-off.png");
        statusbar = QLatin1String("Tor Check Failed");

        tororaIssues = QString(QLatin1String("<table align='center'><tr><td></td> <td>"
                                                      "<object type=\"application/x-qt-plugin\" classid=\"QPushButton\" "
                                                      "name=\"TryAgainButton\" height=25 width=110></object>\n"
                                                      "<script>\n"
                                                      "document.TryAgainButton.text = 'Try Again';\n"
                                                      "</script>\n"
                                                      "</td>  <td></td></tr></table>\n"));
        /* Create a new circuit for the next test, just in case we used an exit that isn't listed yet */
        if (torcontrol)
            torcontrol->newIdentity();
        setBrowsingEnabled(false);
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
        imageBuffer.open(QBuffer::ReadWrite);
        icon = QIcon(QLatin1String(":graphics/info.png"));
        pixmap = icon.pixmap(QSize(32, 32));
        if (pixmap.save(&imageBuffer, "PNG")) {
            html.replace(QLatin1String("INFO_IMAGE_HERE"),
                        QString(QLatin1String(imageBuffer.buffer().toBase64())));
        }

        imageBuffer.open(QBuffer::ReadWrite);
        icon = QIcon(QLatin1String(":graphics/important.png"));
        pixmap = icon.pixmap(QSize(32, 32));
        if (pixmap.save(&imageBuffer, "PNG")) {
            html.replace(QLatin1String("WARNING_IMAGE_HERE"),
                        QString(QLatin1String(imageBuffer.buffer().toBase64())));
        }

        imageBuffer.open(QBuffer::ReadWrite);
        icon = QIcon(QLatin1String(":graphics/help.png"));
        pixmap = icon.pixmap(QSize(32, 32));
        if (pixmap.save(&imageBuffer, "PNG")) {
            html.replace(QLatin1String("QUESTION_IMAGE_HERE"),
                        QString(QLatin1String(imageBuffer.buffer().toBase64())));
        }
    }
    BrowserApplication::instance()->mainWindow()->currentTab()->page()->mainFrame()->setHtml(html, QUrl());

    if (!m_checkTorSilently && page == USING_TOR)
        connectToTor();

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
        delete torcontrol;
    torcontrol = new TorControl(QLatin1String("localhost"), 9051 );
    torcontrol->setCountries(m_countries->countrycodes());
    connect(torcontrol, SIGNAL(requestPassword(const QString &)),
            this, SLOT(requestPassword(const QString &)));
    connect(torcontrol, SIGNAL(showGeoBrowsingMenu()),
            this, SLOT(showGeoBrowsingMenu()));
    connect(torcontrol, SIGNAL(geoBrowsingUpdate(int)),
            this, SIGNAL(geoBrowsingUpdate(int)));
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
    m_country = m_countries->country(offset)->friendlyName();
    emit geoBrowsingUpdate(offset);
}

void TorManager::showGeoBrowsingMenu()
{
    BrowserApplication::instance()->mainWindow()->showGeoBrowsingMenu();
}

void TorManager::requestPassword(const QString &message)
{
    BrowserMainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    Ui::PasswordDialog passwordDialog;
    passwordDialog.setupUi(&dialog);

    passwordDialog.iconLabel->setText(QLatin1String("Tor Password"));
    passwordDialog.iconLabel->setPixmap(QPixmap(QLatin1String(":graphics/tor-logo.png")));

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
    QString proceedButton = QString(QLatin1String("<object type=\"application/x-qt-plugin\" classid=\"QPushButton\" name=\"DoneButton\" height=25 width=110></object>\n"
                            "<script>\n"
                            "document.DoneButton.text = 'Done';\n"
                            "</script>\n"));

    QFile file(QLatin1String(":/passwordhelp.html"));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "WebPage::handleUnsupportedContent" << "Unable to open torcheck.html";
        return;
    }
    QString tororaIssues;
    QString title, headline, intro, bullettwo, bulletthree, bulletfour, bulletfive, img;
    headline = tr("To Use Mgeni You Need To Set Tor's Password.");
    intro = tr("1. Right click the 'green onion' at the bottom right of your screen and choose 'Settings'.");
    bullettwo = tr("2. Select 'Advanced' in the 'Settings' dialog.");
    bulletthree = tr("3. Clear the 'Randomly Generate' check-box.");
    bulletfour = tr("4. Type in a password of your choosing and click 'OK'.");
    bulletfive = tr("5. Click 'Done' below and enter this password when Mgeni requests it.");
    img = QLatin1String(":graphics/vidalia-password.png");

    QString html = QString(QLatin1String(file.readAll()))
                        .arg(title)
                        .arg(QString())
                        .arg(headline)
                        .arg(intro)
                        .arg(bullettwo)
                        .arg(bulletthree)
                        .arg(bulletfour)
                        .arg(bulletfive)
                        .arg(proceedButton);

    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    QIcon icon = QIcon(QLatin1String(":graphics/vidalia-systray.png"));
    QPixmap pixmap = icon.pixmap(QSize(183, 137));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("HELP_ONE_DATA_HERE"),
                     QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    imageBuffer.open(QBuffer::ReadWrite);
    icon = QIcon(QLatin1String(":graphics/vidalia-advanced.png"));
    pixmap = icon.pixmap(QSize(118, 97));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("HELP_TWO_DATA_HERE"),
                     QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    imageBuffer.open(QBuffer::ReadWrite);
    icon = QIcon(QLatin1String(":graphics/vidalia-random.png"));
    pixmap = icon.pixmap(QSize(149, 71));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("HELP_THREE_DATA_HERE"),
                     QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    imageBuffer.open(QBuffer::ReadWrite);
    icon = QIcon(QLatin1String(":graphics/vidalia-pass.png"));
    pixmap = icon.pixmap(QSize(166, 78));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("HELP_FOUR_DATA_HERE"),
                     QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    imageBuffer.open(QBuffer::ReadWrite);
    icon = QIcon(QLatin1String(":graphics/help.png"));
    pixmap = icon.pixmap(QSize(32, 32));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("INFO_BINARY_DATA_HERE"),
                    QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    BrowserApplication::instance()->mainWindow()->currentTab()->page()->mainFrame()->setHtml(html, QUrl());

}


void TorManager::runServer()
{
    if (m_displayedAlready || (torcontrol && torcontrol->runningServer()))
        return;
    m_displayedAlready = true;
    QString proceedButton = QString(QLatin1String("<object type=\"application/x-qt-plugin\" classid=\"QPushButton\" name=\"RunServerButton\" height=25 width=110></object>\n"
                            "<script>\n"
                            "document.RunServerButton.text = 'Run Server';\n"
                            "</script>\n"));
    QFile file(QLatin1String(":/runserver.html"));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "WebPage::handleUnsupportedContent" << "Unable to open runserver.html";
        return;
    }
    QString tororaIssues;
    QString title, headline, intro, bullettwo, bulletthree, bulletfour, img;
    headline = tr("You're Ready to Browse Services in %1!").arg(m_country);
    intro = tr("While browsing, you can join the Tor network and help improve its performance.");
    bullettwo = tr("Joining is safe and easy, your computer will only act as an internal relay.");
    bulletthree = tr("You can use Vidalia to manage your membership");
    bulletfour = tr("Click the button below to join.");
    img = QLatin1String(":graphics/vidalia-password.png");

    QString html = QString(QLatin1String(file.readAll()))
                        .arg(title)
                        .arg(QString())
                        .arg(headline)
                        .arg(intro)
                        .arg(bullettwo)
                        .arg(bulletthree)
                        .arg(bulletfour)
                        .arg(proceedButton);

    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    QIcon icon = QIcon(QLatin1String(":graphics/info.png"));
    QPixmap pixmap = icon.pixmap(QSize(32, 32));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("INFO_BINARY_DATA_HERE"),
                    QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    BrowserApplication::instance()->mainWindow()->currentTab()->page()->mainFrame()->setHtml(html, QUrl());

}

void TorManager::serverRunning()
{
    QFile file(QLatin1String(":/serverunning.html"));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "WebPage::handleUnsupportedContent" << "Unable to open serverunning.html";
        return;
    }
    QString tororaIssues;
    QString title, headline, intro, bullettwo, bulletthree, bulletfour, img;
    headline = tr("You've joined the Tor network!");
    intro = tr("You've now added to the capacity of the Tor network for other users by running a Tor server.");
    bullettwo = tr("Running a server helps improve Tor's speed both for you and other users.");
    bulletthree = tr("Your Tor server will only handle traffic passing through the Tor network.");
    bulletfour = tr("You can manage your Tor server's configuration using the 'Setup Relaying' option in the Vidalia control panel.");
    img = QLatin1String(":graphics/vidalia-panel.png");

    QString html = QString(QLatin1String(file.readAll()))
                        .arg(title)
                        .arg(QString())
                        .arg(headline)
                        .arg(intro)
                        .arg(bullettwo)
                        .arg(bulletthree)
                        .arg(bulletfour);

//     QBuffer imageBuffer;
//     imageBuffer.open(QBuffer::ReadWrite);
//     QIcon icon = QIcon(img);
//     QPixmap pixmap = icon.pixmap(QSize(546, 219));
//     if (pixmap.save(&imageBuffer, "PNG")) {
//         html.replace(QLatin1String("IMAGE_BINARY_DATA_HERE"),
//                      QString(QLatin1String(imageBuffer.buffer().toBase64())));
//     }

    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    QIcon icon = QIcon(QLatin1String(":graphics/info.png"));
    QPixmap pixmap = icon.pixmap(QSize(32, 32));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("INFO_BINARY_DATA_HERE"),
                    QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    BrowserApplication::instance()->mainWindow()->currentTab()->page()->mainFrame()->setHtml(html, QUrl());

}

void TorManager::enableRelay()
{
    torcontrol->enableRelay();
    serverRunning();
}
