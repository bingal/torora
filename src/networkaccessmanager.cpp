/*
 * Copyright 2008 Benjamin C. Meyer <ben@meyerhome.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/****************************************************************************
**
** Copyright (C) 2007-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "networkaccessmanager.h"

#include "acceptlanguagedialog.h"
#include "browserapplication.h"
#include "browsermainwindow.h"
#include "ui_passworddialog.h"
#include "ui_proxy.h"

#include <qdialog.h>
#include <qmessagebox.h>
#include <qsettings.h>
#include <qstyle.h>
#include <qtextdocument.h>

#include <qauthenticator.h>
#include <qnetworkproxy.h>
#include <qnetworkreply.h>
#include <qsslconfiguration.h>
#include <qsslerror.h>
#include <qdatetime.h>

#if QT_VERSION >= 0x040500
#include <qnetworkdiskcache.h>
#include <qdesktopservices.h>
#endif

#if QT_VERSION >= 0x040500
NetworkProxyFactory::NetworkProxyFactory()
    : QNetworkProxyFactory()
{
}

void NetworkProxyFactory::setHttpProxy(const QNetworkProxy &proxy)
{
    m_httpProxy = proxy;
}

void NetworkProxyFactory::setGlobalProxy(const QNetworkProxy &proxy)
{
    m_globalProxy = proxy;
}

QList<QNetworkProxy> NetworkProxyFactory::queryProxy(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> ret;

    if (query.protocolTag() == QLatin1String("http") && m_httpProxy.type() != QNetworkProxy::DefaultProxy)
        ret << m_httpProxy;
    ret << m_globalProxy;

    return ret;
}
#endif

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
    connect(this, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
            SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));
    connect(this, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));
#ifndef QT_NO_OPENSSL
    connect(this, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
            SLOT(sslErrors(QNetworkReply*, const QList<QSslError>&)));
#endif
    loadSettings();
}

QNetworkProxy NetworkAccessManager::currentProxy()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("proxy"));
    QNetworkProxy proxy;
    if (BrowserApplication::instance()->isTor()) {
        proxy = QNetworkProxy::HttpProxy;
        proxy.setHostName(QLatin1String("127.0.0.1"));
        proxy.setPort(settings.value(QLatin1String("port"), 8118).toInt());
        proxy.setUser(settings.value(QLatin1String("userName")).toString());
        proxy.setPassword(settings.value(QLatin1String("password")).toString());
    }else if (settings.value(QLatin1String("enabled"), false).toBool()) {
        int proxyType = settings.value(QLatin1String("type"), 0).toInt();
        if (proxyType == 0)
            proxy = QNetworkProxy::Socks5Proxy;
        else if (proxyType == 1)
            proxy = QNetworkProxy::HttpProxy;
        else { // 2
            proxy.setType(QNetworkProxy::HttpCachingProxy);
        }
        proxy.setHostName(settings.value(QLatin1String("hostName")).toString());
        proxy.setPort(settings.value(QLatin1String("port"), 1080).toInt());
        proxy.setUser(settings.value(QLatin1String("userName")).toString());
        proxy.setPassword(settings.value(QLatin1String("password")).toString());
    }
    settings.endGroup();
    return proxy;
}

void NetworkAccessManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("proxy"));
    QNetworkProxy proxy;

    /*Torora: Req 2.2*/
    if (BrowserApplication::instance()->isTor()) {
        proxy = QNetworkProxy::HttpProxy;
#if QT_VERSION < 0x040500
        // Allows http browsing but not https browsing. Qt 4.5 is the only one
        // that works for both.
        proxy.setType(QNetworkProxy::HttpCachingProxy);
#endif
        proxy.setHostName(QLatin1String("127.0.0.1"));
        proxy.setPort(settings.value(QLatin1String("port"), 8118).toInt());
        proxy.setUser(settings.value(QLatin1String("userName")).toString());
        proxy.setPassword(settings.value(QLatin1String("password")).toString());
    } else if (settings.value(QLatin1String("enabled"), false).toBool()) {
        int proxyType = settings.value(QLatin1String("type"), 0).toInt();
        if (proxyType == 0)
            proxy = QNetworkProxy::Socks5Proxy;
        else if (proxyType == 1)
            proxy = QNetworkProxy::HttpProxy;
        else { // 2
            proxy.setType(QNetworkProxy::HttpCachingProxy);
#if QT_VERSION >= 0x040500
            proxy.setCapabilities(QNetworkProxy::CachingCapability | QNetworkProxy::HostNameLookupCapability);
#endif
        }
        proxy.setHostName(settings.value(QLatin1String("hostName")).toString());
        proxy.setPort(settings.value(QLatin1String("port"), 1080).toInt());
        proxy.setUser(settings.value(QLatin1String("userName")).toString());
        proxy.setPassword(settings.value(QLatin1String("password")).toString());
    }
