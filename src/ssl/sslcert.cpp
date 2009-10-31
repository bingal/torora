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
#include "crypto.h"
#include "browserapplication.h"
#include "sslstrings.h"
#include <QSslError>

#if defined(Q_OS_WIN32)
#include <windows.h>
#include <wincrypt.h>
#endif

AroraSSLError::AroraSSLError(const QList<QSslError> &errors, const QUrl url)
{
    m_errorid = crypto_rand_string(16);
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

void AroraSSLCertificate::microSSLText(QPainter &p, QRect rect)
{
    QFont normalfont = p.font();
    QFont boldfont = p.font();
    QFontMetrics fb(boldfont);
    QFontMetrics fn(normalfont);
    int skipline = fn.height() + 5;
    QSslCipher sessionCipher = sslConfiguration().sessionCipher();
    QString enc = sessionCipher.encryptionMethod();

    QPoint point = QPoint(rect.x() + (rect.width()/8), (rect.y() + (rect.height()/3) + 48));
    QRect certrect = QRect(point, QSize((rect.width()/8) * 6, 100));
    p.setBrush(Qt::white);
    p.drawRoundedRect(certrect, 20, 15);

    boldfont.setBold(true);

    int startx = certrect.x();
    int starty = certrect.y();
    certrect.setX(certrect.x() + skipline);

    //Issuer Info
    p.setFont(boldfont);
    certrect.setY(certrect.y() + 5);
    p.drawText(certrect, QLatin1String("Encryption: "));
    certrect.setY(certrect.y() + skipline + 5);
    p.setFont(normalfont);
    p.drawText(certrect, (sslDefinition(enc).isEmpty())?QLatin1String("<Cipher Not Specified>"):sslDefinition(enc));

}

void AroraSSLCertificate::miniSSLText(QPainter &p, QRect rect)
{
    QFont normalfont = p.font();
    QFont boldfont = p.font();
    QFontMetrics fb(boldfont);
    QFontMetrics fn(normalfont);
    int skipline = fn.height() + 5;
    int errors = 0;

    QSslCipher sessionCipher = sslConfiguration().sessionCipher();
    QString enc = sessionCipher.encryptionMethod();

    QPoint point = QPoint(rect.x() + (rect.width()/8), (rect.y() + (rect.height()/3) - 32));
    QRect certrect = QRect(point, QSize((rect.width()/8) * 6, 180));
    p.setBrush(Qt::white);
    p.drawRoundedRect(certrect, 20, 15);

    point = QPoint(rect.x() + (rect.width()/2) - 32,
                   rect.y() + (rect.height()/3) - 32);
    p.drawPixmap(point,QPixmap(QString(QLatin1String(":graphics/ssl/%1")).arg(icon(m_polluted)))
                        .scaled(64,64));

    boldfont.setBold(true);


    certrect.setY(certrect.y() + 64);
    int starty = certrect.y();
    certrect.setX(certrect.x() + skipline);
    int startx = certrect.x();

    if (hasError()) {
        /* FIXME: m_errorFrames is a list of frames and frame metadata that use this cert
                 with its associated errors, that's why we only need to iterate through
                 the errors of m_errorFrames->first(). Need a less confusing name! */
        for (errors = 0; errors < m_errorFrames.first()->errors().count(); ++errors) {
            if (errors > 2)
                break;
            SSLString *errorString = sslErrorString(m_errorFrames.first(), errors);
            QString error = errorString->string(6);
            p.setFont(boldfont);
            certrect.setX(startx);
            certrect.setY(certrect.y() + skipline);
            p.drawText(certrect, QLatin1String("Error: "));
            certrect.setX(certrect.x() + fn.width(QLatin1String("Error: ")) + 5);
            p.setFont(normalfont);
            p.drawText(certrect, error);
        }
    }

    if (m_polluted) {
        errors++;
        p.setFont(boldfont);
        certrect.setX(startx);
        certrect.setY(certrect.y() + skipline);
        p.drawText(certrect, QObject::tr("Warning: "));
        certrect.setX(certrect.x() + fn.width(QObject::tr("Warning: ")) + 10);
        p.setFont(normalfont);
        p.drawText(certrect, QObject::tr("Some Items Unencrypted"));
    }

    if (errors == 0) {
        //Issuer Info
        p.setFont(boldfont);
        certrect.setX(startx);
        certrect.setY(certrect.y() + skipline);
        p.drawText(certrect, QLatin1String("Site: "));
        certrect.setX(certrect.x() + fn.width(QLatin1String("Site: ")) + 5);
        p.setFont(normalfont);
        p.drawText(certrect, subject());
    }

    if (errors <= 1) {
        certrect.setX(startx);
        certrect.setY(certrect.y() + skipline);
        p.setFont(boldfont);
        p.drawText(certrect, QLatin1String("Resources: "));
        certrect.setX(certrect.x() + fn.width(QLatin1String("Resources: ")) + 5);
        p.setFont(normalfont);
        p.drawText(certrect, m_resources.join(QLatin1String(",")));
    }

    if (errors <= 2) {
        certrect.setX(startx);
        certrect.setY(certrect.y() + skipline);
        p.setFont(boldfont);
        p.drawText(certrect, QLatin1String("Encryption: "));
        certrect.setX(certrect.x() + fn.width(QLatin1String("Encryption: ") + 5));
        p.setFont(normalfont);
        p.drawText(certrect, (sslDefinition(enc).isEmpty())?QLatin1String("<Cipher Not Specified>"):sslDefinition(enc));
    }
}

void AroraSSLCertificate::fullSSLText(QPainter &p, QRect rect)
{
    QFont normalfont = p.font();
    QFont boldfont = p.font();
    QFontMetrics fb(boldfont);
    QFontMetrics fn(normalfont);
    int skipline = fn.height() + 5;
    int errors = 0;

    QPoint point = QPoint(rect.x() + (rect.width()/2 - 200),
                          (rect.y() + (rect.height()/3) - 32));
    QRect certrect = QRect(point, QSize((400), 250 + 64));
    p.setBrush(Qt::white);
    p.drawRoundedRect(certrect, 20, 15);

    point = QPoint(rect.x() + (rect.width()/2) - 32,
                   rect.y() + (rect.height()/3) - 32);
    p.drawPixmap(point,QPixmap(QString(QLatin1String(":graphics/ssl/%1")).arg(icon(m_polluted)))
                        .scaled(64,64));

    boldfont.setBold(true);


    int startx = certrect.x();
    int starty = certrect.y();
    certrect.setX(certrect.x() + skipline);
    certrect.setY(certrect.y() + 64);

    if (hasError()) {
        /* FIXME: m_errorFrames is a list of frames and frame metadata that use this cert
                 with its associated errors, that's why we only need to iterate through
                 the errors of m_errorFrames->first(). Need a less confusing name! */
        for (errors = 0; errors < m_errorFrames.first()->errors().count(); ++errors) {
            if (errors > 2)
                break;
            SSLString *errorString = sslErrorString(m_errorFrames.first(), errors);
            QString title = errorString->string(0).replace(QLatin1String("<b>"),QLatin1String(""))
                                                  .replace(QLatin1String("</b>"),QLatin1String(""));
            QString text = errorString->string(1).replace(QLatin1String("<b>"),QLatin1String(""))
                                                 .replace(QLatin1String("</b>"),QLatin1String(""));
            p.setFont(boldfont);
            certrect.setY(certrect.y() + skipline);
            p.drawText(certrect, title);
            certrect.setY(certrect.y() + skipline);
            p.setFont(normalfont);
            p.drawText(certrect, text);
        }
    }

    if (m_polluted) {
        p.setFont(boldfont);
        certrect.setY(certrect.y() + (skipline * (errors + 1)));
        p.drawText(certrect, QObject::tr("Page Contains Insecure Elements"));
        certrect.setY(certrect.y() + skipline);
        p.setFont(normalfont);
        p.drawText(certrect, QObject::tr("The page contains items that are not secured with SSL"));
        errors++;
    }

    if (errors <= 0) {
        //Issuer Info
        p.setFont(boldfont);
        certrect.setY(certrect.y() + skipline);
        p.drawText(certrect, issuerTitle());
        certrect.setY(certrect.y() + skipline);
        p.setFont(normalfont);
        p.drawText(certrect, issuerText(false));
    }

    if (errors <= 1) {
        //Encryption Info
        p.setFont(boldfont);
        certrect.setY(certrect.y() + (skipline * 2));
        p.drawText(certrect, encryptionTitle());
        certrect.setY(certrect.y() + skipline);
        p.setFont(normalfont);
        p.drawText(certrect, encryptionText(false));
    }

    if (errors <= 2) {
        //Certificate Info
        p.setFont(boldfont);
        certrect.setY(certrect.y() + (skipline * 4));
        p.drawText(certrect, certificateTitle());
        certrect.setY(certrect.y() + skipline);
        p.setFont(normalfont);
        p.drawText(certrect, certificateText(false));
    }
}

void AroraSSLCertificate::populateSSLText(QPainter &p, QRect rect, bool polluted, const QStringList &resources)
{
    int width = rect.width();
    int height = rect.height();

    //check for polluted frame
    m_polluted = polluted;
    m_resources = resources;

    if (width < 150) {
        qDebug() << "returning" << endl;
        return;
    }else if (width < 250)
        microSSLText(p, rect);
    else if (width < 800)
        miniSSLText(p, rect);
    else
        fullSSLText(p, rect);


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

