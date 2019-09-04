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

int CSDLUtils::initializeDisplayContext(SDLDisplayCtx *ctx,
                                        SDLDisplayCreateOptions *options,
                                        char **errorString) {
    if (!options || !ctx || !errorString) {
        return EINVAL;
    }
    memset(ctx, 0, sizeof(SDLDisplayCtx));
    *errorString = nullptr;
    bool ok = false;
    do {
        if (options->scaleFactor == 0) {
            options->scaleFactor = 1;
        }
        ctx->window =
            SDL_CreateWindow(options->windowName, options->posX, options->posY,
                             options->w * options->scaleFactor, options->h * options->scaleFactor, options->windowFlags);
        if (!ctx->window) {
            break;
        }        
        #ifdef __APPLE__
            // set this, or scaling do not work!
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal"); 
        #endif
        ctx->renderer =
            SDL_CreateRenderer(ctx->window, -1, options->rendererFlags);
        if (!ctx->renderer) {
            break;
        }
        ctx->texture = SDL_CreateTexture(
            ctx->renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, options->w, options->h);
        if (!ctx->texture) {
            break;
        }
        ctx->pxFormat = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
        if (!ctx->pxFormat) {
            break;
        }
        ok = true;
    } while (0);

    if (!ok) {
        // some error happened
        *errorString = (char *)SDL_GetError();
        CSDLUtils::finalizeDisplayContext(ctx);
        return ECANCELED;
    }
    return 0;
}

void CSDLUtils::finalizeDisplayContext(SDLDisplayCtx *ctx) {
    if (ctx->pxFormat) {
        SDL_FreeFormat(ctx->pxFormat);
        ctx->pxFormat = nullptr;
    }
    if (ctx->texture) {
        SDL_DestroyTexture(ctx->texture);
        ctx->texture = nullptr;
    }
    if (ctx->renderer) {
        SDL_DestroyRenderer(ctx->renderer);
        ctx->renderer = nullptr;
    }
    if (ctx->window) {
        SDL_DestroyWindow(ctx->window);
        ctx->window = nullptr;
    }
}

int CSDLUtils::update(SDLDisplayCtx *ctx, void *texture, int w) {
    if (!ctx || !texture || !w) {
        return EINVAL;
    }
    SDL_UpdateTexture(ctx->texture, NULL, texture, w * sizeof(uint32_t));
    SDL_RenderClear(ctx->renderer);
    SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
    SDL_RenderPresent(ctx->renderer);
    return 0;
}