#if QT_VERSION >= 0x040500
    NetworkProxyFactory *proxyFactory = new NetworkProxyFactory;
    if (proxy.type() == QNetworkProxy::HttpCachingProxy) {
      proxyFactory->setHttpProxy(proxy);
      proxyFactory->setGlobalProxy(QNetworkProxy::DefaultProxy);
    } else {
      proxyFactory->setHttpProxy(QNetworkProxy::DefaultProxy);
      proxyFactory->setGlobalProxy(proxy);
    }
    setProxyFactory(proxyFactory);
#else
    setProxy(proxy);
#endif
    settings.endGroup();

#ifndef QT_NO_OPENSSL
    QSslConfiguration sslCfg = QSslConfiguration::defaultConfiguration();
    QList<QSslCertificate> ca_list = sslCfg.caCertificates();
    QList<QSslCertificate> ca_new = QSslCertificate::fromData(settings.value(QLatin1String("CaCertificates")).toByteArray());
    ca_list += ca_new;

    sslCfg.setCaCertificates(ca_list);
    QSslConfiguration::setDefaultConfiguration(sslCfg);
#endif

    settings.beginGroup(QLatin1String("network"));
    QStringList acceptList = settings.value(QLatin1String("acceptLanguages"),
            AcceptLanguageDialog::defaultAcceptList()).toStringList();
    m_acceptLanguage = AcceptLanguageDialog::httpString(acceptList);

#if QT_VERSION >= 0x040500
    bool m_cacheEnabled = settings.value(QLatin1String("cacheEnabled"), true).toBool();
    if (m_cacheEnabled) {
        QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
        QString location = QDesktopServices::storageLocation(QDesktopServices::CacheLocation)
                                + QLatin1String("/browser");
        diskCache->setCacheDirectory(location);
        setCache(diskCache);
    }
#endif
    settings.endGroup();
}

void NetworkAccessManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *auth)
{
    BrowserMainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    Ui::PasswordDialog passwordDialog;
    passwordDialog.setupUi(&dialog);

    passwordDialog.iconLabel->setText(QString());
    passwordDialog.iconLabel->setPixmap(mainWindow->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, mainWindow).pixmap(32, 32));

    QString introMessage = tr("<qt>Enter username and password for \"%1\" at %2</qt>");
    introMessage = introMessage.arg(Qt::escape(auth->realm())).arg(Qt::escape(reply->url().toString()));
    passwordDialog.introLabel->setText(introMessage);
    passwordDialog.introLabel->setWordWrap(true);

    if (dialog.exec() == QDialog::Accepted) {
        auth->setUser(passwordDialog.userNameLineEdit->text());
        auth->setPassword(passwordDialog.passwordLineEdit->text());
    }
}

void NetworkAccessManager::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth)
{
    BrowserMainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    Ui::ProxyDialog proxyDialog;
    proxyDialog.setupUi(&dialog);

    proxyDialog.iconLabel->setText(QString());
    proxyDialog.iconLabel->setPixmap(mainWindow->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, mainWindow).pixmap(32, 32));

    QString introMessage = tr("<qt>Connect to proxy \"%1\" using:</qt>");
    introMessage = introMessage.arg(Qt::escape(proxy.hostName()));
    proxyDialog.introLabel->setText(introMessage);
    proxyDialog.introLabel->setWordWrap(true);

    if (dialog.exec() == QDialog::Accepted) {
        auth->setUser(proxyDialog.userNameLineEdit->text());
        auth->setPassword(proxyDialog.passwordLineEdit->text());
    }
}

#ifndef QT_NO_OPENSSL
static QString certToFormattedString(QSslCertificate cert)
{
    QString resultstring = QLatin1String("<p>");
    QStringList tmplist;

    resultstring += cert.subjectInfo(QSslCertificate::CommonName);

    resultstring += QString::fromLatin1("<br/>Issuer: %1")
        .arg(cert.issuerInfo(QSslCertificate::CommonName));

    resultstring += QString::fromLatin1("<br/>Not valid before: %1<br/>Valid Until: %2")
        .arg(cert.effectiveDate().toString(Qt::ISODate))
        .arg(cert.expiryDate().toString(Qt::ISODate));

    QMultiMap<QSsl::AlternateNameEntryType, QString> names = cert.alternateSubjectNames();
    if (names.count() > 0) {
        tmplist = names.values(QSsl::DnsEntry);
        resultstring += QLatin1String("<br/>Alternate Names:<ul><li>")
            + tmplist.join(QLatin1String("</li><li>"))
            + QLatin1String("</li><</ul>");
    }

    resultstring += QLatin1String("</p>");

    return resultstring;
}

