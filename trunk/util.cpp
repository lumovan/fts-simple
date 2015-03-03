#include <stdio.h>
#include <QDebug>
#include <QStringList>
#include <QTextCursor>
#include "util.h"

uint16_t ga2uint(QString ga) {
    QStringList parts = ga.trimmed().split("/");
    if (parts.count() != 3) {
        return 0;
    }
    uint16_t ret;
    bool ok;
    ret = parts[2].toUInt(&ok);
    if (!ok) {
        return 0;
    }
    ret |= parts[1].toUInt(&ok) << 8;
    if (!ok) {
        return 0;
    }
    ret += parts[0].toUInt(&ok) << 11;
    if (!ok) {
        return 0;
    }
    return ret;
}


QString uint2ga(uint16_t ga) {
    return QString(QString::number((ga >> 11) & 0x0F).append("/").append(QString::number((ga >>8)& 0x07)).append("/").append(QString::number(ga & 0xFF)));
}

uint16_t pa2uint(QString pa) {
    QStringList parts = pa.trimmed().split(".");
    if (parts.count() != 3) {
        return 0;
    }
    return (parts[2].toUInt() | (parts[1].toUInt() << 8) | (parts[0].toUInt() << 12));
}


QString uint2pa(uint16_t pa) {
    QString ret = "";

    ret.append(QString::number((pa >> 12) & 0x000F));
    ret.append(".");
    ret.append(QString::number((pa >>8) & 0x000F));
    ret.append(".");
    ret.append(QString::number(pa & 0x00FF));

    return ret;
}

QTextEdit *Logger::txe_log;

void Logger::log(QString str) {
    txe_log->append(str);
    QTextCursor c =  txe_log->textCursor();
    c.movePosition(QTextCursor::End);
    txe_log->setTextCursor(c);
}

void Logger::log(const char* format, ...) {
    char dest[2048];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf_s(dest, 2048, format, argptr);
    va_end(argptr);
    log(QString(dest));
}
