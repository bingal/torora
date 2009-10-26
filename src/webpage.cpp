/*
 * Copyright 2009 Benjamin C. Meyer <ben@meyerhome.net>
 * Copyright 2009 Jakub Wieczorek <faw217@gmail.com>
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

#include "webpage.h"

#include "browserapplication.h"
#include "cookiejar.h"
#include "downloadmanager.h"
#include "historymanager.h"
#include "networkaccessmanager.h"
#include "opensearchengine.h"
#include "opensearchmanager.h"
#include "sslerror.h"
#include "sslstrings.h"
#include "tabwidget.h"
#include "toolbarsearch.h"
#include "webpluginfactory.h"
#include "webview.h"

#include <qbuffer.h>
#include <qdesktopservices.h>
#include <qmessagebox.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qsettings.h>
#include <qwebframe.h>

#include <quiloader.h>

#if QT_VERSION >= 0x040600 || defined(WEBKIT_TRUNK)
#include <qwebelement.h>
#else
#define QT_NO_UITOOLS
#endif

WebPluginFactory *WebPage::s_webPluginFactory = 0;
QString WebPage::s_userAgent;

JavaScriptExternalObject::JavaScriptExternalObject(QObject *parent)
    : QObject(parent)
{
}

void JavaScriptExternalObject::AddSearchProvider(const QString &url)
{
    ToolbarSearch::openSearchManager()->addEngine(QUrl(url));
}

Q_DECLARE_METATYPE(OpenSearchEngine*)
Q_DECLARE_METATYPE(QWebFrame*)
JavaScriptAroraObject::JavaScriptAroraObject(QObject *parent)
    : QObject(parent)
{
    static const char *translations[] = {
        QT_TR_NOOP("Welcome to Arora!"),
        QT_TR_NOOP("Arora Start"),
        QT_TR_NOOP("Search!"),
        QT_TR_NOOP("Search results provided by"),
        QT_TR_NOOP("About Arora")
    };
    Q_UNUSED(translations);

    qRegisterMetaType<OpenSearchEngine*>("OpenSearchEngine*");
}

QString JavaScriptAroraObject::translate(const QString &string)
{
    QString translatedString = trUtf8(string.toUtf8().constData());

    // If the translation is the same as the original string
    // it could not be translated.  In that case
    // try to translate using the QApplication domain
    if (translatedString != string)
        return translatedString;
    else
        return qApp->trUtf8(string.toUtf8().constData());
}

QObject *JavaScriptAroraObject::currentEngine() const
{
    return ToolbarSearch::openSearchManager()->currentEngine();
}

QString JavaScriptAroraObject::searchUrl(const QString &string) const
{
    return QString::fromUtf8(ToolbarSearch::openSearchManager()->currentEngine()->searchUrl(string).toEncoded());
}

WebPage::WebPage(QObject *parent)
    : WebPageProxy(parent)
    , m_openTargetBlankLinksIn(TabWidget::NewWindow)
    , m_javaScriptExternalObject(0)
    , m_javaScriptAroraObject(0)
{
    setPluginFactory(webPluginFactory());
    NetworkAccessManagerProxy *networkManagerProxy = new NetworkAccessManagerProxy(this);
    networkManagerProxy->setWebPage(this);
    networkManagerProxy->setPrimaryNetworkAccessManager(BrowserApplication::networkAccessManager());
    setNetworkAccessManager(networkManagerProxy);
    networkManagerProxy->setCookieJar(BrowserApplication::instance()->cookieJar());
    networkManagerProxy->cookieJar()->setParent(0); // Required for CookieJars shared by networkaccessmanagers
    connect(networkManagerProxy, SIGNAL(setSSLError(AroraSSLError *, QNetworkReply *)),
            this, SLOT(setSSLError(AroraSSLError *, QNetworkReply *)));
    connect(networkManagerProxy, SIGNAL(sslErrorPage(AroraSSLError *, QNetworkReply*)),
            this, SLOT(handleSSLErrorPage(AroraSSLError *, QNetworkReply*)));
    connect(networkManagerProxy, SIGNAL(finished(QNetworkReply*)),
            SLOT(setSSLConfiguration(QNetworkReply*)));
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(handleUnsupportedContent(QNetworkReply *)));
    connect(this, SIGNAL(frameCreated(QWebFrame *)),
            this, SLOT(addExternalBinding(QWebFrame *)));
    connect(this, SIGNAL(networkRequestStarted(QWebFrame *, QNetworkRequest *)),
            this, SLOT(bindRequestToFrame(QWebFrame *, QNetworkRequest *)));
/*    connect(view(), SIGNAL(urlChanged(const QUrl &)),
            this, SLOT(clearAllSSLErrors()));*/
    addExternalBinding(mainFrame());
    loadSettings();
}

