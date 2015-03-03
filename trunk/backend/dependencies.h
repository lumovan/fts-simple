#ifndef DEPENDENCIES_H
#define DEPENDENCIES_H

#include <QtXml>
#include "commobject.h"
#include "parameter.h"

class Dependency {
public:
    Dependency(Parameter *param, QDomElement elem);
    Dependency(CommObject *co, QDomElement elem);

    QString getParentID() { return parentID; }
    Parameter *getParam() { return param; }
    CommObject *getCO() { return co; }
    QList<Dependency*> children;
    bool enabled;
    double *parentMinValue, *parentMaxValue;

private:
    Parameter *param;
    CommObject *co;

    QString parentID;
};

class Dependencies {
public:
    Dependencies();
    void addDependency(Dependency*);
    void propagateValues(Dependency *dep);
    bool isEnabled(Parameter *param);
    bool isEnabled(CommObject *co);
    QList <Dependency*> flatTree(Dependency *root);
    QDomElement condition(QDomDocument doc, Parameter *param);
    QDomElement condition(QDomDocument doc, CommObject *co);

private:
    bool traverseToInsert(Dependency *root, Dependency *leaf);
    void rebuildTree();
    Dependency* findByParam(Dependency *root, Parameter *param);
    Dependency* findByCO(Dependency *root, CommObject *co);

    QList<Dependency*> top;

    QList<Dependency*> unhandled;
};

#endif // DEPENDENCIES_H
