/*
 * Copyright 2009 Robert Hogan <robert@roberthogan.net>
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


#ifndef INVALIDSSL_H
#define INVALIDSSL_H

#include <qobject.h>

#include <qsslconfiguration.h>
#include <qsslcipher.h>
#include <qabstractitemmodel.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qsslerror.h>
#include <qurl.h>
#include <qlist.h>

#include "ui_ssldialog.h"

class SSLMessage
{

public:
    SSLMessage(QString &title, QString &text, QPixmap &image);
    ~SSLMessage();

    QString title() { return m_title;}
    QString text() { return m_text;}
    QPixmap image() { return m_image;}
    
private:
    QString m_title;
    QString m_text;
    QPixmap m_image;
};

class AroraSSLError;
class AroraSSLCertificate;
class SSLDialog : public QDialog, public Ui_SSLDialog
{
    Q_OBJECT

signals:

public:
    SSLDialog(QWidget *parent = 0, AroraSSLCertificate *cert = 0, QUrl url = QUrl(), bool polluted = false);
    ~SSLDialog();

private slots:
    void displayNext();
    void displayPrevious();

private:
    void displayMessages(int start);
    void warnWeakCiphers(const QSslCipher &sessionCipher);
    void warnPollutedFrame(const QString &host);

    QList<SSLMessage *> errors;
    int m_next;
    int m_previous;
    QList<QSslError> m_sslErrors;
    bool m_polluted;
};

#endif // INVALIDSSL_H
