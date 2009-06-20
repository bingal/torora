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

#include "tabwidget.h"

#include <qlist.h>
#include <qwebpage.h>

class WebPageLinkedResource
{
public:
    QString rel;
    QString type;
    QString href;
    QString title;
};

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

class WebPage : public QWebPage
{
    Q_OBJECT

signals:
    void aboutToLoadUrl(const QUrl &url);

public:
    WebPage(QObject *parent = 0);
    void loadSettings();

    static WebPluginFactory *webPluginFactory();
    QList<WebPageLinkedResource> linkedResources(const QString &relation = QString());

protected:
    bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
                                 NavigationType type);
    QObject *createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues);
    QWebPage *createWindow(QWebPage::WebWindowType type);
    QString userAgentForUrl(const QUrl& url) const;

protected slots:
    void handleUnsupportedContent(QNetworkReply *reply);
    void addExternalBinding(QWebFrame *frame = 0);

protected:
    static WebPluginFactory *s_webPluginFactory;
    TabWidget::OpenUrlIn m_openTargetBlankLinksIn;
    QUrl m_requestedUrl;
    JavaScriptExternalObject *m_javaScriptBinding;
};

#endif // WEBPAGE_H

