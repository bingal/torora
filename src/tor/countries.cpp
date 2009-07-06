/****************************************************************************
 ** $Id: torclient.h,v 1.76 2009/01/17 15:49:08 hoganrobert Exp $
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

#include "countries.h"
#include <QStringList>

Countries::Countries()
{
    QStringList ccs;
    QStringList names;

    countries.clear();

    ccs << QLatin1String("us") << QLatin1String("gb") << QLatin1String("de")
        << QLatin1String("cn") << QLatin1String("fr") << QLatin1String("se")
        << QLatin1String("");
    names << QLatin1String("United States") << QLatin1String("United Kingdom")
          << QLatin1String("Germany") << QLatin1String("China")
          << QLatin1String("France") << QLatin1String("Sweden")
          << QLatin1String("Anonymous");
    for ( int i = 0; i != ccs.count(); ++i ) {
        Country *country = new Country(ccs[i],names[i]);
        countries.append(country);
    }
    m_countrycodes = ccs;
}

Countries::~Countries()
{
}

Country::Country(const QString &cc, const QString &name )
{
    if (!cc.isEmpty())
        m_icon = QPixmap(QString(QLatin1String(":graphics/flags/%1.png")).arg(cc));
    else
        m_icon = QPixmap(QString(QLatin1String(":graphics/tor.png")));
    m_name = name;
    m_cc = cc;
}

Country::~Country()
{
}
