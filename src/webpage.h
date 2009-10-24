/*
 * Copyright 2009 Benjamin C. Meyer <ben@meyerhome.net>
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

#ifndef WEBPAGE_H
#define WEBPAGE_H

#include "webpageproxy.h"
#include "tabwidget.h"
#include "sslerror.h"

#include <qlist.h>
#include <qnetworkrequest.h>
#include <QSslConfiguration>

class WebPageLinkedResource
{
public:
    QString rel;
    QString type;
    QUrl href;
    QString title;
};

class AroraSSLError;
class OpenSearchEngine;
class QNetworkReply;
class WebPluginFactory;
// See https://developer.mozilla.org/en/adding_search_engines_from_web_pages
class JavaScriptExternalObject : public QObject
{
    Q_OBJECT

public:
    JavaScriptExternalObject(QObject *parent = 0);

public slots:
    void AddSearchProvider(const QString &url);
};

class JavaScriptAroraObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QObject *currentEngine READ currentEngine)

public:
    JavaScriptAroraObject(QObject *parent = 0);

public slots:
    QString translate(const QString &string);
    QObject *currentEngine() const;
    QString searchUrl(const QString &string) const;
};

class WebPage : public WebPageProxy
{
    Q_OBJECT

signals:
    void aboutToLoadUrl(const QUrl &url);

public slots:
    void clearAllSSLErrors();

public:
    WebPage(QObject *parent = 0);
    ~WebPage();

    void loadSettings();

    static WebPluginFactory *webPluginFactory();
    QList<WebPageLinkedResource> linkedResources(const QString &relation = QString());

    bool hasSSLErrors();
    bool hasSSLCerts();
    bool hasLowGradeEncryption() { return m_sslLowGradeEncryption; }
    void setSSLConfiguration( const QSslConfiguration &config) { m_sslConfiguration = config; }
    QSslConfiguration sslConfiguration(){ return m_sslConfiguration; }
    QList<AroraSSLCertificate*> sslCertificates() { return m_AroraSSLCertificates; }
    int sslSecurity();
    bool pageHasSSLErrors(QWebFrame *frame);
    bool pageHasSSLCerts(QWebFrame *frame);
    bool frameIsPolluted(QWebFrame *frame, AroraSSLCertificate *cert);
    bool certHasPollutedFrame(AroraSSLCertificate *cert);
    bool containsFrame(QWebFrame *frame);

protected:
    QString userAgentForUrl(const QUrl &url) const;
    bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
                                 NavigationType type);
    QObject *createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues);
    QWebPage *createWindow(QWebPage::WebWindowType type);

    bool isNewWebsite(QWebFrame *frame, QUrl url);
    void clearSSLErrors(QWebFrame *frame);
    bool alreadyHasSSLCertForUrl(const QUrl url, QNetworkReply *reply, AroraSSLError *sslError=0L);
    void markPollutedFrame(QNetworkReply *reply);

protected slots:
    void handleUnsupportedContent(QNetworkReply *reply);
    void addExternalBinding(QWebFrame *frame = 0);

    void bindRequestToFrame(QWebFrame *frame, QNetworkRequest *request);
    void setSSLError(AroraSSLError *error, QNetworkReply *reply);
    void handleSSLErrorPage(AroraSSLError *error, QNetworkReply *reply);
    void setSSLConfiguration(QNetworkReply *reply);

protected:
    void populateNetworkRequest(QNetworkRequest &request);
    static QString s_userAgent;
    static WebPluginFactory *s_webPluginFactory;
    TabWidget::OpenUrlIn m_openTargetBlankLinksIn;
    QUrl m_requestedUrl;
    JavaScriptExternalObject *m_javaScriptExternalObject;
    JavaScriptAroraObject *m_javaScriptAroraObject;
    QWebFrame* getFrame(const QNetworkRequest& request);

private:
    QNetworkRequest lastRequest;
    QWebPage::NavigationType lastRequestType;

    bool m_sslLowGradeEncryption;
    QSslConfiguration m_sslConfiguration;
    typedef QList<AroraSSLCertificate*> AroraSSLCertificates;
    AroraSSLCertificates m_AroraSSLCertificates;
    QList<QWebFrame*> m_pollutedFrames;

};

#endif // WEBPAGE_H

