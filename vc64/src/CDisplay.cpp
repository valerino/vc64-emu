//
// Created by valerino on 10/01/19.
//

#include "CDisplay.h"

CDisplay* CDisplay::_instance = nullptr;

CDisplay::CDisplay() {
    _renderer = nullptr;
    _window = nullptr;
}

bool CDisplay::init() {
    SDL_Renderer* r;
    SDL_Window* w;
    w = SDL_CreateWindow("vc64", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    if (!w) {
        return false;
    }
    r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);
    if (!r) {
        SDL_DestroyWindow(w);
        return false;
    }
    SDL_SetRenderDrawColor(r, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(_renderer);
    SDL_RenderPresent(_renderer);
    _renderer = r;
    _window = w;
    return true;
}

CDisplay *CDisplay::instance() {
    if (_instance == nullptr) {
        _instance = new CDisplay();
    }
    return _instance;
}

CDisplay::~CDisplay() {
    if (_renderer) {
        SDL_DestroyRenderer(_renderer);
    }
    if (_window) {
        SDL_DestroyWindow(_window);
    }
    delete this;
}
