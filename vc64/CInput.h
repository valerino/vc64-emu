//
// Created by valerino on 23/01/19.
//

#ifndef VC64_EMU_CINPUT_H
#define VC64_EMU_CINPUT_H

#include "CCIA1.h"
#include <SDL.h>
#include <queue>

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_INPUT
#endif

/**
 * @brief pressing ctrl-d enters the debugger, softice style :)
 */
#define HOTKEY_DEBUGGER 1

/**
 * @brief pressing ctrl-v pastes text into the emulator
 */
#define HOTKEY_PASTE_TEXT 2

/**
 * @brief pressing ctrl-esc (force) exit the emulator
 */
#define HOTKEY_FORCE_EXIT 3

/**
 * handles emulator input
 */
class CInput {
  public:
    CInput(CCIA1 *cia1);

    ~CInput();

    /**
     * update input status
     * @param ev the input event
     * @param hotkeys on return, signal hotkeys pressed
     * @return 0
     */
    int update(SDL_Event *ev, uint32_t *hotkeys);

    /**
     * fills the clipboard queue with fake keyboard events, in response to
     * HOTKEY_PASTE_TEXT
     */
    void fillClipboardQueue();

    /**
     * called at every redraw, handles events in the clipboard queue, if any
     *
     * @param totalCycles total cpu cycles elapsed
     * @param cyclesPerFrame how many cpu cycles in a frame
     * @param frameSkip how many frames to skip before injecting clipboard, if
     * any
     */
    void checkClipboard(int64_t totalCycles, int cyclesPerFrame, int frameSkip);

    /**
     * inject characters in the keyboard buffer
     * @param chars the characters to inject (max 10)
     */
    void injectKeyboardBuffer(const char *chars);

  private:
    CCIA1 *_cia1 = nullptr;
    std::queue<SDL_Event *> _kqueue = {};
    uint8_t sdlScancodeToC64Scancode(uint32_t sdlScanCode);
    void processEvent(SDL_Event *ev);
    int64_t _prevTotalCycles = 0;

    void processClipboardQueue();
    bool hasClipboardEvents();
};

#endif // VC64_EMU_CINPUT_H