WebPage::~WebPage()
{
    setNetworkAccessManager(0);
}

WebPluginFactory *WebPage::webPluginFactory()
{
    if (!s_webPluginFactory)
        s_webPluginFactory = new WebPluginFactory(BrowserApplication::instance());
    return s_webPluginFactory;
}

QList<WebPageLinkedResource> WebPage::linkedResources(const QString &relation)
{
    QList<WebPageLinkedResource> resources;

#if QT_VERSION >= 0x040600 || defined(WEBKIT_TRUNK)
    QUrl baseUrl = mainFrame()->baseUrl();

    QWebElementCollection linkElements = mainFrame()->findAllElements(QLatin1String("html > head > link"));

    foreach (const QWebElement &linkElement, linkElements) {
        QString rel = linkElement.attribute(QLatin1String("rel"));
        QString href = linkElement.attribute(QLatin1String("href"));
        QString type = linkElement.attribute(QLatin1String("type"));
        QString title = linkElement.attribute(QLatin1String("title"));

        if (href.isEmpty() || type.isEmpty())
            continue;
        if (!relation.isEmpty() && rel != relation)
            continue;

        WebPageLinkedResource resource;
        resource.rel = rel;
        resource.type = type;
        resource.href = baseUrl.resolved(QUrl::fromEncoded(href.toUtf8()));
        resource.title = title;

        resources.append(resource);
    }
#else
    QString baseUrlString = mainFrame()->evaluateJavaScript(QLatin1String("document.baseURI")).toString();
    QUrl baseUrl = QUrl::fromEncoded(baseUrlString.toUtf8());

    QFile file(QLatin1String(":fetchLinks.js"));
    if (!file.open(QFile::ReadOnly))
        return resources;
    QString script = QString::fromUtf8(file.readAll());

    QVariantList list = mainFrame()->evaluateJavaScript(script).toList();
    foreach (const QVariant &variant, list) {
        QVariantMap map = variant.toMap();
        QString rel = map[QLatin1String("rel")].toString();
        QString type = map[QLatin1String("type")].toString();
        QString href = map[QLatin1String("href")].toString();
        QString title = map[QLatin1String("title")].toString();

        if (href.isEmpty() || type.isEmpty())
            continue;
        if (!relation.isEmpty() && rel != relation)
            continue;

        WebPageLinkedResource resource;
        resource.rel = rel;
        resource.type = type;
        resource.href = baseUrl.resolved(QUrl::fromEncoded(href.toUtf8()));
        resource.title = title;

        resources.append(resource);
    }
#endif

    return resources;
}

void WebPage::populateNetworkRequest(QNetworkRequest &request)
{
    if (request == lastRequest) {
        request.setAttribute((QNetworkRequest::Attribute)(pageAttributeId() + 1), lastRequestType);
    }
    WebPageProxy::populateNetworkRequest(request);
}

