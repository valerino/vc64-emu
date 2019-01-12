//
// Created by valerino on 11/01/19.
//

#include "CSDLUtils.h"
#include <SDL.h>
#include <errno.h>
#include <include/CSDLUtils.h>


int CSDLUtils::pollEvents(uint8_t **keys, bool* mustExit) {
    if (!keys || !mustExit) {
        return EINVAL;
    }
    *keys = nullptr;
    *mustExit = false;

    // run an SDL poll cycle
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            // SDL window closed, application must quit asap
            *mustExit = true;
        } else if (ev.type == SDL_KEYDOWN) {
            // save state
            uint8_t *state = const_cast<uint8_t *>(SDL_GetKeyboardState(nullptr));
            *keys = state;
        }
    }
    return 0;
}

int CSDLUtils::initializeDisplayContext(SDLDisplayCtx *ctx, SDLDisplayCreateOptions *options, char **errorString) {
    if (!options || !ctx || !errorString) {
        return EINVAL;
    }
    *errorString = nullptr;
    SDL_Renderer* r;
    SDL_Window* w;
    w = SDL_CreateWindow(options->windowName, options->posX, options->posY, options->w, options->h, options->windowFlags);
    if (!w) {
        *errorString = (char*)SDL_GetError();
        return ECANCELED;
    }
    r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);
    if (!r) {
        *errorString = (char*)SDL_GetError();
        SDL_DestroyWindow(w);
        return ECANCELED;
    }
    ctx->renderer = r;
    ctx->window = w;
    ctx->initialized = true;
    return 0;
}

void CSDLUtils::finalizeDisplayContext(SDLDisplayCtx *ctx) {
    if (ctx->initialized) {
        if (ctx->renderer) {
            SDL_DestroyRenderer(ctx->renderer);
        }
        if (ctx->window) {
            SDL_DestroyWindow(ctx->window);
        }
        ctx->initialized = false;
    }
}

int CSDLUtils::show(SDLDisplayCtx *ctx) {
    if (!ctx) {
        return EINVAL;
    }
    SDL_RenderPresent(ctx->renderer);
    return 0;
}

int CSDLUtils::clearDisplay(SDLDisplayCtx *ctx, char **errorString) {
    if (!ctx || !errorString) {
        return EINVAL;
    }
    *errorString = nullptr;
    int res = SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    if (res == -1) {
        *errorString = (char*)SDL_GetError();
        return ECANCELED;
    }
    res = SDL_RenderClear(ctx->renderer);
    if (res == -1) {
        *errorString = (char*)SDL_GetError();
        return ECANCELED;
    }
    return 0;
}
