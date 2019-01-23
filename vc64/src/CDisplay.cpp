//
// Created by valerino on 10/01/19.
//

#include "CDisplay.h"

CDisplay::CDisplay() {
}

int CDisplay::init(SDLDisplayCreateOptions* options, char** errorString) {
    // init display
    memset(_ctx,0,sizeof(SDLDisplayCtx));
    int res = CSDLUtils::initializeDisplayContext(_ctx, options, errorString);
    if (res != 0) {
        return res;
    }

    // clear
    res = CSDLUtils::clearDisplay(_ctx, errorString);
    if (res != 0) {
        CSDLUtils::finalizeDisplayContext(_ctx);
        return res;
    }

    // show!
    CSDLUtils::show(_ctx);
    return 0;
}

CDisplay::~CDisplay() {
    CSDLUtils::finalizeDisplayContext(_ctx);
}

int CDisplay::update(uint8_t*frameBuffer, uint32_t size) {
    return 0;
}
