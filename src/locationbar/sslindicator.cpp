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

#include "sslindicator.h"
#include "browserapplication.h"
#include <qpainter.h>

SSLIndicator::SSLIndicator(QWidget *parent)
    : QAbstractButton(parent)
{
    m_sslIcons.insert(0, QPixmap(QLatin1String(":graphics/ssl/security-low.png")));
    m_sslIcons.insert(1, QPixmap(QLatin1String(":graphics/ssl/security-medium.png")));
    m_sslIcons.insert(2, QPixmap(QLatin1String(":graphics/ssl/security-high.png")));

    setCursor(Qt::ArrowCursor);
    setToolTip(tr("Security Information"));
    setFocusPolicy(Qt::NoFocus);
    setMinimumSize(16, 16);
    setIcon(QPixmap(QLatin1String(":graphics/ssl/security-high.png")));
    setVisible(false);
    
}

void SSLIndicator::displayPadlock(int security)
{
    m_security = security;
    setVisible(true);
}

void SSLIndicator::paintEvent(QPaintEvent *event)
{
   Q_UNUSED(event);
   QPainter painter(this);
   painter.drawPixmap(rect(),m_sslIcons[m_security]);
}
