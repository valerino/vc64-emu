//
// Created by valerino on 23/01/19.
//

#include "CInput.h"

CInput::CInput(CCIA1* cia1) {
    _cia1 = cia1;
}

CInput::~CInput() {

}

void CInput::update(SDL_Event* event, uint32_t* hotkeys) {
    // we have a keyup or keydown
    const uint8_t* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_LCTRL] && keys[SDL_SCANCODE_D]) {
        // break requested!
        *hotkeys = HOTKEY_DEBUGGER;
        return;
    }
}
