//
// Created by valerino on 10/01/19.
//

#include "CDisplay.h"
#include <CBuffer.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_DISPLAY
#endif

CDisplay::CDisplay(CVICII* vic) {
    // allocate the texture memory for the framebuffer
    _fb = (uint32_t*)calloc(1, VIC_PAL_SCREEN_W * VIC_PAL_SCREEN_H * sizeof(uint32_t));
    _vic = vic;

}

int CDisplay::init(SDLDisplayCreateOptions* options, char** errorString) {

    // init display
    int res = CSDLUtils::initializeDisplayContext(&_ctx, options, errorString);
    if (res != 0) {
        return res;
    }
    _vic->setSdlCtx(&_ctx, _fb);

    // show!
    CSDLUtils::update(&_ctx, (void*)_fb, VIC_PAL_SCREEN_W);
    return 0;
}

CDisplay::~CDisplay() {
    CSDLUtils::finalizeDisplayContext(&_ctx);
    SAFE_FREE(_fb)
}

void CDisplay::update() {
    // tell the vic to update the framebuffer
    _vic->updateScreen();

}