void WebPage::addExternalBinding(QWebFrame *frame)
{
#if QT_VERSION < 0x040600
    QWebSettings *defaultSettings = QWebSettings::globalSettings();
    if (!defaultSettings->testAttribute(QWebSettings::JavascriptEnabled))
        return;
#endif
    if (!m_javaScriptExternalObject)
        m_javaScriptExternalObject = new JavaScriptExternalObject(this);

    if (frame == 0) { // called from QWebFrame::javaScriptWindowObjectCleared
        frame = qobject_cast<QWebFrame*>(sender());

        if (frame->url().scheme() == QLatin1String("qrc")
            && frame->url().path() == QLatin1String("/startpage.html")) {

            if (!m_javaScriptAroraObject)
                m_javaScriptAroraObject = new JavaScriptAroraObject(this);

            frame->addToJavaScriptWindowObject(QLatin1String("arora"), m_javaScriptAroraObject);
        }
    } else { // called from QWebPage::frameCreated
        connect(frame, SIGNAL(javaScriptWindowObjectCleared()),
                this, SLOT(addExternalBinding()));
    }
    frame->addToJavaScriptWindowObject(QLatin1String("external"), m_javaScriptExternalObject);
}

QString WebPage::userAgentForUrl(const QUrl &url) const
{
    if (s_userAgent.isEmpty())
        s_userAgent = QWebPage::userAgentForUrl(url);
    return s_userAgent;
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
                                      NavigationType type)
{
    lastRequest = request;
    lastRequestType = type;

    QString scheme = request.url().scheme();
    if (scheme == QLatin1String("mailto")
        || scheme == QLatin1String("ftp")) {
        BrowserApplication::instance()->askDesktopToOpenUrl(request.url());
        return false;
    }

    if (type == QWebPage::NavigationTypeFormResubmitted) {
        QMessageBox::StandardButton button = QMessageBox::warning(view(), tr("Resending POST request"),
                             tr("In order to display the site, the request along with all the data must be sent once again, "
                                "which may lead to some unexpected behaviour of the site e.g. the same action might be "
                                "performed once again. Do you want to continue anyway?"), QMessageBox::Yes | QMessageBox::No);
        if (button != QMessageBox::Yes)
            return false;
    }

    TabWidget::OpenUrlIn openIn = frame ? TabWidget::CurrentTab : TabWidget::NewWindow;
    openIn = TabWidget::modifyWithUserBehavior(openIn);

    // handle the case where we want to do something different then
    // what qwebpage would do
    if (openIn == TabWidget::NewSelectedTab
        || openIn == TabWidget::NewNotSelectedTab
        || (frame && openIn == TabWidget::NewWindow)) {
        if (WebView *webView = qobject_cast<WebView*>(view())) {
            TabWidget *tabWidget = webView->tabWidget();
            if (tabWidget) {
                WebView *newView = tabWidget->getView(openIn, webView);
                QWebPage *page = 0;
                if (newView)
                    page = newView->page();
                if (page && page->mainFrame())
                    page->mainFrame()->load(request);
            }
        }
        return false;
    }

    bool accepted = QWebPage::acceptNavigationRequest(frame, request, type);
    if (accepted && frame == mainFrame()) {
        m_requestedUrl = request.url();
        emit aboutToLoadUrl(request.url());
    }

    return accepted;
}

void WebPage::loadSettings()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("tabs"));
    m_openTargetBlankLinksIn = (TabWidget::OpenUrlIn)settings.value(QLatin1String("openTargetBlankLinksIn"),
                                                                    TabWidget::NewSelectedTab).toInt();
    settings.endGroup();
    s_userAgent = settings.value(QLatin1String("userAgent")).toString();
}

QWebPage *WebPage::createWindow(QWebPage::WebWindowType type)
{
    Q_UNUSED(type);
    if (WebView *webView = qobject_cast<WebView*>(view())) {
        TabWidget *tabWidget = webView->tabWidget();
        if (tabWidget) {
            TabWidget::OpenUrlIn openIn = m_openTargetBlankLinksIn;
            openIn = TabWidget::modifyWithUserBehavior(openIn);
            return tabWidget->getView(openIn, webView)->page();
        }
    }
    return 0;
}

