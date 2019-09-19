//
// Created by valerino on 10/01/19.
//

#include "CDisplay.h"
#include <CBuffer.h>
#include <stdexcept>
#include <string>
#include <stdio.h>
#include <stdlib.h>

int CDisplay::initializeDisplay(bool fullScreen, const char *wndName,
                                char **errorString) {
    if (!wndName || !errorString) {
        return EINVAL;
    }
    *errorString = nullptr;
    bool ok = false;
    do {
        // set the display windows big as twice as the real emulated display
        int scaleFactor = 2;
        _window = SDL_CreateWindow(
            wndName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            VIC_PAL_SCREEN_W * scaleFactor, VIC_PAL_SCREEN_H * scaleFactor,
            fullScreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE);
        if (!_window) {
            break;
        }
#ifdef __APPLE__
        // set this, or scaling do not work!
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
#endif
        _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
        if (!_renderer) {
            break;
        }
        _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     VIC_PAL_SCREEN_W, VIC_PAL_SCREEN_H);
        if (!_texture) {
            break;
        }
        _pxFormat = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
        if (!_pxFormat) {
            break;
        }
        ok = true;
    } while (0);

    if (!ok) {
        // some error happened
        *errorString = (char *)SDL_GetError();
        return ECANCELED;
    }
    return 0;
}

CDisplay::CDisplay(CVICII *vic, const char *wndName, bool fullScreen) {
    // allocate the texture memory for the framebuffer
    _fb = (uint32_t *)calloc(1, (VIC_PAL_SCREEN_W * VIC_PAL_SCREEN_H) *
                                    sizeof(uint32_t));
    _vic = vic;

    char *sdlError;
    int res = initializeDisplay(fullScreen, wndName, &sdlError);
    if (res != 0) {
        throw std::runtime_error(std::string(sdlError));
    }

    // set callbacks
    vic->setBlitCallback(this, CDisplay::blitCallback);

    // show!
    update();
}

CDisplay::~CDisplay() {
    if (_pxFormat) {
        SDL_FreeFormat(_pxFormat);
    }
    if (_texture) {
        SDL_DestroyTexture(_texture);
    }
    if (_renderer) {
        SDL_DestroyRenderer(_renderer);
    }
    if (_window) {
        SDL_DestroyWindow(_window);
    }
    SAFE_FREE(_fb)
}

void CDisplay::update() {
    // update the framebuffer
    SDL_UpdateTexture(_texture, NULL, _fb, VIC_PAL_SCREEN_W * sizeof(uint32_t));
    SDL_RenderClear(_renderer);
    SDL_RenderCopy(_renderer, _texture, NULL, NULL);
    SDL_RenderPresent(_renderer);
}

void CDisplay::blitCallback(void *thisPtr, rgbStruct *rgb, int pos) {
    CDisplay *d = (CDisplay *)thisPtr;
    d->_fb[pos] = SDL_MapRGB(d->_pxFormat, rgb->r, rgb->g, rgb->b);
}