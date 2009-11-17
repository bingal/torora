/*
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

#include "notificationsbar.h"

#include <qevent.h>
#include <qtimeline.h>
#include <qshortcut.h>
#include <qtimer.h>

#include <qdebug.h>

NotificationsBar::NotificationsBar(QWidget *parent)
    : QWidget(parent)
    , m_object(0)
    , m_widget(new QWidget(this))
    , m_timeLine(new QTimeLine(150, this))
    , m_timer(new QTimer(this))
{
    setAutoFillBackground(true);
    m_widget->setContentsMargins(0, 0, 0, 0);
    ui.setupUi(m_widget);
//    setMinimumWidth(m_widget->minimumWidth());
//    setMaximumWidth(m_widget->maximumWidth());
//    setMinimumHeight(m_widget->minimumHeight());
    m_widget->setGeometry(0, -1 * m_widget->height(),
                          m_widget->width(), m_widget->height());
    m_widget->setObjectName(QLatin1String("bar"));

    // we start off hidden
    setMaximumHeight(0);
    hide();

    connect(ui.buttonBox, SIGNAL(accepted()),
            this, SLOT(animateHide()));
    connect(ui.buttonBox, SIGNAL(rejected()),
            this, SLOT(animateHide()));
    connect(m_timeLine, SIGNAL(frameChanged(int)),
            this, SLOT(frameChanged(int)));

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(animateHide()));

    m_timer->setSingleShot(true);
    m_timer->setInterval(10 * 1000);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(animateHide()));
}

void NotificationsBar::setNotifyingObject(QObject *object)
{
    m_object = object;
}

void NotificationsBar::message(const QString &message, NotificationItem::Notification type)
{
    QPalette pal = palette();

    int interval = 10 * 1000;
    QPixmap icon;
    switch (type) {
        case NotificationItem::Success:
            //green
            setStyleSheet(QLatin1String("QWidget#bar{ background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
                                        "stop:0 rgba(0,191,255,175), stop: 0.4 rgba(135,206,250,175), stop:1 rgba(173,216,230,175))}"));
            icon = style()->standardIcon(QStyle::SP_MessageBoxInformation, 0, this).pixmap(m_widget->height() - 5, m_widget->height() - 5);
            ui.buttonBox->clear();
            ui.buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        break;
        case NotificationItem::Information:
            //blue
            setStyleSheet(QLatin1String("QWidget#bar{ background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
                                        "stop:0 rgba(0,191,255,175), stop: 0.4 rgba(135,206,250,175), stop:1 rgba(173,216,230,175))}"));
            icon = style()->standardIcon(QStyle::SP_MessageBoxInformation, 0, this).pixmap(m_widget->height() - 5, m_widget->height() - 5);
            ui.buttonBox->clear();
            ui.buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        break;
        case NotificationItem::Warning:
            //yellow
            setStyleSheet(QLatin1String("QWidget#bar{ background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
                                        "stop:0 rgba(255,255,0,175), stop: 0.4 rgba(255,215,0,175), stop:1 rgba(255,140,0,175))}"));
            icon = style()->standardIcon(QStyle::SP_MessageBoxWarning, 0, this).pixmap(m_widget->height() - 5, m_widget->height() - 5);
            ui.buttonBox->clear();
            ui.buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
        break;
        case NotificationItem::Error:
            //red
            setStyleSheet(QLatin1String("QWidget#bar{ background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
                                        "stop:0 rgba(255,0,0,175), stop: 0.4 rgba(255,99,71,175), stop:1 rgba(250,128,114,175))}"));
            icon = style()->standardIcon(QStyle::SP_MessageBoxCritical, 0, this).pixmap(m_widget->height() - 5, m_widget->height() - 5);
            ui.buttonBox->clear();
            ui.buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        break;
        case NotificationItem::Password:
            //yellow
            setStyleSheet(QLatin1String("QWidget#bar{ background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
                                        "stop:0 rgba(255,255,0,175), stop: 0.8 rgba(255,215,0,175), stop:1 rgba(255,140,0,175))}"));
            icon = style()->standardIcon(QStyle::SP_MessageBoxInformation, 0, this).pixmap(m_widget->height() - 5, m_widget->height() - 5);
            ui.buttonBox->clear();
            ui.buttonBox->addButton(tr("Save Password"), QDialogButtonBox::AcceptRole);
            ui.buttonBox->addButton(tr("Never For This Site"), QDialogButtonBox::RejectRole);
            connect(ui.buttonBox, SIGNAL(accepted()), m_object, SLOT(notifyAccept()));
            connect(ui.buttonBox, SIGNAL(rejected()), m_object, SLOT(notifyReject()));
            connect(this, SIGNAL(decline()), m_object, SLOT(notifyDecline()));
            interval = 20 * 1000;
        break;
    }

    animateShow();
    ui.messageLabel->setText(message);
    ui.iconLabel->setPixmap(icon);
    m_timer->setInterval(interval);
    m_timer->start();

}

void NotificationsBar::animateShow()
{
    if (isVisible())
        return;

    show();
    raise();
    m_timeLine->setFrameRange(-1 * m_widget->height(), 0);
    m_timeLine->setDirection(QTimeLine::Forward);
    disconnect(m_timeLine, SIGNAL(finished()),
                this, SLOT(readyForNext()));
    m_timeLine->start();
}

void NotificationsBar::animateHide()
{
    m_timeLine->setDirection(QTimeLine::Backward);
    m_timeLine->start();
    connect(m_timeLine, SIGNAL(finished()), this, SLOT(readyForNext()));
    if (sender() == m_timer)
        emit decline();
}

void NotificationsBar::readyForNext()
{
    hide();
    if (!m_notificationQueue.isEmpty()) {
        if (m_notificationQueue.first()->m_type == NotificationItem::Password) {
            disconnect(ui.buttonBox, SIGNAL(accepted()), m_object, SLOT(notifyAccept()));
            disconnect(ui.buttonBox, SIGNAL(rejected()), m_object, SLOT(notifyReject()));
            disconnect(this, SIGNAL(decline()), m_object, SLOT(notifyDecline()));
        }
        m_notificationQueue.removeFirst();
    }
    processNotificationQueue();
}

void NotificationsBar::resizeEvent(QResizeEvent *event)
{
    if (event->size().width() != m_widget->width())
        m_widget->resize(event->size().width(), m_widget->height());
    QWidget::resizeEvent(event);
}

void NotificationsBar::frameChanged(int frame)
{
    if (!m_widget)
        return;

    m_widget->move(0, frame);
    int height = qMax(0, m_widget->y() + m_widget->height());
    setMinimumHeight(height);
    setMaximumHeight(height);
}

void NotificationsBar::queueItem(const QString &message, NotificationItem::Notification type, QObject *object)
{
    NotificationItem *item = new NotificationItem(message, type, object);
    m_notificationQueue.append(item);
    if (m_notificationQueue.count() == 1)
        processNotificationQueue();
}

void NotificationsBar::processNotificationQueue()
{
    if (m_notificationQueue.isEmpty())
        return;
    NotificationItem *item = m_notificationQueue.first();
    setNotifyingObject(item->m_object);
    message(item->m_message, item->m_type);
}


