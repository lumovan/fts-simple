#include "dependencies.h"
#include <limits>

Dependency::Dependency(Parameter *param, QDomElement elem) {
    this->param = param;
    this->co = NULL;

    QDomElement e_depends = elem.firstChildElement("condition");
    if (e_depends.isNull()) {
        //printf("No parent: %s\n",elem.attribute("id").toLatin1().data());
        parentID.clear();
        return;
    } else {
        //printf("Has parent: %s\n",elem.attribute("id").toLatin1().data());
        if (!e_depends.hasAttribute("depends-on")) {
            parentID.clear();
            return;
        }
        if (!e_depends.hasAttribute("value") && !e_depends.hasAttribute("min-value") && e_depends.hasAttribute("max-value")) {
            parentID.clear();
            return;
        }
        parentID = e_depends.attribute("depends-on");
        parentMinValue = NULL;
        parentMaxValue = NULL;
        if (e_depends.hasAttribute("value")) {
            parentMinValue = (double*)malloc(sizeof(double));
            parentMaxValue = (double*)malloc(sizeof(double));
            *parentMinValue = e_depends.attribute("value").toDouble() * (1-(1e-14));
            *parentMaxValue = e_depends.attribute("value").toDouble() * (1+(1e-14));
        } else {
            if (e_depends.hasAttribute("min-value")) {
                parentMinValue = (double*)malloc(sizeof(double));
                *parentMinValue = e_depends.attribute("min-value").toDouble() * (1-(1e-14));
            }
            if (e_depends.hasAttribute("max-value")) {
                parentMaxValue = (double*)malloc(sizeof(double));
                *parentMaxValue = e_depends.attribute("max-value").toDouble() * (1-(1e-14));
            }
        }
    }
    if ((parentMaxValue == NULL) && (parentMinValue == NULL)) {
        parentID.clear();
    }
//   printf("min: %f max: %f\n",!parentMinValue?-100.0f:*parentMinValue,!parentMaxValue?-100.0f:*parentMaxValue);
}

Dependency::Dependency(CommObject *co, QDomElement elem) {
    this->param = NULL;
    this->co = co;

    QDomElement e_depends = elem.firstChildElement("condition");
    if (e_depends.isNull()) {
        //printf("No parent: %s\n",elem.attribute("id").toLatin1().data());
        parentID.clear();
        return;
    } else {
        //printf("Has parent: %s\n",elem.attribute("id").toLatin1().data());
        if (!e_depends.hasAttribute("depends-on")) {
            return;
        }
        if (!e_depends.hasAttribute("value") && !e_depends.hasAttribute("min-value") && e_depends.hasAttribute("max-value")) {
            return;
        }
        parentID = e_depends.attribute("depends-on");
        if (e_depends.hasAttribute("value")) {
            parentMinValue = (double*)malloc(sizeof(double));
            parentMaxValue = (double*)malloc(sizeof(double));
            *parentMinValue = e_depends.attribute("value").toDouble() * (1-(1e-14));
            *parentMaxValue = e_depends.attribute("value").toDouble() * (1+(1e-14));
        } else {
            parentMinValue = NULL;
            parentMaxValue = NULL;
            if (e_depends.hasAttribute("min-value")) {
                parentMinValue = (double*)malloc(sizeof(double));
                *parentMinValue = e_depends.attribute("min-value").toDouble() * (1-(1e-14));
            }
            if (e_depends.hasAttribute("max-value")) {
                parentMaxValue = (double*)malloc(sizeof(double));
                *parentMaxValue = e_depends.attribute("max-value").toDouble() * (1-(1e-14));
            }
        }
    }
    if ((parentMaxValue == NULL) && (parentMinValue == NULL)) {
        parentID.clear();
    }
}



Dependencies::Dependencies() {}

void Dependencies::addDependency(Dependency* dep) {
    /*
    if (dep->getParam() == NULL) {
        printf("Dep-Adding CO %i, parent: %s\n",dep->getCO()->getID(),dep->getParentID().isNull()?"None":dep->getParentID().toLatin1().data());
    } else {
        printf("Dep-Adding %s, parent: %s\n",dep->getParam()->getID().toLatin1().data(),dep->getParentID().isNull()?"None":dep->getParentID().toLatin1().data());
    }
    */
    if (dep->getParentID().isNull()) {
        top.append(dep);
        rebuildTree();
        return;
    }
    unhandled.append(dep);
    rebuildTree();
}

