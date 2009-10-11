/**
 * Copyright (c) 2009, Robert Hogan
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Benjamin Meyer nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "sslcert.h"
#include "browserapplication.h"
#include "sslstrings.h"
#include <QSslError>
#include <QCryptographicHash>

#if defined(Q_OS_WIN32)
#include <windows.h>
#include <wincrypt.h>
#endif

AroraSSLError::AroraSSLError(const QList<QSslError> &errors, const QUrl url)
{
    /* Create a unique identifier for the error. This will be used to link the
     * 'Proceed' and 'Cancel' buttons with the correct action for the frame
     *  in the error page displayed to the user. */
    QTime midnight(0, 0, 0);
    qsrand(midnight.secsTo(QTime::currentTime()));
    QByteArray hash = QCryptographicHash::hash(QByteArray::number(qrand()), QCryptographicHash::Sha1);
    m_errorid = QString::fromAscii(hash.toHex());

    m_errors = errors;
    m_url = url;
}

void AroraSSLError::clear()
{
    m_errors.clear();
    m_url.clear();
    m_errorid.clear();
}

void AroraSSLError::loadFrame()
{
    BrowserApplication::instance()->m_sslwhitelist.append(m_url.host());
    m_frame->load(m_url);
}

AroraSSLCertificate::AroraSSLCertificate(AroraSSLError *error, const QSslConfiguration sslCfg, const QUrl url)
{
    initSSLDefinitions();
    m_sslCfg = sslCfg;
    if (error)
        m_errorFrames.append(error);
    m_url = url;
    m_LowGradeEncryption = false;
    m_frames << QList<QWebFrame*>();
}

void AroraSSLCertificate::addError(AroraSSLError *error)
{
    QListIterator<AroraSSLError*> errs(m_errorFrames);
    while (errs.hasNext()) {
        AroraSSLError *err = errs.next();
        if (err->errors() == error->errors() && err->frame() == error->frame())
          return;
    }
    m_errorFrames.append(error);
}

void AroraSSLCertificate::clear()
{
    m_errorFrames.clear();
    m_url.clear();
    m_frames.clear();
    m_LowGradeEncryption = false;
}

QString AroraSSLCertificate::icon(bool polluted)
{
    QString icon;
    if (hasError())
        icon = QLatin1String("security-low.png");
    else if (lowGradeEncryption() || polluted)
        icon = QLatin1String("security-medium.png");
    else
        icon = QLatin1String("security-high.png");
    return icon;
}

QColor AroraSSLCertificate::color(bool polluted)
{
    if (hasError())
        return QColor(255, 0, 0, 127);
    else if (lowGradeEncryption() || polluted)
        return QColor(255, 255, 0, 127);
    else 
        return QColor(0, 255, 0, 127);
}

QString AroraSSLCertificate::subject()
{
    return (peerCert().subjectInfo(QSslCertificate::CommonName).isEmpty()) ?
                      tr("[Subject Not Named]"):peerCert().subjectInfo(QSslCertificate::CommonName);
}

QString AroraSSLCertificate::issuer()
{
    return ((peerCert().issuerInfo(QSslCertificate::Organization).isEmpty())?
                            tr("[Issuer Not Named]"):peerCert().issuerInfo(QSslCertificate::Organization));
}

QString AroraSSLCertificate::issuerTitle()
{
    QString title = tr("Certificate Was Issued By %1").arg(issuer());
    return title;
}

QString AroraSSLCertificate::issuerText(bool markup)
{
    if (markup)
      return tr("This SSL certificate was issued to <b>%1</b> by <b>%2</b>.").arg(subject()).arg(issuer());
    return tr("This SSL certificate was issued to %1 by %2.").arg(subject()).arg(issuer());

}

QString AroraSSLCertificate::certificateTitle()
{
     QString expiryDate = peerCert().expiryDate().toString(QLatin1String("dddd d MMMM yyyy"));
     return tr("Certificate Expires on %1")
                   .arg((expiryDate.isEmpty())?QLatin1String("<Date Not Provided>"):expiryDate);
}

