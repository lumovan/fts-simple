#include "knxcfgmain.h"
#include "falcon_if.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication *a;
    a = new QApplication(argc, argv);
    KnxCfgMain *w = new KnxCfgMain();
    w->afterConstructor();
    w->show();

    int ret = a->exec();
    delete w;

    return ret;
}
