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

#include "sslstrings.h"
#include "browsermainwindow.h"
#include "browserapplication.h"
#include "sslcert.h"

#include <qbuffer.h>
#include <qdesktopservices.h>
#include <qfile.h>
#include <qdebug.h>
#include <qstring.h>
#include <qicon.h>
#include <qstyle.h>
#include <QSslError>
#include <QDateTime>

SSLString::SSLString(QStringList &strings, QPixmap &image)
{
    m_strings = strings;
    m_image = image;
}

SSLString *sslErrorString(AroraSSLError *error, int ref)
{
    BrowserMainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QStringList errStr;
    QPixmap image;

    QString subject = error->errors().at(ref).certificate().subjectInfo(QSslCertificate::CommonName);
    QString issuerinfo = error->errors().at(ref).certificate().issuerInfo(QSslCertificate::CommonName);
    QString surl = error->url().host();
    QString effectiveDate = error->errors().at(ref).certificate().effectiveDate().toString();
    QString expiryDate = error->errors().at(ref).certificate().expiryDate().toString();
    QString proceedButton = QString(QLatin1String("<object type=\"application/x-qt-plugin\" classid=\"QPushButton\" name=\"SSLProceedButton%1\" height=25 width=110></object>\n"
                            "<script>\n"
                            "document.SSLProceedButton%2.text = 'Proceed Anyway';\n"
                            "</script>\n")).arg(error->errorid()).arg(error->errorid());
    QString cancelButton = QString(QLatin1String("<object type=\"application/x-qt-plugin\" classid=\"QPushButton\" name=\"SSLCancelButton%1\" height=25 width=110></object>\n"
                            "<script>\n"
                            "document.SSLCancelButton%2.text = 'Go Back';\n"
                            "</script>\n")).arg(error->errorid()).arg(error->errorid());

    switch (error->errors().at(ref).error()) {
        case QSslError::NoError:
            errStr << QObject::tr("No error");
            errStr << QObject::tr("No error");
            errStr << proceedButton;
            errStr << cancelButton;
            break;
        case QSslError:: UnableToGetIssuerCertificate:
            errStr << QObject::tr("<b>%1</b> Has A Broken Security Configuration!")
                                .arg(surl);
            errStr << QObject::tr("They're telling us that their security certificate is available"
                                  "from %1, but it is not.").arg(issuerinfo);
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << QString();
            errStr << cancelButton;
            errStr << QObject::tr("Broken Security Configuration!");
            break;
        case QSslError:: UnableToDecryptCertificateSignature:
            errStr << QObject::tr("<b>%1</b> Has A Broken Security Configuration!")
                                .arg(surl);
            errStr << QObject::tr("The certificate signature could not be decrypted");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << QString();
            errStr << cancelButton;
            errStr << QObject::tr("Corrupt Certificate!");
            break;
        case QSslError:: UnableToDecodeIssuerPublicKey:
            errStr << QObject::tr("<b>%1</b> Has A Broken Security Configuration!")
                                .arg(surl);
            errStr << QObject::tr("The public key in the certificate could not be read");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and has made a clumsy attempt to forge their security certificate.").arg(surl);
            errStr << QObject::tr("<b>Best Case Scenario:</b> There was an undetected internal error in Arora.");
            errStr << QString();
            errStr << cancelButton;
            errStr << QObject::tr("Corrupt Certificate!");
            break;
        case QSslError:: CertificateSignatureFailed:
            errStr << QObject::tr("<b>%1</b> Has A Broken Security Configuration!")
                                .arg(surl);
            errStr << QObject::tr("The signature of the certificate is invalid");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and has made a clumsy attempt to forge their security certificate.").arg(surl);
            errStr << QObject::tr("<b>Best Case Scenario:</b> There was an undetected internal error in Arora.");
            errStr << QString();
            errStr << cancelButton;
            errStr << QObject::tr("Invalid Signature!");
            break;
        case QSslError:: CertificateNotYetValid:
            errStr << QObject::tr("Strange, it's not %1 yet is it?").arg(effectiveDate);
            errStr << QObject::tr("The security certificate %1 has given us is not valid until %2.").arg(surl).arg(effectiveDate);
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and has made a clumsy attempt to forge their security certificate.").arg(surl);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of %1 have deployed their security certificate too early.");
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Invalid Date!");
            break;
        case QSslError:: CertificateExpired:
            errStr << QObject::tr("This Certificate is Out of Date.").arg(expiryDate);
            errStr << QObject::tr("The security certificate %1 has given us is expired on %2.").arg(surl).arg(expiryDate);
            errStr << QObject::tr("<b>Worst Case Scenario:</b> This security certificate is compromised and you may end up being redirected to a malicious website.").arg(surl);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of %1 have forgotten to update their security certificate.");
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Out of Date!");
            break;
        case QSslError:: InvalidNotBeforeField:
            errStr << QObject::tr("Invalid Date");
            errStr << QObject::tr("The certificate's notBefore field contains an invalid time");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> There was an undetected internal error in Arora.");
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Invalid Date!");
            break;
        case QSslError:: InvalidNotAfterField:
            errStr << QObject::tr("Invalid Date");
            errStr << QObject::tr("The certificate's notAfter field contains an invalid time");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> There was an undetected internal error in Arora.");
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Invalid!");
            break;
        case QSslError:: SelfSignedCertificate:
            errStr << QObject::tr("This Site is Using a Home-Made Certificate.");
            errStr << QObject::tr("The security certificate is self-signed. This means that "
                                  "the website presenting it to us has not had it signed by anyone Arora knows or trusts.");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is pretending to be %1 and has rolled their own certificate to imitate them.").arg(surl);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of %1 have not paid a certificate authority to sign their website's certificate.").arg(surl);
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Home-made Certificate!");
            break;
        case QSslError:: SelfSignedCertificateInChain:
            errStr << QObject::tr("This Site is Using a Home-Made Certificate.");
            errStr << QObject::tr("The root certificate of the certificate chain is self-signed, and untrusted.");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is pretending to be %1 and has rolled their own certificate to imitate them.").arg(surl);
            errStr << QObject::tr("<b>Best Case Scenario:</b> Arora does not recognize <b>%1</b> as a trusted vendor.").arg(issuerinfo);
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Home-made Certificate!");
            break;
        case QSslError:: UnableToGetLocalIssuerCertificate:
            errStr << QObject::tr("<b>%1</b> Has A Broken Security Configuration!")
                                .arg(surl);
            errStr << QObject::tr("The issuer certificate of a locally looked up certificate could not be found");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> have muddled their security configuration.").arg(surl);
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Broken Configuration!");
            break;
        case QSslError:: UnableToVerifyFirstCertificate:
            errStr << QObject::tr("<b>%1</b> Has A Broken Security Configuration!")
                                .arg(surl);
            errStr << QObject::tr("No certificates could be verified");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> have muddled their security configuration.").arg(surl);
            errStr << QString();
            errStr << cancelButton;
            errStr << QObject::tr("Broken Configuration!");
            break;
        case QSslError:: InvalidCaCertificate:
            errStr << QObject::tr("<b>%1</b> Has A Broken Security Configuration!")
                                .arg(surl);
            errStr << QObject::tr("One of the CA certificates is invalid");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << QString();
            errStr << cancelButton;
            errStr << QObject::tr("Invalid CA Cert!");
            break;
        case QSslError:: PathLengthExceeded:
            errStr << QObject::tr("<b>%1</b> Has A Broken Security Configuration!")
                                .arg(surl);
            errStr << QObject::tr("The basicConstraints pathlength parameter has been exceeded");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << QString();
            errStr << cancelButton;
            errStr << QObject::tr("Broken Configuration!");
            break;
        case QSslError:: InvalidPurpose:
            errStr << QObject::tr("No error");
            errStr << QObject::tr("The supplied certificate is unsuited for this purpose");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Unsuitable Cert!");
            break;
        case QSslError:: CertificateUntrusted:
            errStr << QObject::tr("No error");
            errStr << QObject::tr("The root CA certificate is not trusted for this purpose");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Not Trusted!");
            break;
        case QSslError:: CertificateRejected:
            errStr << QObject::tr("<b>%1</b> Has A Broken Security Configuration!")
                                .arg(surl);
            errStr << QObject::tr("The root CA certificate is marked to reject the specified purpose");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Rejected by CA!");
            break;
        case QSslError:: SubjectIssuerMismatch: // hostname mismatch
            errStr << QObject::tr("No error");
            errStr << QObject::tr("The current candidate issuer certificate was rejected because its"
                        " subject name did not match the issuer name of the current certificate");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Rejected by CA!");
            break;
        case QSslError:: AuthorityIssuerSerialNumberMismatch:
            errStr << QObject::tr("No error");
            errStr << QObject::tr("The current candidate issuer certificate was rejected because"
                                                  " its issuer name and serial number was present and did not match the"
                                                  " authority key identifier of the current certificate");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Rejected by CA!");
            break;
        case QSslError:: NoPeerCertificate:
            errStr << QObject::tr("<b>%1</b> Has A Broken Security Configuration!")
                                .arg(surl);
            errStr << QObject::tr("The peer did not present any certificate");
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << QString();
            errStr << cancelButton;
            errStr << QObject::tr("No Certificate!");
            break;
        case QSslError:: HostNameMismatch:
            errStr << QObject::tr("Mmm.. This may not be <b>%1</b>")
                                .arg(surl);
            errStr << QObject::tr("You asked for <b>%1</b> but the site certificate gives its name as <b>%2</b>")
                                .arg(surl).arg(subject);
            errStr << QObject::tr("<b>Worst Case Scenario:</b> Someone is intercepting your communications to %1 and re-directing you to %2.").arg(surl).arg(subject);
            errStr << QObject::tr("<b>Best Case Scenario:</b> The owners of <b>%1</b> and <b>%2</b> are the same, and have used the same security certificate for both.").arg(surl).arg(subject);
            errStr << proceedButton;
            errStr << cancelButton;
            errStr << QObject::tr("Wrong Name!");
            break;
        case QSslError:: NoSslSupport:
            break;
        default:
            errStr << QObject::tr("No error");
            errStr << QObject::tr("Unknown error");
            break;
    }

    image = QPixmap(QLatin1String(":graphics/ssl/security-low.png")).scaled(QSize(32,32));

    SSLString *msg = new SSLString(errStr,image);
    return msg;
}

