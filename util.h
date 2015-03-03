#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <QObject>
#include <QString>
#include <QTextEdit>

uint16_t ga2uint(QString ga);
uint16_t pa2uint(QString pa);
QString uint2ga(uint16_t ga);
QString uint2pa(uint16_t pa);

class Logger  {
public:
    static void log(QString);
    static void log(const char* format, ...);
    static void setTxe (QTextEdit *txe) { txe_log = txe; }

private:
    static QTextEdit *txe_log;
};

#endif // UTIL_H
