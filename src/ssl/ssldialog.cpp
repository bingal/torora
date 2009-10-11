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


#include "ssldialog.h"
#include "browserapplication.h"
#include "sslstrings.h"
#include "sslcert.h"

#include <qdebug.h>
#include <qdatetime.h>

#define MAXERRORS 2

SSLMessage::SSLMessage(QString &title, QString &text, QPixmap &image)
{
    m_title = title;
    m_text = text;
    m_image = image;
}

SSLDialog::SSLDialog(QWidget *parent, AroraSSLCertificate *cert, QUrl url, bool polluted)
    : QDialog(parent),
    errors(),
    m_polluted(polluted)
{
    Q_UNUSED(url);
    QStringList sslErrorTitle;
    QStringList sslErrorText;

    setupUi(this);

    SSLMessage *msg;
    QPixmap image;
    QString title;
    QString text;

    /*FIXME: not sure we always want the first one here */
    AroraSSLError *error = (!cert->errors().isEmpty())?cert->errors().first():0L;
    QSslConfiguration sslCfg = cert->sslConfiguration();
    if (error) {
        m_sslErrors = error->errors();
        setWindowTitle(QLatin1String("The Certificate Is Not Valid"));
        for (int i = 0; i < m_sslErrors.count(); ++i) {
            SSLString *errorString = sslErrorString(error, i);
            title = errorString->string(0);
            text = errorString->string(1);
            image = errorString->image();
            msg = new SSLMessage(title,text,image);
            errors.append(msg);
        }
    } else
        setWindowTitle(QLatin1String("The Certificate Is Valid"));

    warnPollutedFrame(cert->url().host());

    if (!sslCfg.isNull()) {
        image = QPixmap(QLatin1String(":graphics/ssl/security-high.png")).scaled(QSize(32,32));
        QSslCipher sessionCipher = sslCfg.sessionCipher();

        qDebug() << "sessionCipher.isNull()" << sessionCipher.isNull();
        warnWeakCiphers(sessionCipher);
        title = cert->issuerTitle();
        text = cert->issuerText(true);
        msg = new SSLMessage(title, text, image);
        errors.append(msg);

        title = cert->certificateTitle();
        text = cert->certificateText(true);
        msg = new SSLMessage(title, text, image);
        errors.append(msg);

        title = cert->encryptionTitle();
        text = cert->encryptionText(true);
        msg = new SSLMessage(title, text, image);
        errors.append(msg);
    } else {
        image = QPixmap(QLatin1String(":graphics/ssl/security-medium.png")).scaled(QSize(32,32));
        title = tr("Certificate Information Not Available");
        text =   tr("Due to a known bug in this version of Qt, certificate information is not available for this request. "
                    "You can refresh the page to retrieve the certificate information.");
        msg = new SSLMessage(title, text, image);
        errors.append(msg);
    }
    PrevButton->setVisible(false);
    if (m_sslErrors.count() <= MAXERRORS) {
        NextButton->setText(QLatin1String("Close   "));
    }
    connect(NextButton,SIGNAL(clicked()), this, SLOT(displayNext()));
    connect(PrevButton,SIGNAL(clicked()), this, SLOT(displayPrevious()));
    displayMessages(0);

}

void SSLDialog::displayNext()
{
    if (m_next >= errors.count()) {
        accept();
        return;
    }
    displayMessages(m_next);
}

void SSLDialog::displayPrevious()
{
    displayMessages(m_previous);
}

void SSLDialog::displayMessages(int start)
{

    if (start < MAXERRORS)
        start = 0;
    else
        m_previous = (start - MAXERRORS);

    ErrorImage1->setPixmap(errors[start]->image());
    ErrorTitle1->setText(errors[start]->title());
    ErrorText1->setText(errors[start]->text());
    start++;

    if (errors.count() > start) {
        ErrorImage2->setVisible(true);
        ErrorTitle2->setVisible(true);
        ErrorText2->setVisible(true);
        ErrorImage2->setPixmap(errors[start]->image());
        ErrorTitle2->setText(errors[start]->title());
        ErrorText2->setText(errors[start]->text());
        start++;
    }else{
        ErrorImage2->setVisible(false);
        ErrorTitle2->setVisible(false);
        ErrorText2->setVisible(false);
    }

    m_next = start;
    if (m_next >= errors.count())
        NextButton->setText(QLatin1String("Close   "));
    else
        NextButton->setText(QLatin1String("Next >> "));
    if (m_next > MAXERRORS)
        PrevButton->setVisible(true);
    else
        PrevButton->setVisible(false);
}

void SSLDialog::warnPollutedFrame(const QString &host)
{
    if (!m_polluted)
        return;
    QPixmap image = QPixmap(QLatin1String(":graphics/ssl/security-medium.png")).scaled(QSize(32,32));
    QString title =  tr("Page Includes Resources That Are Not Secure");
    QString text =  tr("The connection to %1 is encrypted but the page includes resources which are not "
                       "secure and which could be modified by a third party to alter the behaviour of "
                       "the page.").arg(host);
    SSLMessage *msg = new SSLMessage(title, text, image);
    errors.append(msg);
}

void SSLDialog::warnWeakCiphers(const QSslCipher &sessionCipher)
{
    QString cipher = sessionCipher.name();
    QString auth = sessionCipher.authenticationMethod();
    QString enc = sessionCipher.encryptionMethod();
    QString protocol = sessionCipher.protocolString();
    int bits = sessionCipher.usedBits();

    qDebug() << sessionCipher;
    QPixmap image = QPixmap(QLatin1String(":graphics/ssl/security-medium.png")).scaled(QSize(32,32));
    QString title;
    QString text;
    SSLMessage *msg;

    if (weakProtocol(sessionCipher)) {
        title =  tr("Website Uses A Weak Security Protocol: %1").arg(protocol);
        text =  tr("This website is securing your session with the %1 protocol. This protocol "
                                     "is considered insecure for modern use.").arg(protocol);
        msg = new SSLMessage(title, text, image);
        errors.append(msg);
    }

    if (weakCipher(sessionCipher)) {
        title =  tr("Website Uses a Weak Encryption Standard: %1").arg(enc);
        text =  tr("This website is securing your session with the %1 encryption cipher. This cipher "
                   "is considered insecure for modern use.").arg(enc);
        msg = new SSLMessage(title, text, image);
        errors.append(msg);
    }

    if (weakBitSecurity(sessionCipher)) {
        title =  tr("Website Is Using Less Than 256 bit Security: %1 bits used").arg(bits);
        text =  tr("This website is securing your session with %1 bits of security. 256 bits is "
                   "preferred, though websites may use less for performance reasons.").arg(bits);
        msg = new SSLMessage(title, text, image);
        errors.append(msg);
    }

}

SSLDialog::~SSLDialog()
{
}