void Dependencies::propagateValues(Dependency *dep) {
//printf("!!propagate %s\n",((dep != NULL) && (dep->getParam() != NULL))?(dep->getParam()->getID().toLatin1().data()):"CO"); fflush(stdout);
    if (dep == NULL) {
        for (QList<Dependency*>::iterator iter = top.begin(); iter != top.end(); iter++) {
//            printf("p0 (enabled)\n");
            (*iter)->enabled = true;
            propagateValues(*iter);
        }
        return;
    }

    for (QList<Dependency*>::iterator iter = dep->children.begin(); iter != dep->children.end(); iter++) {
        (*iter)->enabled = true;
//        printf("Iteration: %s\n",(*iter)->getParam()?(*iter)->getParam()->getID().toLatin1().data():"");
        if ((*iter)->parentMinValue && (*((*iter)->parentMinValue) > dep->getParam()->getData().toDouble())) {
//            printf("p1 %f > %f %s\n",*((*iter)->parentMinValue),dep->getParam()->getData().toDouble(),dep->getParam()->getData().toLatin1().data());
            (*iter)->enabled = false;
        }
        if ((*iter)->parentMaxValue && (*((*iter)->parentMaxValue) < dep->getParam()->getData().toDouble())) {
//            printf("p2 %f < %f %s\n",*((*iter)->parentMaxValue),dep->getParam()->getData().toDouble(),dep->getParam()->getData().toLatin1().data());
            (*iter)->enabled = false;
        }
        if (!dep->enabled) {
//            printf("p3 disabled %s\n",dep->getParam()->getID().toLatin1().data());
            (*iter)->enabled = false;
        }
//        printf((*iter)->enabled?"p4 (enabled)\n":"p4 (disabled)\n");
        propagateValues(*iter);
    }
}

void depprint(Dependency *dep, uint8_t depth) {
    for (uint8_t n=0; n < 2*depth; n++) {
        printf(" ");
    }
    if (dep->getParam() != NULL) {
        printf(dep->getParam()->getID().toLatin1().data());
    } else {
        printf("CO %i",dep->getCO()->getID());
    }
    printf("\n");
    for (QList<Dependency*>::iterator i = dep->children.begin(); i != dep->children.end(); i++) {
        depprint(*i,depth+1);
    }
}

bool Dependencies::traverseToInsert(Dependency *root, Dependency *leaf) {
    QList<Dependency*> list;

    if (leaf->getParentID().isNull()) {
        return false;
    }

    /*
    if (leaf->getParam() != NULL) {
        printf("DEP: Leaf %s, parent %s\n",leaf->getParam()->getID().toLatin1().data(),leaf->getParentID().toLatin1().data()); fflush(stdout);
    } else {
        printf("DEP: Leaf CO %i, parent %s\n",leaf->getCO()->getID(),leaf->getParentID().toLatin1().data()); fflush(stdout); fflush(stdout);
    }
    */

    if (root == NULL) {
        list = top;
    } else {
        if (root->getParam() == NULL) {
            return false;
        }
        // printf("DEPnode: %s\n",root->getParam()->getID().toLatin1().data()); fflush(stdout);
        list = root->children;
    }
    for (QList<Dependency*>::iterator iter = list.begin(); iter != list.end(); iter++) {
        if ((*iter)->getParam() == NULL) {
            continue;
        }
        // printf("DEPcompare %s\n",(*iter)->getParam()->getID().toLatin1().data()); fflush(stdout);
        if (leaf->getParentID().compare((*iter)->getParam()->getID()) == 0) {
            (*iter)->children.append(leaf);
            return true;
        } else {
            if (traverseToInsert(*iter,leaf)) {
                return true;
            }
        }
    }
    return false;
}

Dependency* Dependencies::findByParam(Dependency *root, Parameter *param) {
    QList<Dependency*> list;

    if (root == NULL) {
        list = top;
    } else {
        if (root->getParam() == param) {
            return root;
        }
        list = root->children;
        if (list.count() == 0) {
            return NULL;
        }
    }

    for (QList<Dependency*>::iterator iter = list.begin(); iter != list.end(); iter++) {
        Dependency *d = findByParam(*iter,param);
        if (d) {
            return d;
        }
    }
    return NULL;
}

