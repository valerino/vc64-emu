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

    // update internal keyboard state
    uint8_t scancode = sdlKeycodeToc64Scancode(event->key.keysym.sym);
    _cia1->setKeyState(scancode, event->type == SDL_KEYDOWN ? true : false);
}

/**
 * maps sdl keycode to c64 scancode
 * https://www.c64-wiki.com/wiki/Keyboard_code
 * @param sdlKeyCode
 * @return
 */
uint8_t CInput::sdlKeycodeToc64Scancode(uint32_t sdlKeyCode) {
    switch (sdlKeyCode) {
        case SDLK_INSERT:
            return 0x0;
        case SDLK_RETURN:
            return 0x1;
        case SDLK_LEFT:
            return 0x2;
        case SDLK_RIGHT:
            return 0x2;
        case SDLK_F7:
            return 0x3;
        case SDLK_F8:
            return 0x3;
        case SDLK_F1:
            return 0x4;
        case SDLK_F2:
            return 0x4;
        case SDLK_F3:
            return 0x5;
        case SDLK_F4:
            return 0x5;
        case SDLK_F5:
            return 0x6;
        case SDLK_F6:
            return 0x6;
        case SDLK_UP:
            return 0x7;
        case SDLK_DOWN:
            return 0x7;
        case SDLK_3:
            return 0x8;
        case SDLK_w:
            return 0x9;
        case SDLK_a:
            return 0xa;
        case SDLK_4:
            return 0xb;
        case SDLK_z:
            return 0xc;
        case SDLK_s:
            return 0xd;
        case SDLK_e:
            return 0xe;
        case SDLK_LSHIFT:
            return 0xf;
        case SDLK_5:
            return 0x10;
        case SDLK_r:
            return 0x11;
        case SDLK_d:
            return 0x12;
        case SDLK_6:
            return 0x13;
        case SDLK_c:
            return 0x14;
        case SDLK_f:
            return 0x15;
        case SDLK_t:
            return 0x16;
        case SDLK_x:
            return 0x17;
        case SDLK_7:
            return 0x18;
        case SDLK_y:
            return 0x19;
        case SDLK_g:
            return 0x1a;
        case SDLK_8:
            return 0x1b;
        case SDLK_b:
            return 0x1c;
        case SDLK_h:
            return 0x1d;
        case SDLK_u:
            return 0x1e;
        case SDLK_v:
            return 0x1f;
        case SDLK_9:
            return 0x20;
        case SDLK_i:
            return 0x21;
        case SDLK_j:
            return 0x22;
        case SDLK_0:
            return 0x23;
        case SDLK_m:
            return 0x24;
        case SDLK_k:
            return 0x25;
        case SDLK_o:
            return 0x26;
        case SDLK_n:
            return 0x27;
        case SDLK_PLUS:
            return 0x28;
        case SDLK_p:
            return 0x29;
        case SDLK_l:
            return 0x2a;
        case SDLK_MINUS:
            return 0x2b;
        case SDLK_GREATER:
            return 0x2c;
        case SDLK_LEFTBRACKET:
            return 0x2d;
        case SDLK_AT:
            return 0x2e;
        case SDLK_LESS:
            return 0x2f;
        case SDLK_CURRENCYUNIT:
            return 0x30;
        case SDLK_ASTERISK:
            return 0x31;
        case SDLK_RIGHTBRACKET:
            return 0x32;
        case SDLK_HOME:
            return 0x33;
        case SDLK_RSHIFT:
            return 0x34;
        case SDLK_EQUALS:
            return 0x35;
        case SDLK_EXCLAIM:
            return 0x36;
        case SDLK_QUESTION:
            return 0x37;
        case SDLK_1:
            return 0x38;
        case SDLK_ESCAPE:
            return 0x39;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            return 0x3a;
        case SDLK_2:
            return 0x3b;
        case SDLK_SPACE:
            return 0x3c;
        case SDLK_LALT:
            // TODO: commodore, allow configure mapping for special keys
            return 0x3d;
        case SDLK_q:
            return 0x3e;
        case SDLK_TAB:
            // TODO: runstop, allow configure mapping for special keys
            return 0x3f;
        default:
            break;
    }
    return 0;
}
