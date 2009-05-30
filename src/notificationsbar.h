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

#ifndef NOTIFICATIONSBAR_H
#define NOTIFICATIONSBAR_H

#include <qwidget.h>

#include "browserapplication.h"

#include "ui_notificationsbanner.h"

QT_BEGIN_NAMESPACE
class QTimeLine;
QT_END_NAMESPACE

class NotificationItem
{
public:
    NotificationItem(const QString &message, BrowserApplication::Notification type, QObject *object)
    {
        m_object = object;
        m_message = message;
        m_type = type;
    }
    QObject *m_object;
    QString m_message;
    BrowserApplication::Notification m_type;
};


class NotificationsBar : public QWidget
{
    Q_OBJECT
signals:
    void decline();
public:
    NotificationsBar(QWidget *parent = 0);
    void registerNotifier(QObject *object);
    void queueItem(const QString &message, BrowserApplication::Notification type, QObject *object);

public slots:
    void animateShow();
    void animateHide();
    void message(const QString &message, BrowserApplication::Notification type);
    void setNotifyingObject(QObject *object);
protected:
    void resizeEvent(QResizeEvent *event);
    Ui_NotificationsBanner ui;
    void processNotificationQueue();

private slots:
    void frameChanged(int frame);
    void readyForNext();

private:
    QObject *m_object;
    QWidget *m_widget;
    QTimeLine *m_timeLine;
    QTimer *m_timer;
    typedef QList<NotificationItem*> NotificationQueue;
    NotificationQueue m_notificationQueue;
};

#endif

