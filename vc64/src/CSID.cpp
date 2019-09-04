//
// Created by valerino on 23/01/19.
//

#include "CSID.h"

CSID::CSID(CMOS65xx *cpu) {
    _cpu = cpu;
}

CSID::~CSID() {

}

int CSID::update(int cycleCount) {
    return 0;
}
