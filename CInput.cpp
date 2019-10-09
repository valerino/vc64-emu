//
// Created by valerino on 23/01/19.
//

#include "CInput.h"
#include <SDL.h>
#include <bitutils.h>
CInput::CInput(CCIA1 *cia1, int joyConfiguration) {
    _cia1 = cia1;
    _joyNum = joyConfiguration;
    switch (_joyNum) {
    case 1:
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "joystick port #1 connected!");
        break;
    case 2:
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "joystick port #2 connected!");

        // enable joystick 2 through cia1 hack
        _cia1->enableJoy2Hack(true);
        break;
    default:
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "no joystick connected!");
    }
}

CInput::~CInput() {}

/**
 * @brief populates a queue with events from string on clipboard, when pasting
 * text into the emulator
 * @todo rework the mapping!
 * @todo support tags like {CLR/HOME}, etc....
 * @todo this may be rewritten exploiting the keyboard buffer at 631 as well (as
 * for injecting RUN....)
 * @see @ref sdlScancodeToC64Scancode for information about the mapping
 */
void CInput::fillClipboardQueue() {
    char *txt = SDL_GetClipboardText();
    if (!txt) {
        // clipboard empty
        return;
    }

    // push a keydown/keyup event for each key in the string
    int l = strlen(txt);
#ifdef DEBUG_INPUT
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                 "pushing to keyboard input queue: %s", txt);
#endif
    for (int i = 0; i < l; i++) {
        bool addShift = false;
        // allocate
        SDL_Event *evDown = (SDL_Event *)calloc(1, sizeof(SDL_Event));
        SDL_Event *evUp = (SDL_Event *)calloc(1, sizeof(SDL_Event));

        // build the events
        SDL_Keycode c = tolower(txt[i]);
        SDL_Scancode s;
        switch (c) {
        case SDLK_EQUALS:
            s = SDL_SCANCODE_EQUALS;
            break;

            // return is both 0x0d and 0x0a
        case SDLK_RETURN:
        case 0x0a:
            s = SDL_SCANCODE_RETURN;
            break;

        case SDLK_COLON:
            s = SDL_SCANCODE_SEMICOLON;
            break;

        case SDLK_PLUS:
            s = SDL_SCANCODE_BACKSLASH;
            break;

        case SDLK_DOLLAR:
            s = SDL_SCANCODE_4;
            addShift = true;
            break;

        case SDLK_LEFTPAREN:
            s = SDL_SCANCODE_8;
            addShift = true;
            break;

        case SDLK_RIGHTPAREN:
            s = SDL_SCANCODE_9;
            addShift = true;
            break;

        case SDLK_SEMICOLON:
            s = SDL_SCANCODE_APOSTROPHE;

            break;

        default:
            s = SDL_GetScancodeFromKey(c);
            break;
        }
        //#ifdef DEBUG_INPUT
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                     "key pushed to keyboard input queue: %c", txt[i]);
        //#endif
        if (addShift) {
            // key is shifted
            SDL_Event *evShiftDown = (SDL_Event *)calloc(1, sizeof(SDL_Event));
            evShiftDown->key.keysym.scancode = SDL_SCANCODE_LSHIFT;
            evShiftDown->key.keysym.sym = SDLK_LSHIFT;
            evShiftDown->type = SDL_KEYDOWN;

            // push shift down to the queue
            _kqueue.push(evShiftDown);
        }
        evDown->key.keysym.scancode = s;
        evDown->key.keysym.sym = c;
        evDown->type = SDL_KEYDOWN;
        evUp->key.keysym.scancode = s;
        evUp->key.keysym.sym = c;
        evUp->type = SDL_KEYUP;

        // push to the queue
        _kqueue.push(evDown);
        _kqueue.push(evUp);

        if (addShift) {
            // key is shifted
            SDL_Event *evShiftUp = (SDL_Event *)calloc(1, sizeof(SDL_Event));
            evShiftUp->key.keysym.scancode = SDL_SCANCODE_LSHIFT;
            evShiftUp->key.keysym.sym = SDLK_LSHIFT;
            evShiftUp->type = SDL_KEYUP;

            // push shift down to the queue
            _kqueue.push(evShiftUp);
        }
    }
    SDL_free(txt);
}

/**
 * processes the clipboard queue by injecting the events into the emulated
 * keyboard (useful to type listings!)
 */
void CInput::processClipboardQueue() {
    // pop an event
    SDL_Event *ev = _kqueue.front();
    _kqueue.pop();

    // process event
    processEvent(ev);
    free(ev);
}

/*
 * returns wether the clipboard queue has events to be injected
 */
bool CInput::hasClipboardEvents() {
    int l = _kqueue.size();
#ifdef DEBUG_INPUT
    if (l > 0) {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                     "keyboard input queue size: %d", l);
    }
#endif
    if (l != 0) {
        // has events!
        return true;
    }
    return false;
}

