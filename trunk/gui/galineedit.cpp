#include "galineedit.h"
#include "util.h"

GALineEdit::GALineEdit(QWidget *parent) : QLineEdit(parent) {
    QObject::connect(this,SIGNAL(textChanged(QString)),this,SLOT(checkValid()));
}

void GALineEdit::checkValid() {
    if (ga2uint(text()) != 0) {
        emit valid(true);
    } else {
        emit valid(false);
    }
}
