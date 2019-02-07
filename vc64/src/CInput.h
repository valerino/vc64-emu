//
// Created by valerino on 23/01/19.
//

#ifndef VC64_EMU_CINPUT_H
#define VC64_EMU_CINPUT_H

#include "CCIA1.h"
#include <SDL.h>

#define HOTKEY_DEBUGGER 1

/**
 * handles emulator input
 */
class CInput {
public:
    CInput(CCIA1* cia1);

    ~CInput();

    /**
     * update input status
     * @param ev the input event
     * @param hotkeys on return, signal hotkeys pressed
     * @return
     */
    int update(SDL_Event *ev, uint32_t *hotkeys);

private:
    CCIA1* _cia1;
    uint8_t sdlScancodeToc64Scancode(uint32_t sdlScanCode);
};


#endif //VC64_EMU_CINPUT_H
