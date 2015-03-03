#include "backend.h"

BackEnd::BackEnd() {
    programmer = new Programmer();
    preferences = new QSettings("kalassi","KNXCfg");
}