QString AroraSSLCertificate::certificateText(bool markup)
{
     QString expiryDate = peerCert().expiryDate().toString(QLatin1String("dddd d MMMM yyyy"));
     QString effectiveDate = peerCert().effectiveDate().toString(QLatin1String("dddd d MMMM yyyy"));

     if (markup)
        return tr("This certificate became effective on <b>%1</b> and expires on <b>%2</b>.")
                   .arg((effectiveDate.isEmpty())?QLatin1String("<Date Not Provided>"):effectiveDate)
                   .arg((expiryDate.isEmpty())?QLatin1String("<Date Not Provided>"):expiryDate);

     return tr("This certificate became effective on %1 and expires on %2.")
                   .arg((effectiveDate.isEmpty())?QLatin1String("<Date Not Provided>"):effectiveDate)
                   .arg((expiryDate.isEmpty())?QLatin1String("<Date Not Provided>"):expiryDate);

}

QString AroraSSLCertificate::encryptionTitle()
{
        QSslCipher sessionCipher = sslConfiguration().sessionCipher();
        QString protocol = sessionCipher.protocolString();
        if (protocol.isEmpty())
            return tr("Security Protocol Not Specified");
        else
            return tr("%1 Secured with the %2 Protocol")
                   .arg(m_resources.join(QLatin1String(","))).arg(protocol);
}

QString AroraSSLCertificate::encryptionText(bool markup)
{
    QSslCipher sessionCipher = sslConfiguration().sessionCipher();
    QString auth = sessionCipher.authenticationMethod();
    QString enc = sessionCipher.encryptionMethod();
    QString exchange = sessionCipher.keyExchangeMethod();

    if (markup)
        return  tr("This means Arora authenticated the connection with <b>%1</b> using the <b>%2</b> algorithm, "
                "exchanged keys using the <b>%3</b> method, and agreed to encrypt communication "
                "using <b>%4</b>. ")
                .arg(m_url.host())
                .arg((sslDefinition(auth).isEmpty())?QLatin1String("<Algorithm Not Specified>"):sslDefinition(auth))
                .arg((sslDefinition(exchange).isEmpty())?QLatin1String("<Algorithm Not Specified>"):sslDefinition(exchange))
                .arg((sslDefinition(enc).isEmpty())?QLatin1String("<Cipher Not Specified>"):sslDefinition(enc));

    return  tr("This means Arora authenticated the connection with %1 using the %2 algorithm, "
            "exchanged keys using the %3 method, and agreed to encrypt communication "
            "using %4. ")
            .arg(m_url.host())
            .arg((sslDefinition(auth).isEmpty())?QLatin1String("<Algorithm Not Specified>"):sslDefinition(auth))
            .arg((sslDefinition(exchange).isEmpty())?QLatin1String("<Algorithm Not Specified>"):sslDefinition(exchange))
            .arg((sslDefinition(enc).isEmpty())?QLatin1String("<Cipher Not Specified>"):sslDefinition(enc));

}

void AroraSSLCertificate::initSSLDefinitions()
{
    sslDefinitions.insert(QLatin1String("DH"), QLatin1String("Diffie-Hellman"));
    sslDefinitions.insert(QLatin1String("RSA"), QLatin1String("Rivest-Shamir-Adleman"));
    sslDefinitions.insert(QLatin1String("AES(256)"),  tr("AES (Advanced Encryption Standard) with 256 bits security"));
    sslDefinitions.insert(QLatin1String("RC4(128)"),  tr("RC4 (Rivest Cipher 4) with 128 bits security."));
    sslDefinitions.insert(QLatin1String("3DES(128)"),  tr("3-DES (Triple Data Encryption Standard) with 128 bits security."));
    sslDefinitions.insert(QLatin1String("3DES(168)"),  tr("3-DES (Triple Data Encryption Standard) with 168 bits security."));
    sslDefinitions.insert(QLatin1String("3DES(256)"),  tr("3-DES (Triple Data Encryption Standard) with 256 bits security."));

    resourceDefinitions.insert(QLatin1String("IMG"), QLatin1String("Image(s)"));
    resourceDefinitions.insert(QLatin1String("SCRIPT"), QLatin1String("Javascript"));

}

QString AroraSSLCertificate::sslDefinition(const QString &element)
{
    QString definition = sslDefinitions[element];

    if (definition.isEmpty())
        return element;
    return definition;
}

QString AroraSSLCertificate::resourceDefinition(const QString &element)
{
    QString definition = resourceDefinitions[element];

    if (definition.isEmpty())
        return element;
    return definition;
}

