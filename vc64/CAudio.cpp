//
// Created by valerino on 23/01/19.
//

#include "CAudio.h"
#include <SDL.h>

CAudio::~CAudio() {}

int CAudio::update() { return 0; }

CAudio::CAudio(CSID *sid) { _sid = sid; }
