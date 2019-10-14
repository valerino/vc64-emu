//
// Created by valerino on 23/01/19.
//
#pragma once

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
 * @brief pressing ctrl-esc (force) exit the emulator
 */
#define HOTKEY_FORCE_EXIT 3

#define JOY_2_LEFT 0
#define JOY_2_RIGHT 1
#define JOY_2_UP 2
#define JOY_2_DOWN 3
#define JOY_2_FIRE 4
#define JOY_1_LEFT 5
#define JOY_1_RIGHT 6
#define JOY_1_UP 7
#define JOY_1_DOWN 8
#define JOY_1_FIRE 9
#define JOY_LEFT 10
#define JOY_RIGHT 11
#define JOY_UP 12
#define JOY_DOWN 13
#define JOY_FIRE 14

/**
 * @brief handles emulator input
 * special keys:
 * TAB  ->  run-stop
 * TAB + BACKSPACE -> runstop + restore
 * LEFT ALT -> commodore
 */
class CInput {
  public:
    /**
     * @brief constructor
     * @param cia1 pointer to the CIA1 instance
     * @param joyConfiguration 1=joystick in port 1, 2=joystick in port 2, 0=no
     * joystick(default)
     */
    CInput(CCIA1 *cia1, int joyConfiguration = 0);

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
    int _joyNum = 0;
    void processClipboardQueue();
    bool hasClipboardEvents();
    bool handleJoystick(uint32_t sdlScanCode, bool pressed);
};
