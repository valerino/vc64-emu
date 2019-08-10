//
// Created by valerino on 10/01/19.
//

#include "CDisplay.h"
#include <CBuffer.h>
#include <stdexcept>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_DISPLAY
#endif

CDisplay::CDisplay(CVICII *vic, const char* wndName, bool fullScreen) {
    // allocate the texture memory for the framebuffer
    _fb = (uint32_t *)calloc(1, (VIC_PAL_SCREEN_W * VIC_PAL_SCREEN_H) * sizeof(uint32_t));
    _vic = vic;

    // initialize display
    SDLDisplayCreateOptions displayOpts = {};
    displayOpts.posX = SDL_WINDOWPOS_CENTERED;
    displayOpts.posY = SDL_WINDOWPOS_CENTERED;
    displayOpts.scaleFactor = 2;
    displayOpts.w = VIC_PAL_SCREEN_W;
    displayOpts.h = VIC_PAL_SCREEN_H;
    displayOpts.windowName = wndName;
    displayOpts.windowFlags = fullScreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE;
    displayOpts.rendererFlags = SDL_RENDERER_ACCELERATED;
    char *sdlError;
    int res = CSDLUtils::initializeDisplayContext(&_ctx, &displayOpts, &sdlError);
    if (res != 0) {
        throw std::runtime_error(std::string(sdlError));
    }
    _vic->setSdlCtx(&_ctx, _fb);

    // show!
    update();
}

CDisplay::~CDisplay() {
    CSDLUtils::finalizeDisplayContext(&_ctx);
    SAFE_FREE(_fb)
}

void CDisplay::update() {
    // update the framebuffer
    CSDLUtils::update(&_ctx, _fb, VIC_PAL_SCREEN_W);
}
