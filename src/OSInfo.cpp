/* Copyright (C) 2005-2009, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "OSInfo.h"
#include "Version.h"

#ifdef Q_WS_X11
#include <sys/utsname.h>
#endif

#ifdef Q_WS_MAC
#include <mach-o/arch.h>
#undef qDebug
#include <CoreServices/CoreServices.h>
#endif

QString OSInfo::getMacHash(const QHostAddress &qhaBind) {
	foreach(const QNetworkInterface &qni, QNetworkInterface::allInterfaces()) {
		if (! qni.isValid())
			continue;
		if (! (qni.flags() & (QNetworkInterface::IsUp | QNetworkInterface::IsRunning)))
			continue;
        if (qni.flags() & QNetworkInterface::IsLoopBack)
			continue;
		if (qni.hardwareAddress().isEmpty())
			continue;

		foreach(const QNetworkAddressEntry &qnae, qni.addressEntries()) {
			const QHostAddress &qha = qnae.ip();
			if (((qhaBind == QHostAddress::Any) && (qha.protocol() ==  QAbstractSocket::IPv4Protocol)) || (qha == qhaBind)) {
				return QString::fromAscii(QCryptographicHash::hash(qni.hardwareAddress().toAscii(), QCryptographicHash::Sha1).toHex());
			}
		}
	}
	return QString();
}

QString OSInfo::getOS() {
#if defined(Q_WS_WIN)
	return QLatin1String("Win");
#elif defined(Q_WS_MAC)
	return QLatin1String("OSX");
#else
	return QLatin1String("X11");
#endif
}

QString OSInfo::getOSVersion() {
#if defined(Q_WS_WIN)
	OSVERSIONINFOEXW ovi;
	memset(&ovi, 0, sizeof(ovi));

	ovi.dwOSVersionInfoSize=sizeof(ovi);
	GetVersionEx(reinterpret_cast<OSVERSIONINFOW *>(&ovi));

	QString os;
	os.sprintf("%d.%d.%d.%d", ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber, (ovi.wProductType == VER_NT_WORKSTATION) ? 1 : 0);
	return os;
#elif defined(Q_OS_MAC)
	SInt32 major, minor, bugfix;
	OSErr err = Gestalt(gestaltSystemVersionMajor, &major);
	if (err == noErr)
		err = Gestalt(gestaltSystemVersionMinor, &minor);
	if (err == noErr)
		err = Gestalt(gestaltSystemVersionBugFix, &bugfix);
	if (err != noErr)
		return QString::number(QSysInfo::MacintoshVersion, 16);

	const NXArchInfo *local = NXGetLocalArchInfo();
	const NXArchInfo *ai = local ? NXGetArchInfoFromCpuType(local->cputype, CPU_SUBTYPE_MULTIPLE) : NULL;
	const char *arch = ai ? ai->name : "unknown";

	QString os;
	os.sprintf("%i.%i.%i (%s)", major, minor, bugfix, arch);
	return os;
#else
#ifdef Q_OS_LINUX
	QProcess qp;
	QStringList args;
	args << QLatin1String("-s");
	args << QLatin1String("-d");
	qp.start(QLatin1String("lsb_release"), args);
	if (qp.waitForFinished(5000)) {
		QString os = QString::fromUtf8(qp.readAll());
		return os.trimmed();
	}
#endif
	struct utsname un;
	if (uname(&un) == 0) {
		QString os;
		os.sprintf("%s %s", un.sysname, un.release);
		return os;
	}
	return QString();
#endif
}

void OSInfo::fillXml(QDomDocument &doc, QDomElement &root, const QString &os, const QString &osver, const QHostAddress &qhaBind) {
	QDomElement tag;
	QDomText t;
        bool bIs64;

        tag=doc.createElement(QLatin1String("machash"));
        root.appendChild(tag);
	t=doc.createTextNode(OSInfo::getMacHash(qhaBind));
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("version"));
	root.appendChild(tag);
	t=doc.createTextNode(QLatin1String(MUMTEXT(MUMBLE_VERSION_STRING)));
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("release"));
	root.appendChild(tag);
	t=doc.createTextNode(QLatin1String(MUMBLE_RELEASE));
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("os"));
	root.appendChild(tag);
	t=doc.createTextNode(os);
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("osver"));
	root.appendChild(tag);
	t=doc.createTextNode(osver);
	tag.appendChild(t);

	tag=doc.createElement(QLatin1String("qt"));
	root.appendChild(tag);
	t=doc.createTextNode(QString::fromLatin1(qVersion()));
	tag.appendChild(t);

#if defined(Q_WS_WIN)
        BOOL bIsWow64 = FALSE;
        IsWow64Process(GetCurrentProcess(), &bIsWow64);
        bIs64 = bIsWow64;
#else
        bIs64 = (QSysInfo::WordSize == 64);
#endif
        tag=doc.createElement(QLatin1String("is64bit"));
        root.appendChild(tag);
        t=doc.createTextNode(QString::number(bIs64 ? 1 : 0));
        tag.appendChild(t);

}