void CInput::injectKeyboardBuffer(const char *chars) {
    // get memory object from cia1
    IMemory *mem = _cia1->_cpu->memory();

    // inject max 10 characters in the keyboard buffer at 631
    // https://www.c64-wiki.com/wiki/631-640
    // thanks to flavioweb/hokutoforce for the trick!
    // https://www.youtube.com/watch?v=EHuyVeC6gFw
    int written = 0;
    int len = strlen(chars);
    if (len > 10) {
        // avoid overflow
        len = 10;
    }

    // write chars
    for (int i = 0; i < len; i++) {
        mem->writeByte(631 + i, chars[i]);
        written++;
    }
    // number of entries goes at location 198
    mem->writeByte(198, written);
}

void CInput::checkClipboard(int64_t totalCycles, int cyclesPerFrame,
                            int frameSkip) {
    int toSkip = cyclesPerFrame * frameSkip;
    if (_prevTotalCycles == 0) {
        // init
        _prevTotalCycles = totalCycles;
    }
    int diff = totalCycles - _prevTotalCycles;

    // every 4 frames, check the clipboard
    if (diff > toSkip) {
        // check the clipboard queue, and process one event if not empty
        if (hasClipboardEvents()) {
            processClipboardQueue();
        }
        _prevTotalCycles = totalCycles;
    }
}

/**
 * @brief simulate joysticks via keypresses
 * (https://www.c64-wiki.com/wiki/Joystick)
 * @param sdlScanCode the pressed key, SDL side
 * @param pressed true if pressed, false if released
 * @return true if handled
 */
bool CInput::handleJoystick(uint32_t sdlScanCode, bool pressed) {
    uint8_t c64ScanCode = 0;
    uint8_t pra = 0;
    switch (sdlScanCode) {
    case SDL_SCANCODE_LEFT:
        // ctrl
        c64ScanCode = 0x3a;
        _cia1->setKeyState(c64ScanCode, pressed);
        return true;

    case SDL_SCANCODE_RIGHT:
        // 2
        c64ScanCode = 0x3b;
        _cia1->setKeyState(c64ScanCode, pressed);
        return true;

    case SDL_SCANCODE_UP:
        // 1
        c64ScanCode = 0x38;
        _cia1->setKeyState(c64ScanCode, pressed);
        return true;

    case SDL_SCANCODE_DOWN:
        // <-
        c64ScanCode = 0x39;
        _cia1->setKeyState(c64ScanCode, pressed);
        return true;

    case SDL_SCANCODE_LSHIFT:
        // fire button
        // space
        c64ScanCode = 0x3c;
        _cia1->setKeyState(c64ScanCode, pressed);
        return true;
    }

    return false;
}

/**
 * process an event, used also to process injected keyboard events from
 * clipboard
 * @param ev the event
 */
void CInput::processEvent(SDL_Event *ev) {
#ifdef DEBUG_INPUT
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "scancode: %d, key: %s, type=%s",
                 ev->key.keysym.scancode, SDL_GetKeyName(ev->key.keysym.sym),
                 ev->type == SDL_KEYDOWN ? "keydown" : "keyup");
#endif

    // convert to c64 scancode
    SDL_Scancode sds = ev->key.keysym.scancode;
    bool pressed = (ev->type == SDL_KEYDOWN);

    // handle joystick 1/2 presses
    bool isJoystick = false;
    if (_joyNum != 0) {
        isJoystick = handleJoystick(sds, pressed);
    }
    if (!isJoystick) {
        // handle keyboard, if no joystick
        uint8_t scancode = sdlScancodeToC64Scancode(sds);
        if (scancode != 0xff) {
            // update internal keyboard state
            _cia1->setKeyState(scancode, pressed);
        }
    }
}

int CInput::update(SDL_Event *ev, uint32_t *hotkeys) {
    // we have a keyup or keydown
    const uint8_t *keys = SDL_GetKeyboardState(nullptr);

    // check hotkeys
    if (keys[SDL_SCANCODE_LCTRL] && keys[SDL_SCANCODE_D]) {
        // break requested!
        *hotkeys = HOTKEY_DEBUGGER;
        return 0;
    } else if (keys[SDL_SCANCODE_TAB] && keys[SDL_SCANCODE_BACKSPACE]) {
        // runstop + restore causes a nonmaskable interrupt
        _cia1->_cpu->nmi();
        return 0;
    } else if (keys[SDL_SCANCODE_LCTRL] && keys[SDL_SCANCODE_V]) {
        // handle clipboard copying keystrokes to the input queue
        *hotkeys = HOTKEY_PASTE_TEXT;
        return 0;
    } else if (keys[SDL_SCANCODE_LCTRL] && keys[SDL_SCANCODE_ESCAPE]) {
        // force exit
        *hotkeys = HOTKEY_FORCE_EXIT;
        return 0;
    }

    // process event
    processEvent(ev);
    return 0;
}

