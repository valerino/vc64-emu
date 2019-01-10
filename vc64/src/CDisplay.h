//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_CDISPLAY_H
#define VC64_EMU_CDISPLAY_H

#include <SDL.h>

/**
 * implements emulator display (VIC)
 */
class CDisplay {

private:
    SDL_Window * _window;
    SDL_Renderer * _renderer;
    CDisplay();
    static CDisplay* _instance;

public:
    /**
     * gets the singleton
     * @return the CDisplay instance
     */
    static CDisplay* instance();

    /**
     * to be called first to initialize display
     * @return true on success
     */
    bool init();

    ~CDisplay();
};


#endif //VC64_EMU_DISPLAY_H
