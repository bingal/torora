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

#ifndef JSObjects_h
#define JSObjects_h

#include <QBasicTimer>
#include <QObject>
#include "browserapplication.h"
#include "browsermainwindow.h"

class TororaScreenObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(unsigned availHeight READ availHeight)
    Q_PROPERTY(unsigned availWidth READ availWidth)
    Q_PROPERTY(unsigned height READ height)
    Q_PROPERTY(unsigned width READ width)
    Q_PROPERTY(unsigned pixelDepth READ pixelDepth)
    Q_PROPERTY(unsigned colorDepth READ colorDepth)
public:
    TororaScreenObject(QObject* parent = 0)
        : QObject(parent) { }
    unsigned availHeight() const {
        return BrowserApplication::instance()->mainWindow()->size().height();
    }
    unsigned availWidth() const {
        return BrowserApplication::instance()->mainWindow()->size().width();
    }
    unsigned height() const {
        return BrowserApplication::instance()->mainWindow()->size().height();
    }
    unsigned width() const {
        return BrowserApplication::instance()->mainWindow()->size().width();
    }
    unsigned pixelDepth() const {
        return 24;
    }
    unsigned colorDepth() const {
        return 24;
    }
};

class TororaNavigatorObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString appCodeName READ appCodeName)
    Q_PROPERTY(QString appName READ appName)
    Q_PROPERTY(QString appVersion READ appVersion)
    Q_PROPERTY(QString platform READ platform)
    Q_PROPERTY(QString product READ product)
    Q_PROPERTY(QString productSub READ productSub)
    Q_PROPERTY(QString vendor READ vendor)
    Q_PROPERTY(QString oscpu READ oscpu)
    Q_PROPERTY(QString useragent READ useragent)
    Q_PROPERTY(QString userAgent READ userAgent)
    Q_PROPERTY(QString useragent_vendorSub READ useragent_vendorSub)
    Q_PROPERTY(QString buildID READ buildID)
    Q_PROPERTY(QString language READ language)
    Q_PROPERTY(QString plugins READ plugins)
public:
    TororaNavigatorObject(QObject* parent = 0)
        : QObject(parent) { }
    QString appCodeName() const {
        return QLatin1String("Mozilla");
    }
    QString appName() const {
        return QLatin1String("Netscape");
    }
    QString appVersion() const {
        return QLatin1String("5.0 (Windows; LANG)");
    }
    QString platform() const {
        return QLatin1String("Win32");
    }
    QString product() const {
        return QLatin1String("Gecko");
    }
    QString productSub() const {
        return QLatin1String("20030107");
    }
    QString vendor() const {
        return QLatin1String("Apple Computer, Inc.");
    }
    QString oscpu() const {
        return QLatin1String("Windows NT 5.1");
    }
    QString useragent() const {
        return QLatin1String("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/528.16 (KHTML, like Gecko) Version/4.0 Safari/528.16");
    }
    QString userAgent() const {
        return QLatin1String("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/528.16 (KHTML, like Gecko) Version/4.0 Safari/528.16");
    }
    QString useragent_vendorSub() const {
        return QLatin1String("");
    }
    QString buildID() const {
        return QLatin1String("0");
    }
    QString language() const {
        return QLatin1String("en-US");
    }
    QString plugins() const {
        return QLatin1String("");
    }
    // Geolocation is not allowed or wanted!
    // Should this be uncommented? Or is it safer for navigator to be ignorant of the feature completely?
/*    QString geolocation() const {
        return QLatin1String("");
    }*/
};
#endif // JSObjects_h
