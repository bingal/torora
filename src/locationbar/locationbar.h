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

#ifndef LOCATIONBAR_H
#define LOCATIONBAR_H

#include "lineedit.h"

#include <qpointer.h>
#include <qurl.h>

class WebView;
class LocationBarSiteIcon;
class PrivacyIndicator;
class LocationBar : public LineEdit
{
    Q_OBJECT

public:
    LocationBar(QWidget *parent = 0);
    void setWebView(WebView *webView);
<<<<<<< HEAD:src/locationbar.h

public slots:
    void setPrivate(bool isPrivate);
    void setTor(bool isTor);
=======
    WebView *webView() const;
>>>>>>> 6b1f3bd88908b220ae620c0131ed97bad5c315c8:src/locationbar/locationbar.h

protected:
    void paintEvent(QPaintEvent *event);
    void focusOutEvent(QFocusEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

private slots:
    void webViewUrlChanged(const QUrl &url);

private:
    QPointer<WebView> m_webView;
    QColor m_defaultBaseColor;
<<<<<<< HEAD:src/locationbar.h
    QLabel *m_privacyIndicator;
    QLabel *m_torIndicator;
=======

    LocationBarSiteIcon *m_siteIcon;
    PrivacyIndicator *m_privacyIndicator;
>>>>>>> 6b1f3bd88908b220ae620c0131ed97bad5c315c8:src/locationbar/locationbar.h
};

#endif // LOCATIONBAR_H