QObject *WebPage::createPlugin(const QString &classId, const QUrl &url,
                               const QStringList &paramNames, const QStringList &paramValues)
{
    Q_UNUSED(classId);
    Q_UNUSED(url);
    Q_UNUSED(paramNames);
    Q_UNUSED(paramValues);
#if !defined(QT_NO_UITOOLS)
    if (classId == QLatin1String("QPushButton")) {
        for (int i = 0; i < paramNames.count(); ++i) {
            if (paramValues[i].contains(QLatin1String("SSLProceedButton")) ||
                paramValues[i].contains(QLatin1String("SSLCancelButton"))) {
              QUiLoader loader;
              QObject *object;
              object = loader.createWidget(classId, view());
              QListIterator<AroraSSLCertificate*> certs(allCerts());
              while (certs.hasNext()) {
                  AroraSSLCertificate *cert = certs.next();
                  if (!cert->hasError())
                      continue;
                  QListIterator<AroraSSLError*> errors(cert->errors());
                  while (errors.hasNext()) {
                      AroraSSLError *error = errors.next();
                      if (paramValues[i] == QString(QLatin1String("SSLProceedButton%1")).arg(error->errorid())) {
                          connect (object, SIGNAL(clicked()), error, SLOT(loadFrame()));
                          return object;
                      }
                      if (paramValues[i] == QString(QLatin1String("SSLCancelButton%1")).arg(error->errorid())) {
                          connect (object, SIGNAL(clicked()), networkAccessManager(), SLOT(sslCancel()));
                          return object;
                      }
                  }
              }
            }
        }
    }
#endif
    return 0;
}

// The chromium guys have documented many examples of incompatibilities that
// different browsers have when they mime sniff.
// http://src.chromium.org/viewvc/chrome/trunk/src/net/base/mime_sniffer.cc
//
// All WebKit ports should share a common set of rules to sniff content.
// By having this here we are yet another browser that has different behavior :(
// But sadly QtWebKit does no sniffing at all so we are forced to do something.
static bool contentSniff(const QByteArray &data)
{
    if (data.contains("<!doctype")
        || data.contains("<script")
        || data.contains("<html")
        || data.contains("<!--")
        || data.contains("<head")
        || data.contains("<iframe")
        || data.contains("<h1")
        || data.contains("<div")
        || data.contains("<font")
        || data.contains("<table")
        || data.contains("<a")
        || data.contains("<style")
        || data.contains("<title")
        || data.contains("<b")
        || data.contains("<body")
        || data.contains("<br")
        || data.contains("<p"))
        return true;
    return false;
}

void WebPage::handleUnsupportedContent(QNetworkReply *reply)
{
    if (!reply)
        return;

    QUrl replyUrl = reply->url();

    if (replyUrl.scheme() == QLatin1String("abp"))
        return;

    switch (reply->error()) {
    case QNetworkReply::NoError:
        if (reply->header(QNetworkRequest::ContentTypeHeader).isValid()) {
            BrowserApplication::downloadManager()->handleUnsupportedContent(reply);
            return;
        }
        break;
    case QNetworkReply::ProtocolUnknownError: {
        QSettings settings;
        settings.beginGroup(QLatin1String("WebView"));
        QStringList externalSchemes = settings.value(QLatin1String("externalSchemes")).toStringList();
        if (externalSchemes.contains(replyUrl.scheme())) {
            QDesktopServices::openUrl(replyUrl);
            return;
        }
        break;
    }
    default:
        break;
    }

    // Find the frame that has the unsupported content
    if (replyUrl.isEmpty() || replyUrl != m_requestedUrl)
        return;

    QWebFrame *notFoundFrame = mainFrame();
    if (!notFoundFrame)
        return;

    if (reply->header(QNetworkRequest::ContentTypeHeader).toString().isEmpty()) {
        // do evil
        QByteArray data = reply->readAll();
        if (contentSniff(data)) {
            notFoundFrame->setHtml(QLatin1String(data), replyUrl);
            return;
        }
    }

    // Generate translated not found error page with an image
    QFile notFoundErrorFile(QLatin1String(":/notfound.html"));
    if (!notFoundErrorFile.open(QIODevice::ReadOnly))
        return;
    QString title = tr("Error loading page: %1").arg(QString::fromUtf8(replyUrl.toEncoded()));
    QString html = QLatin1String(notFoundErrorFile.readAll());
    QPixmap pixmap = qApp->style()->standardIcon(QStyle::SP_MessageBoxWarning, 0, view()).pixmap(QSize(32, 32));
    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("IMAGE_BINARY_DATA_HERE"),
                     QLatin1String(imageBuffer.buffer().toBase64()));
    }
    html = html.arg(title,
                    reply->errorString(),
                    tr("When connecting to: %1.").arg(QString::fromUtf8(replyUrl.toEncoded())),
                    tr("Check the address for errors such as <b>ww</b>.arora-browser.org instead of <b>www</b>.arora-browser.org"),
                    tr("If the address is correct, try checking the network connection."),
                    tr("If your computer or network is protected by a firewall or proxy, make sure that the browser is permitted to access the network."));
    notFoundFrame->setHtml(html, replyUrl);
    // Don't put error pages to the history.
    BrowserApplication::instance()->historyManager()->removeHistoryEntry(replyUrl, notFoundFrame->title());
}

