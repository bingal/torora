/**
 * Copyright (c) 2009, Benjamin C. Meyer
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

#ifndef SSLERROR_H
#define SSLERROR_H

#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QUrl>
#include <qwebframe.h>
#include <qsslcipher.h>
#include <qsslerror.h>
#include <qsslconfiguration.h>
#include <qobject.h>
#include <qpainter.h>
#include <qdatetime.h>

class QSslErrorPrivate;
class QSslConfiguration;
class QPainter;
class QRect;

class AroraSSLError: public QObject
{
    Q_OBJECT
public:
    AroraSSLError(const QList<QSslError> &errors, const QUrl url);

    QUrl url() { return m_url;}
    void setUrl(const QUrl url) { m_url = url; };
    QString errorid() { return m_errorid;}
    QList<QSslError> errors() { return m_errors;}
    void clear();
    bool isNull() { return m_errors.isEmpty(); }
    void setFrame(QWebFrame *frame) { m_frame = frame; }
    QWebFrame* frame(){ return m_frame; }

protected slots:
    void loadFrame();

private:
    QList<QSslError> m_errors;
    QString m_errorid;
    QUrl m_url;
    QWebFrame *m_frame;
};

class AroraSSLCertificate: public QObject
{
    Q_OBJECT
public:
    AroraSSLCertificate(AroraSSLError *error, const QSslConfiguration sslCfg, const QUrl url);

    QUrl url() { return m_url;}
    QSslConfiguration sslConfiguration() { return m_sslCfg;}
    void setLowGradeEncryption(bool low){ m_LowGradeEncryption = low; }
    bool lowGradeEncryption(){ return m_LowGradeEncryption; }
    void clear();
    bool hasError() { return (!m_errorFrames.isEmpty()); }
    void addFrame(QWebFrame *frame) { if (!m_frames.contains(frame)) { m_frames.append(frame);} }
    void addError(AroraSSLError *error);
    QList<QWebFrame*> frames(){ return m_frames; }
    void removeFrame(QWebFrame *frame) { m_frames.removeAll(frame); }
    void removeError(AroraSSLError *error) { m_errorFrames.removeAll(error); }
    QList<AroraSSLError*> errors(){ return m_errorFrames; };
    QString icon(bool polluted);
    QColor color(bool polluted);
    QString subject();
    QString issuer();
    QString issuerTitle();
    QString issuerText(bool markup);
    QString certificateTitle();
    QString certificateText(bool markup);
    QString encryptionTitle();
    QString encryptionText(bool markup);
    void populateSSLText(QPainter &p, QRect rect, bool polluted, const QStringList &resources);
    QString resourceDefinition(const QString &element);

private:
    QSslCertificate peerCert() {return m_sslCfg.peerCertificate(); }
    void initSSLDefinitions();
    QString sslDefinition(const QString &element);
    void microSSLText(QPainter &p, QRect rect);
    void miniSSLText(QPainter &p, QRect rect);
    void fullSSLText(QPainter &p, QRect rect);

    QList<AroraSSLError*> m_errorFrames;
    QSslConfiguration m_sslCfg;
    QUrl m_url;
    bool m_LowGradeEncryption;
    QList<QWebFrame*> m_frames;
    QMap<QString, QString> sslDefinitions;
    QMap<QString, QString> resourceDefinitions;
    QStringList m_resources;
    bool m_polluted;
};
Q_DECLARE_METATYPE(AroraSSLCertificate*)
Q_DECLARE_METATYPE(AroraSSLError*)

#endif