Dependency* Dependencies::findByCO(Dependency *root, CommObject *co) {
    QList<Dependency*> list;

    if (root == NULL) {
        list = top;
    } else {
        if (root->getCO() == co) {
            return root;
        }
        list = root->children;
        if (list.count() == 0) {
            return NULL;
        }
    }

    for (QList<Dependency*>::iterator iter = list.begin(); iter != list.end(); iter++) {
        Dependency *d = findByCO(*iter,co);
        if (d) {
            return d;
        }
    }
    return NULL;
}


void Dependencies::rebuildTree() {
    if (unhandled.count() == 0) {
        return;
    }

    uint16_t n = unhandled.count();
    while (n-- > 0) {
        for (QList<Dependency*>::iterator iter = unhandled.begin(); iter != unhandled.end(); iter++) {
            if (traverseToInsert(NULL,*iter)) {
                unhandled.removeOne(*iter);
                break;
            }
        }
    }
    for (QList<Dependency*>::iterator i = top.begin(); i != top.end(); i++) {
        depprint(*i,0);
    }
    fflush(stdout);
}

QList <Dependency*> Dependencies::flatTree(Dependency *root) {
    QList<Dependency*> ret;
    QList<Dependency*> input;

    if (root == NULL) {
        input = top;
    } else {
        input = root->children;
    }
    for (QList<Dependency*>::iterator iter = input.begin(); iter != input.end(); iter++) {
        ret.append(*iter);
        ret.append(flatTree(*iter));
    }
    return ret;
}

bool Dependencies::isEnabled(Parameter *param) {
    Dependency *dep = findByParam(NULL,param);

    if (dep == NULL) {
        return false;
    }
    return dep->enabled;
}

bool Dependencies::isEnabled(CommObject *co) {
    Dependency *dep = findByCO(NULL,co);

    if (dep == NULL) {
        return false;
    }
    return dep->enabled;
}

QDomElement Dependencies::condition(QDomDocument doc,Parameter *param) {
    Dependency *dep = findByParam(NULL,param);
    if ((dep == NULL) || (dep->getParentID().isNull())) {
        return QDomElement();
    }

    QDomElement ret;
    ret = doc.createElement("condition");
    ret.setAttribute("depends-on",dep->getParentID());
    if (dep->parentMinValue && dep->parentMaxValue) {
        if ((*(dep->parentMaxValue)-*(dep->parentMinValue)) < 1e-12) {
            ret.setAttribute("value",(float)(*(dep->parentMinValue)));
        } else {
            ret.setAttribute("min-value",(float)(*(dep->parentMinValue)));
            ret.setAttribute("max-value",(float)(*(dep->parentMaxValue)));
        }
    } else if (dep->parentMinValue != NULL) {
        ret.setAttribute("min-value",(float)(*(dep->parentMinValue)));

    } else if (dep->parentMaxValue != NULL) {
        ret.setAttribute("max-value",(float)(*(dep->parentMaxValue)));
    }
    return ret;
}

QDomElement Dependencies::condition(QDomDocument doc,CommObject *co) {
    Dependency *dep = findByCO(NULL,co);
    if ((dep == NULL) || (dep->getParentID().isNull())) {
        return QDomElement();
    }
    QDomElement ret;
    ret = doc.createElement("condition");
    ret.setAttribute("depends-on",dep->getParentID());

    if (dep->parentMinValue && dep->parentMaxValue) {
        if ((*(dep->parentMaxValue)-*(dep->parentMinValue)) < 1e-12) {
            ret.setAttribute("value",(float)(*(dep->parentMinValue)));
        } else {
            ret.setAttribute("min-value",(float)(*(dep->parentMinValue)));
            ret.setAttribute("max-value",(float)(*(dep->parentMaxValue)));
        }
    } else if (dep->parentMinValue != NULL) {
        ret.setAttribute("min-value",(float)(*(dep->parentMinValue)));

    } else if (dep->parentMaxValue != NULL) {
        ret.setAttribute("max-value",(float)(*(dep->parentMaxValue)));
    }
    return ret;
}
