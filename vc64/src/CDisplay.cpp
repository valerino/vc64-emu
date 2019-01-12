//
// Created by valerino on 10/01/19.
//

#include "CDisplay.h"

CDisplay* CDisplay::_instance = nullptr;

CDisplay::CDisplay() {
    memset(_ctx,0,sizeof(SDLDisplayCtx));
}

int CDisplay::init(SDLDisplayCreateOptions* options, char** errorString) {
    // init display
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

CDisplay *CDisplay::instance() {
    if (_instance == nullptr) {
        _instance = new CDisplay();
    }
    return _instance;
}

CDisplay::~CDisplay() {
    CSDLUtils::finalizeDisplayContext(_ctx);
    delete this;
}

int CDisplay::update(uint8_t*frameBuffer, uint32_t size) {
    return 0;
}
