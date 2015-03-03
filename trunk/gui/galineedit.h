#ifndef GALINEEDIT_H
#define GALINEEDIT_H

#include <QLineEdit>

class GALineEdit : public QLineEdit {
    Q_OBJECT
public:
    GALineEdit(QWidget *parent);

signals:
    void valid(bool);

private slots:
    void checkValid();
};

#endif // GALINEEDIT_H
