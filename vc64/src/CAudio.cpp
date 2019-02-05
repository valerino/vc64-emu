//
// Created by valerino on 23/01/19.
//

#include "CAudio.h"

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_AUDIO
#endif

CAudio::~CAudio() {

}

void CAudio::update() {
}

CAudio::CAudio(CSID *sid) {
    _sid = sid;
}