#ifndef QT_NO_OPENSSL
void WebPage::setSSLError(AroraSSLError *sslError, QNetworkReply *reply)
{
    if (alreadyHasSSLCertForUrl(sslError->url(), reply, sslError))
        return;

    AroraSSLCertificate *certificate = new AroraSSLCertificate(sslError, reply->sslConfiguration(), sslError->url());

    QVariant var;
    QWebFrame *frame;
    frame = getFrame(reply->request());
    sslError->setFrame(frame);
    certificate->addFrame(frame);
    addCertToFrame(certificate, frame);
}

void WebPage::addCertToFrame(AroraSSLCertificate *certificate, QWebFrame *frame)
{
    AroraSSLCertificates certs;
    if (m_frameSSLCertificates.contains(frame)) {
        certs.append(m_frameSSLCertificates.value(frame));
        m_frameSSLCertificates.remove(frame);
    }
    certs.append(certificate);
    m_frameSSLCertificates.insert(frame, certs);
}

QList<AroraSSLCertificate*> WebPage::allCerts()
{
    AroraSSLCertificates certs;
    QMapIterator<QWebFrame*, AroraSSLCertificates> i(m_frameSSLCertificates);
    while (i.hasNext()) {
        i.next();
        certs.append(i.value());
    }
    return certs;
}

bool WebPage::alreadyHasSSLCertForUrl(const QUrl url, QNetworkReply *reply, AroraSSLError *sslError)
{

    AroraSSLCertificates certs = allCerts();

    for (int i = 0; i < certs.count(); ++i) {
        AroraSSLCertificate *cert = certs.at(i);
        if (cert->url().host() == url.host()) {
            QVariant var;
            QWebFrame *replyframe = getFrame(reply->request());
            cert->addFrame(replyframe);
            if (sslError) {
                sslError->setFrame(replyframe);
                cert->addError(sslError);
            }
            return true;
        }
    }
    return false;
}

void WebPage::markPollutedFrame(QNetworkReply *reply)
{
    AroraSSLCertificates certs = allCerts();
    QWebFrame *replyframe = getFrame(reply->request());
    for (int i = 0; i < certs.count(); ++i) {
        AroraSSLCertificate *cert = certs.at(i);
        if (cert->frames().contains(replyframe) &&
            cert->url().host() == replyframe->url().host()) {
            m_pollutedFrames.append(replyframe);
            return;
        }
    }
}

bool WebPage::frameIsPolluted(QWebFrame *frame, AroraSSLCertificate *cert)
{
    if (!containsFrame(frame))
        return false;
    if (m_pollutedFrames.contains(frame) &&
        cert->url().host() == frame->url().host())
        return true;
    return false;
}

bool WebPage::certHasPollutedFrame(AroraSSLCertificate *cert)
{
    QListIterator<QWebFrame*> frames(cert->frames());
    while (frames.hasNext()) {
        QWebFrame *frame = frames.next();
        if (frameIsPolluted(frame, cert))
            return true;
    }
    return false;
}