void NetworkAccessManager::sslErrors(QNetworkReply *reply, const QList<QSslError> &error)
{
    BrowserMainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QSettings settings;
    QList<QSslCertificate> ca_merge = QSslCertificate::fromData(settings.value(QLatin1String("CaCertificates")).toByteArray());

    QList<QSslCertificate> ca_new;
    QStringList errorStrings;
    for (int i = 0; i < error.count(); ++i) {
        if (ca_merge.contains(error.at(i).certificate()))
            continue;
        errorStrings += error.at(i).errorString();
        if (!error.at(i).certificate().isNull()) {
            ca_new.append(error.at(i).certificate());
        }
    }
    if (errorStrings.isEmpty()) {
        reply->ignoreSslErrors();
        return;
    }

    QString errors = errorStrings.join(QLatin1String("</li><li>"));
    int ret = QMessageBox::warning(mainWindow,
                           QCoreApplication::applicationName() + tr(" - SSL Errors"),
                           tr("<qt>SSL Errors:"
                              "<br/><br/>for: <tt>%1</tt>"
                              "<ul><li>%2</li></ul>\n\n"
                              "Do you want to ignore these errors?</qt>").arg(reply->url().toString()).arg(errors),
                           QMessageBox::Yes | QMessageBox::No,
                           QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (ca_new.count() > 0) {
            QStringList certinfos;
            for (int i = 0; i < ca_new.count(); ++i)
                certinfos += certToFormattedString(ca_new.at(i));
            ret = QMessageBox::question(mainWindow, QCoreApplication::applicationName(),
                tr("<qt>Certificates:<br/>"
                   "%1<br/>"
                   "Do you want to accept all these certificates?</qt>")
                    .arg(certinfos.join(QString())),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                ca_merge += ca_new;

                QSslConfiguration sslCfg = QSslConfiguration::defaultConfiguration();
                QList<QSslCertificate> ca_list = sslCfg.caCertificates();
                ca_list += ca_new;
                sslCfg.setCaCertificates(ca_list);
                QSslConfiguration::setDefaultConfiguration(sslCfg);
                reply->setSslConfiguration(sslCfg);

                QByteArray pems;
                for (int i = 0; i < ca_merge.count(); ++i)
                    pems += ca_merge.at(i).toPem() + '\n';
                settings.setValue(QLatin1String("CaCertificates"), pems);
            }
        }
        reply->ignoreSslErrors();
    }
}
#endif

bool NetworkAccessManager::requestViolatesTorRules(QNetworkRequest &req)
{
    /*Torora: Req 3.7*/
    if (req.hasRawHeader("Referer"))
        req.setRawHeader("Referer", "/");
    if (req.hasRawHeader("Origin"))
        req.setRawHeader("Origin", "/");

    /*Torora: Req 7.2*/
    if ((req.url().port() == 8118) ||
       (req.url().port() == 9050) ||
       (req.url().port() == 9051) ||
       (req.url().port() == 8180)) {
       QMessageBox::information(0, tr("Access to Port %1 blocked").arg(req.url().port()),
            tr("<p>Either you or a form in the web page you are viewing has attempted to "
               "access port %1. Torora blocks access to this port in order to prevent "
               "HTML form attacks on Tor. <br> See http://www.remote.org/jochen/sec/hfpa/hfpa.pdf "
               "for more information.</a></p>").arg(req.url().port()));
       return true;
    }

    /*Torora: Req 7.2*/
    if ((req.url().host() == QLatin1String("localhost")) ||
       (req.url().host() == QLatin1String("127.0.0.1"))) {
       QMessageBox::information(0, tr("Access to hostname %1 blocked").arg(req.url().host()),
            tr("Either you or a form in the web page you are viewing has attempted to "
               "access %1. Torora blocks access to %1 in order to prevent "
               "HTML form attacks on Tor. <br> See http://www.remote.org/jochen/sec/hfpa/hfpa.pdf "
               "for more information.</a></p>").arg(req.url().host()));
       return true;
    }
    
    /*Torora: Req 3.8*/
    if ((req.url().host().contains(QLatin1String(".exit")))) {
        int choice = QMessageBox::warning(0, tr("You are following a '.exit' URL: %1").arg(req.url().host()),
                                        tr("If you did not type this URL yourself, a remote website may be attempting to track you. \n"
                                           "Do you want to continue anyway?"),
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);
        if (choice == QMessageBox::No)
            return true;
    }
    return false;
}

QNetworkReply *NetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    QNetworkReply *reply;
    QNetworkRequest req = request;

    if (BrowserApplication::instance()->isTor()) {
        if (requestViolatesTorRules(req)) {
            /*FIXME: subclass qnetworkreply and abort?*/
            reply = QNetworkAccessManager::createRequest(op, req, outgoingData);
            reply->abort();
            return reply;
        }
    }

    if (!m_acceptLanguage.isEmpty()) {
        req.setRawHeader("Accept-Language", m_acceptLanguage);
        reply = QNetworkAccessManager::createRequest(op, req, outgoingData);
        emit requestCreated(op, req, reply);
    } else {
        reply = QNetworkAccessManager::createRequest(op, req, outgoingData);
        emit requestCreated(op, req, reply);
    }
    return reply;
}
