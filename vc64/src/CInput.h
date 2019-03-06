//
// Created by valerino on 23/01/19.
//

#ifndef VC64_EMU_CINPUT_H
#define VC64_EMU_CINPUT_H

#include "CCIA1.h"
#include <SDL.h>
#include <queue>

/**
 * @brief pressing ctrl-d enters the debugger, softice style :)
 */
#define HOTKEY_DEBUGGER 1

/**
 * @brief pressing ctrl-v pastes text into the emulator
 */
#define HOTKEY_PASTE_TEXT 2

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
     * processes the clipboard queue by injecting the events into the emulated keyboard (useful to type listings!)
     */
    void processClipboardQueue();

    /*
     * returns wether the clipboard queue has events to be injected
     */
    bool hasClipboardEvents();

    /**
     * fills the clipboard queue with fake keyboard events, in response to HOTKEY_PASTE_TEXT
     */
    void fillClipboardQueue();

private:
    CCIA1* _cia1;
    std::queue<SDL_Event*> _kqueue;
    uint8_t sdlScancodeToc64Scancode(uint32_t sdlScanCode);
    void processEvent(SDL_Event* ev);

};


#endif //VC64_EMU_CINPUT_H
