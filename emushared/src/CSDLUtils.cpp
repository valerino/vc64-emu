//
// Created by valerino on 11/01/19.
//

#include "CSDLUtils.h"
#include <SDL.h>
#include <errno.h>

int CSDLUtils::pollEvents(uint8_t **keys, bool* mustExit) {
    if (!keys || !mustExit) {
        return EINVAL;
    }
    *keys = NULL;
    *mustExit = false;
    // run an SDL poll cycle
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            // SDL window closed, application must quit asap
            *mustExit = true;
        } else if (ev.type == SDL_KEYDOWN) {
            // save state
            uint8_t *state = const_cast<uint8_t *>(SDL_GetKeyboardState(NULL));
            *keys = state;
        }
    }
    return 0;
}
