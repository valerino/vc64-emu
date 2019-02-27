//
// Created by valerino on 23/01/19.
//

#ifndef VC64_EMU_CINPUT_H
#define VC64_EMU_CINPUT_H

#include "CCIA1.h"
#include <SDL.h>
#include <queue>

/**
 * pressing ctrl-d enters the debugger !
 */
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
     * @return 0
     */
    int update(SDL_Event *ev, uint32_t *hotkeys);

    /**
     * check the queue holding clipboard events, and if not empty processes it (injects fake keyboard events)
     */
    void checkClipboardQueue();

private:
    CCIA1* _cia1;
    std::queue<SDL_Event*> _kqueue;
    uint8_t sdlScancodeToc64Scancode(uint32_t sdlScanCode);
    void handleClipboard();
    void processEvent(SDL_Event* ev);

};


#endif //VC64_EMU_CINPUT_H
