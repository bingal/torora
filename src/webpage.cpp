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
#include "downloadmanager.h"
#include "historymanager.h"
#include "networkaccessmanager.h"
#include "opensearchengine.h"
#include "opensearchmanager.h"
#include "tabwidget.h"
#include "toolbarsearch.h"
#include "tormanager.h"
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
    , m_tororaScreenObject(0)
    , m_tororaNavigatorObject(0)
{
    setPluginFactory(webPluginFactory());
    NetworkAccessManagerProxy *networkManagerProxy = new NetworkAccessManagerProxy(this);
    networkManagerProxy->setWebPage(this);
    networkManagerProxy->setPrimaryNetworkAccessManager(BrowserApplication::networkAccessManager());
    setNetworkAccessManager(networkManagerProxy);
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(handleUnsupportedContent(QNetworkReply *)));
    connect(this, SIGNAL(frameCreated(QWebFrame *)),
            this, SLOT(addExternalBinding(QWebFrame *)));
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

    if (!m_tororaScreenObject)
        m_tororaScreenObject = new TororaScreenObject(this);

    if (!m_tororaNavigatorObject)
        m_tororaNavigatorObject = new TororaNavigatorObject(this);

    if (frame == 0) { // called from QWebFrame::javaScriptWindowObjectCleared
        frame = qobject_cast<QWebFrame*>(sender());

        frame->addToJavaScriptWindowObject(QLatin1String("arora"), m_javaScriptAroraObject);
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

    //Reset privacy-sensitive JS Objects
    frame->addToJavaScriptWindowObject(QLatin1String("navigator"), m_tororaNavigatorObject);
    frame->addToJavaScriptWindowObject(QLatin1String("screen"), m_tororaScreenObject);
}

QString WebPage::userAgent()
{
    return s_userAgent;
}

void WebPage::setUserAgent(const QString &userAgent)
{
    if (userAgent == s_userAgent)
        return;

    QSettings settings;
    if (userAgent.isEmpty()) {
        settings.remove(QLatin1String("userAgent"));
    } else {
        settings.setValue(QLatin1String("userAgent"), userAgent);
    }

    s_userAgent = userAgent;
}

QString WebPage::userAgentForUrl(const QUrl &url) const
{
    Q_UNUSED(url);
    return QLatin1String("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/528.16 (KHTML, like Gecko) Version/4.0 Safari/528.16");

    /* Disabled for Torora */
/*    if (s_userAgent.isEmpty())
        s_userAgent = QWebPage::userAgentForUrl(url);
    return s_userAgent;*/
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
                                      NavigationType type)
{
    lastRequest = request;
    lastRequestType = type;

    enforceJSObjectPrivacy(frame, request);

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

void WebPage::enforceJSObjectPrivacy(QWebFrame *frame, const QNetworkRequest& request)
{
    // Enforce a same-origin policy on window.name
    // This still doesn't completely work yet, sometimes after navigating
    // to a new domain from the address bar the window.name cookie is still
    // present. It works well on click-throughs though.
    // http://code.google.com/p/torora/issues/detail?id=6
    if (!QUrl(frame->url().host()).isParentOf(request.url()))
        frame->evaluateJavaScript(QLatin1String("window.name=''"));
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
    Q_UNUSED(url);
    Q_UNUSED(paramNames);
    Q_UNUSED(paramValues);
    if (classId == QLatin1String("QPushButton")) {
        for (int i = 1; i < paramNames.count(); ++i) {
            if (paramValues[i] == QString(QLatin1String("RunServerButton"))) {
                QUiLoader loader;
                QObject *object;
                object = loader.createWidget(classId, view());
                connect (object, SIGNAL(clicked()), BrowserApplication::instance()->torManager(), SLOT(enableRelay()));
                return object;
            }
            if (paramValues[i] == QString(QLatin1String("DoneButton"))) {
                QUiLoader loader;
                QObject *object;
                object = loader.createWidget(classId, view());
                connect (object, SIGNAL(clicked()), BrowserApplication::instance()->torManager(), SLOT(authenticate()));
                return object;
            }
            if (paramValues[i] == QString(QLatin1String("TryAgainButton"))) {
                QUiLoader loader;
                QObject *object;
                object = loader.createWidget(classId, view());
                connect (object, SIGNAL(clicked()), BrowserApplication::instance()->torManager(), SLOT(checkTorExplicitly()));
                return object;
            }
        }
    }
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