void WebPage::handleSSLErrorPage(AroraSSLError *error, QNetworkReply* reply)
{
    QString html = sslErrorHtml(error);
    QWebFrame *replyframe = getFrame(reply->request());
    disconnect(view(), SIGNAL(urlChanged(const QUrl &)),
               this, SLOT(clearAllSSLErrors()));
    replyframe->setHtml(html, error->url());
    connect(view(), SIGNAL(urlChanged(const QUrl &)),
               this, SLOT(clearAllSSLErrors()));
}

void WebPage::setSSLConfiguration(QNetworkReply *reply)
{
    if (reply->sslConfiguration().sessionCipher().isNull()) {
        markPollutedFrame(reply);
        return;
    }

    if (alreadyHasSSLCertForUrl(reply->url(), reply))
        return;

    AroraSSLCertificate *certificate = new AroraSSLCertificate(0L, reply->sslConfiguration(), reply->url());
    QSslCipher sessionCipher = reply->sslConfiguration().sessionCipher();
    bool lowGrade = lowGradeEncryption(sessionCipher);
    m_sslLowGradeEncryption = lowGrade;
    certificate->setLowGradeEncryption(lowGrade);

    /* Associate the cert with it's frame */
    /* FIXME: Remove the duplication of code with sslerror */
    QWebFrame *certFrame = 0;
    QList<QWebFrame*> frames;
    frames.append(mainFrame());
    while (!frames.isEmpty()) {
        QWebFrame *frame = frames.takeFirst();
        if (getFrame(reply->request()) == frame) {
            certFrame = frame;
            break;
        }
        frames += frame->childFrames();
    }

    if (certFrame) {
        certificate->addFrame(certFrame);
        addCertToFrame(certificate, certFrame);
    }
}

int WebPage::sslSecurity()
{
    if (hasSSLErrors())
        return SECURITY_LOW;
    if (m_sslLowGradeEncryption || !m_pollutedFrames.isEmpty())
        return SECURITY_MEDIUM;
    return SECURITY_HIGH;
}

bool WebPage::hasSSLErrors()
{
    AroraSSLCertificates certs = allCerts();
    for (int i = 0; i < certs.count(); ++i) {
        AroraSSLCertificate *cert = certs.at(i);
        if (cert->hasError())
            return true;
    }
    return false;
}

bool WebPage::hasSSLCerts()
{
    if (allCerts().count() > 0)
        return true;
    return false;
}

bool WebPage::frameHasSSLErrors(QWebFrame *frame)
{
    AroraSSLCertificates certs = m_frameSSLCertificates.value(frame);
    for (int i = 0; i < certs.count(); ++i) {
        AroraSSLCertificate *cert = certs.at(i);
        if (cert->hasError())
            return true;
    }
    return false;
}

bool WebPage::frameHasSSLCerts(QWebFrame *frame)
{
    if (m_frameSSLCertificates.contains(frame));
        return true;
    return false;
}

void WebPage::clearAllSSLErrors()
{
    /*FIXME: we call this on urlChanged(). This works fine in all cases
             except when using 'Open Frame' from the context menu in a
             framed page. */
    m_frameSSLCertificates.clear();
}

void WebPage::bindRequestToFrame(QWebFrame *frame, QNetworkRequest *request)
{
    if (!frame)
        return;

    QVariant var;
    var.setValue(frame);
    request->setAttribute((QNetworkRequest::Attribute)(QNetworkRequest::User + 200), var);
}

bool WebPage::containsFrame(QWebFrame *frame)
{
    QList<QWebFrame*> frames;
    frames.append(mainFrame());
    while (!frames.isEmpty()) {
        QWebFrame *f = frames.takeFirst();
        if (f == frame)
            return true;
        frames += f->childFrames();
    }
    return false;
}

QWebFrame* WebPage::getFrame(const QNetworkRequest& request)
{
    QVariant v;
    QWebFrame *frame;
    v = request.attribute((QNetworkRequest::Attribute)(QNetworkRequest::User + 200));
    frame = v.value<QWebFrame*>();
    if (containsFrame(frame))
        return frame;
    return 0L;
}

#endif
