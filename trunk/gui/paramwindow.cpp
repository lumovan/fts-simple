#include <QLabel>
#include <QString>
#include <QSpacerItem>
#include <QComboBox>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QResizeEvent>
#include "paramwindow.h"

ParamWindow::ParamWindow(QWidget *parent): QDialog(parent) {
    QScrollArea *scroll = new QScrollArea(this);
    activeDevice = NULL;
    layout = new QGridLayout(this);
    setLayout(layout);
    canvas = new QWidget(this);
    canvasLayout = new QGridLayout(canvas);
    canvas->setLayout(canvasLayout);
    scroll->horizontalScrollBar()->setSingleStep(30);
//    layout->addWidget(canvas);
    layout->addWidget(scroll);
    scroll->setWidget(canvas);
    //canvas->setGeometry(0,0,100,100);
}

void ParamWindow::updateActiveDevice(KDevice *device) {
    activeDevice = device;

    for (QList<QWidget*>::iterator iter = editors.begin(); iter != editors.end(); iter++) {
        delete *iter;
    }
    editors.clear();
    for (QList<QLabel*>::iterator iter = labels.begin(); iter != labels.end(); iter++) {
        delete *iter;
    }
    labels.clear();
    for (QList<QLabel*>::iterator iter = units.begin(); iter != units.end(); iter++) {
        delete *iter;
    }
    units.clear();
    if (activeDevice == NULL) {
        return;
    }

    QMap<QString,Parameter*> paramlist = device->getParameters();
    printf("#C"); fflush(stdout);

#if 0 // Memory leak known but accepted, fix if you know how
    for (QList<QTableWidgetItem*>::iterator w = items.begin(); w != items.end(); w++) {
        printf("##C%s",(*w)->text().toLatin1().data()); fflush(stdout);
        delete *w;
    }
    printf("#D"); fflush(stdout);
#endif

    printf("#E"); fflush(stdout);
    int row = 0;
    QStringList pkeys = activeDevice->getParamKeys();

    for (QStringList::iterator iter = pkeys.begin(); iter != pkeys.end(); iter++) {
        if (paramlist[*iter]->getDescription().compare("") == 0) {
            continue;
        }

        Parameter *param = paramlist[*iter];
        labels.append(new QLabel(param->getDescription()));
        QStringList items = activeDevice->getParamTypes().items(param);
        if (items.length() > 0) {
            QComboBox *cb = new QComboBox(canvas);
            cb->addItems(items);
            QString entry = activeDevice->getParamTypes().value2list(param,param->getData());
            cb->setCurrentIndex(cb->findText(entry));
            QObject::connect(cb,SIGNAL(currentIndexChanged(QString)),this,SLOT(valueModified(QString)));
            editors.append(cb);
        } else {
            QLineEdit *le = new QLineEdit(canvas);
            le->setText(param->getData());
            QObject::connect(le,SIGNAL(textEdited(QString)),this,SLOT(valueModified(QString)));
            editors.append(le);
        }
        units.append(new QLabel(paramlist[*iter]->unit()));

        canvasLayout->addWidget(labels.last(),row,0,1,1);
        canvasLayout->addWidget(editors.last(),row,1,1,1);
        editors.last()->setProperty("reference",QVariant(*iter));
        canvasLayout->addWidget(units.last(),row,2,1,1);

        if (activeDevice->getParameterEnabled(*iter)) {
            labels[row]->show();
            editors[row]->show();
            units[row]->show();
        } else {
            labels[row]->hide();
            editors[row]->hide();
            units[row]->hide();
        }

        row++;
    }
}

void ParamWindow::valueModified(QString value) {
    if (activeDevice == NULL) {
        return;
    }
    printf("vm\n"); fflush(stdout);
    printf("Update: %s %s\n",QObject::sender()->property("reference").toString().toLatin1().data(),value.toLatin1().data()); fflush(stdout);
    QString id = QObject::sender()->property("reference").toString();
    if (id.isNull()) {
        return;
    }

    if (!activeDevice->getParameters().contains(id)) {
        return;
    }
    QString data = activeDevice->getParamTypes().list2value(activeDevice->getParameters()[id],value);
    if (data.isNull()) {
        data = value;
    }

    if (!(activeDevice->setParameter(id,data))) {
        printf("Failed writing\n"); fflush(stdout);
    }
    emit paramModified();

    for (uint16_t row = 0; row < editors.count(); row++) {
        id = editors[row]->property("reference").toString();
        if (id.isNull()) {
            continue;
        }
        if (activeDevice->getParameterEnabled(id)) {
            labels[row]->show();
            editors[row]->show();
            units[row]->show();
        } else {
            labels[row]->hide();
            editors[row]->hide();
            units[row]->hide();
        }
    }
}

void ParamWindow::resizeEvent(QResizeEvent* e) {
    QDialog::resizeEvent(e);
    canvas->setGeometry(0,0,e->size().width()-50,e->size().height());
}
