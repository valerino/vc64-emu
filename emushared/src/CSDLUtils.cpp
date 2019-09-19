//
// Created by valerino on 11/01/19.
//

#include "CSDLUtils.h"
#include "CLog.h"
#include <SDL.h>
#include <errno.h>

void CSDLUtils::pollEvents(SDLEventCallback cb) {
    // step an SDL poll cycle
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        cb(&ev);
    }
}