QString sslErrorHtml(AroraSSLError *error)
{
    QFile file(QLatin1String(":/sslwarning.html"));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "WebPage::handleUnsupportedContent" << "Unable to open torcheck.html";
        return QString();
    }
    QString title, headline, intro, bullettwo, bulletthree, bulletfour, img;

    SSLString *errorStrings = sslErrorString(error, 0);
    img = QLatin1String(":graphics/ssl/security-low.png");

    QString html = QString(QLatin1String(file.readAll()))
                        .arg(title)
                        .arg(QString())
                        .arg(errorStrings->string(0))
                        .arg(errorStrings->string(1))
                        .arg(errorStrings->string(2))
                        .arg(errorStrings->string(3))
                        .arg(errorStrings->string(4))
                        .arg(errorStrings->string(5));

    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    QIcon icon = QIcon(img);
    QPixmap pixmap = icon.pixmap(QSize(32, 32));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("IMAGE_BINARY_DATA_HERE"),
                     QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }
    return html;
}

bool lowGradeEncryption(const QSslCipher &sessionCipher)
{
    if (weakProtocol(sessionCipher) || weakCipher(sessionCipher) ||
        weakBitSecurity(sessionCipher))
        return true;
    return false;

}

bool weakProtocol(const QSslCipher &sessionCipher)
{
    if (sessionCipher.protocol() == QSsl::SslV2)
        return true;
    return false;
}

bool weakCipher(const QSslCipher &sessionCipher)
{
    QString enc = sessionCipher.encryptionMethod();
    QStringList weakCiphers;
    weakCiphers << QLatin1String("RC2(128)") <<  QLatin1String("RC4(92)")
                << QLatin1String("RC2(256)") <<  QLatin1String("RC4(64)");
    if (weakCiphers.contains(enc))
        return true;
    return false;
}

bool weakBitSecurity(const QSslCipher &sessionCipher)
{
    int bits = sessionCipher.usedBits();

    if (bits < 128)
        return true;
    return false;
}
