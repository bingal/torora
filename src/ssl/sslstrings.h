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

#ifndef SSLSTRINGS_H
#define SSLSTRINGS_H

#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QUrl>
#include <qsslerror.h>
#include <qsslcipher.h>

#define SECURITY_HIGH 2
#define SECURITY_MEDIUM 1
#define SECURITY_LOW 0

class QSslErrorPrivate;
class AroraSSLError;

class SSLString
{

public:
    SSLString(QStringList &strings, QPixmap &image);
    ~SSLString();

    QStringList strings() { return m_strings;}
    QString string(int index) { return m_strings[index];}
    QPixmap image() { return m_image;}
    
private:
    QStringList m_strings;
    QPixmap m_image;
};

SSLString *sslErrorString(AroraSSLError *error, int ref);
QString sslErrorHtml(AroraSSLError *error);
bool lowGradeEncryption(const QSslCipher &sessionCipher);
bool weakProtocol(const QSslCipher &sessionCipher);
bool weakCipher(const QSslCipher &sessionCipher);
bool weakBitSecurity(const QSslCipher &sessionCipher);

#endif
