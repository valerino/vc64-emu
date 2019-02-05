//
// Created by valerino on 23/01/19.
//

#include "CInput.h"
#include <CLog.h>

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_INPUT
#endif


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
    if (keys[SDL_SCANCODE_TAB] && keys[SDL_SCANCODE_PAGEUP]) {
        // runstop + restore causes a nonmaskable interrupt
        _cia1->_cpu->nmi();
        return;
    }

    // update internal keyboard state
#ifdef DEBUG_INPUT
    CLog::print("scancode: %d",  event->key.keysym.scancode);
#endif
    uint8_t scancode = sdlScancodeToc64Scancode(event->key.keysym.scancode);
    if (scancode != 0xff) {
        _cia1->setKeyState(scancode, event->type == SDL_KEYDOWN ? true : false);
    }
}

/**
 * maps sdl keycode to c64 scancode
 * https://www.c64-wiki.com/wiki/Keyboard_code
 * https://sites.google.com/site/h2obsession/CBM/C128/keyboard-scan
 * @param sdlScanCode
 * @return
 */
uint8_t CInput::sdlScancodeToc64Scancode(uint32_t sdlScanCode){
    /**
     * custom mapping
     * TODO: support customizing
     * INST/DL : backspace
     * TAB: runstop
     *
     */
    switch (sdlScanCode) {
        case SDL_SCANCODE_BACKSPACE:
            return 0x0;
        case SDL_SCANCODE_RETURN:
            return 0x1;
        case SDL_SCANCODE_LEFT:
            return 0x2;
        case SDL_SCANCODE_RIGHT:
            return 0x2;
        case SDL_SCANCODE_F7:
            return 0x3;
        case SDL_SCANCODE_F8:
            return 0x3;
        case SDL_SCANCODE_F1:
            return 0x4;
        case SDL_SCANCODE_F2:
            return 0x4;
        case SDL_SCANCODE_F3:
            return 0x5;
        case SDL_SCANCODE_F4:
            return 0x5;
        case SDL_SCANCODE_F5:
            return 0x6;
        case SDL_SCANCODE_F6:
            return 0x6;
        case SDL_SCANCODE_UP:
            return 0x7;
        case SDL_SCANCODE_DOWN:
            return 0x7;
        case SDL_SCANCODE_3:
            return 0x8;
        case SDL_SCANCODE_W:
            return 0x9;
        case SDL_SCANCODE_A:
            return 0xa;
        case SDL_SCANCODE_4:
            return 0xb;
        case SDL_SCANCODE_Z:
            return 0xc;
        case SDL_SCANCODE_S:
            return 0xd;
        case SDL_SCANCODE_E:
            return 0xe;
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_CAPSLOCK:
            return 0xf;
        case SDL_SCANCODE_5:
            return 0x10;
        case SDL_SCANCODE_R:
            return 0x11;
        case SDL_SCANCODE_D:
            return 0x12;
        case SDL_SCANCODE_6:
            return 0x13;
        case SDL_SCANCODE_C:
            return 0x14;
        case SDL_SCANCODE_F:
            return 0x15;
        case SDL_SCANCODE_T:
            return 0x16;
        case SDL_SCANCODE_X:
            return 0x17;
        case SDL_SCANCODE_7:
            return 0x18;
        case SDL_SCANCODE_Y:
            return 0x19;
        case SDL_SCANCODE_G:
            return 0x1a;
        case SDL_SCANCODE_8:
            return 0x1b;
        case SDL_SCANCODE_B:
            return 0x1c;
        case SDL_SCANCODE_H:
            return 0x1d;
        case SDL_SCANCODE_U:
            return 0x1e;
        case SDL_SCANCODE_V:
            return 0x1f;
        case SDL_SCANCODE_9:
            return 0x20;
        case SDL_SCANCODE_I:
            return 0x21;
        case SDL_SCANCODE_J:
            return 0x22;
        case SDL_SCANCODE_0:
            return 0x23;
        case SDL_SCANCODE_M:
            return 0x24;
        case SDL_SCANCODE_K:
            return 0x25;
        case SDL_SCANCODE_O:
            return 0x26;
        case SDL_SCANCODE_N:
            return 0x27;
        case SDL_SCANCODE_BACKSLASH:
            // 'ù' on IT keyboard, returns '+'
            return 0x28;
        case SDL_SCANCODE_P:
            return 0x29;
        case SDL_SCANCODE_L:
            return 0x2a;
        case SDL_SCANCODE_MINUS:
            return 0x2b;
        case SDL_SCANCODE_PERIOD:
            return 0x2c;
        case SDL_SCANCODE_SEMICOLON:
            return 0x2d;
        case SDL_SCANCODE_NONUSBACKSLASH:
            // '<' on IT keyboard, returns '@'
            return 0x2e;
        case SDL_SCANCODE_COMMA:
            return 0x2f;
        case SDL_SCANCODE_GRAVE:
            return 0x30;
        case SDL_SCANCODE_LEFTBRACKET:
            return 0x31;
        case SDL_SCANCODE_APOSTROPHE:
            return 0x32;
        case SDL_SCANCODE_HOME:
            return 0x33;
        case SDL_SCANCODE_RSHIFT:
            return 0x34;
        case SDL_SCANCODE_EQUALS:
            return 0x35;
        case SDL_SCANCODE_RIGHTBRACKET:
            return 0x36;
        case SDL_SCANCODE_SLASH:
            return 0x37;
        case SDL_SCANCODE_1:
            return 0x38;
        case SDL_SCANCODE_ESCAPE:
            return 0x39;
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
            return 0x3a;
        case SDL_SCANCODE_2:
            return 0x3b;
        case SDL_SCANCODE_SPACE:
            return 0x3c;
        case SDL_SCANCODE_LALT:
            return 0x3d;
        case SDL_SCANCODE_Q:
            return 0x3e;
        case SDL_SCANCODE_TAB:
            return 0x3f;
        default:
            break;
    }

    // invalid
    return 0xff;
}