/**
 * maps sdl keycode to c64 scancode
 * @note https://www.c64-wiki.com/wiki/Keyboard_code\n
 *  https://sites.google.com/site/h2obsession/CBM/C128/keyboard-scan
 * @param sdlScanCode the sdl scancode
 * @todo this should be reworked using the system layout, actually maps the
 * C64 keyboard roughly to the italian keyboard. also, shifted keys like
 * cursors should be handled internally
 * @return the c64 scancode
 */
uint8_t CInput::sdlScancodeToC64Scancode(uint32_t sdlScanCode) {
    /**
     * custom mapping
     * TODO: support customizing
     * INST/DL : backspace
     * TAB: runstop
     *
     */
    switch (sdlScanCode) {
    case SDL_SCANCODE_BACKSPACE:
        // ins/del
        return 0x0;
    case SDL_SCANCODE_RETURN:
        return 0x1;
    case SDL_SCANCODE_LEFT:
        // cursor left/right
        // joy 1 left sets 0x3a(key 'CTRL') in PRB, either return 0x2
        // (cursor left/right) when no joy1 is connected
        return 0x2;

    case SDL_SCANCODE_RIGHT:
        // cursor left/right
        // joy 1 left sets 0x3b(key '2') in PRB, either return 0x2 (cursor
        // left/right) when no joy1 is connected
        return 0x2;

    case SDL_SCANCODE_F7:
        // f7/f8
        return 0x3;
    case SDL_SCANCODE_F8:
        // f7/f8
        return 0x3;
    case SDL_SCANCODE_F1:
        // f1/f2
        return 0x4;
    case SDL_SCANCODE_F2:
        // f1/f2
        return 0x4;
    case SDL_SCANCODE_F3:
        // f3/f4
        return 0x5;
    case SDL_SCANCODE_F4:
        // f3/f4
        return 0x5;
    case SDL_SCANCODE_F5:
        // f5/f6
        return 0x6;
    case SDL_SCANCODE_F6:
        // f5/f6
        return 0x6;
    case SDL_SCANCODE_UP:
        // cursor up/down
        // joy 1 up sets 0x38(key '1') in PRB, either return 0x7 (cursor
        // up/down) when no joy1 is connected
        return 0x7;
    case SDL_SCANCODE_DOWN:
        // cursor up/down
        // joy 1 up sets 0x39(key '<-') in PRB, either return 0x7 (cursor
        // up/down) when no joy1 is connected
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
    case SDL_SCANCODE_BACKSLASH: /**< italian keyboard: ù -> + */
        return 0x28;
    case SDL_SCANCODE_P:
        return 0x29;
    case SDL_SCANCODE_L:
        return 0x2a;
    case SDL_SCANCODE_MINUS: /**< italian keyboard: ' -> - */
        return 0x2b;
    case SDL_SCANCODE_PERIOD: /**< italian keyboard: - -> / */
        return 0x2c;
    case SDL_SCANCODE_SEMICOLON: /**< italian keyboard: ò -> : */
        return 0x2d;
    case SDL_SCANCODE_NONUSBACKSLASH: /**< italian keyboard: < ->  @ */
        return 0x2e;
    case SDL_SCANCODE_COMMA:
        return 0x2f;
    case SDL_SCANCODE_GRAVE:
        return 0x30;               /**< italian keyboard: \ -> £ */
    case SDL_SCANCODE_LEFTBRACKET: /**< italian keyboard: è -> * */
        return 0x31;
    case SDL_SCANCODE_APOSTROPHE: /**< italian keyboard: à -> ; */
        return 0x32;
    case SDL_SCANCODE_HOME:
        // clr/home
        return 0x33;
    case SDL_SCANCODE_RSHIFT:
        return 0x34;
    case SDL_SCANCODE_EQUALS: /**< italian keyboard: ì -> = */
        return 0x35;
    case SDL_SCANCODE_RIGHTBRACKET: /**< italian keyboard: + -> (graphical
                                       up arrow) */
        return 0x36;
    case SDL_SCANCODE_SLASH:
        return 0x37;
    case SDL_SCANCODE_1:
        return 0x38;
    case SDL_SCANCODE_ESCAPE:
        return 0x39; /**< italian keyboard: esc -> (graphical left arrow) */
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
        return 0x3a;
    case SDL_SCANCODE_2:
        return 0x3b;
    case SDL_SCANCODE_SPACE:
        // space
        // joy 1 up sets 0x3c(key 'SPACEBAR') in PRB, either return 0x7
        // (cursor up/down) when no joy1 is connected
        return 0x3c;
    case SDL_SCANCODE_LALT:
        // commodore
        return 0x3d;
    case SDL_SCANCODE_Q:
        return 0x3e;
    case SDL_SCANCODE_TAB:
        // run-stop
        return 0x3f;
    default:
        break;
    }

    // invalid
    return 0xff;
}
