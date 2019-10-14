//
// Created by valerino on 10/01/19.
//
#pragma once

#include "CVICII.h"
#include <SDL.h>

/**
 * @brief implements emulator display (VIC)
 */
class CDisplay {
  public:
    /**
     * update the display
     */
    void update();

    /**
     * constructor
     * @param vic the vic-ii chip
     * @param wndName name of the window, for windowed mode
     * @param fullScreen true for fullscreen (default is windowed)
     * @throws std::runtime_error on error
     */
    CDisplay(CVICII *vic, const char *wndName, bool fullScreen = false);
    ~CDisplay();

    /**
     * @brief called by vic when blitting
     */
    static void blitCallback(void *thisPtr, RgbStruct *rgb, int pos);

  private:
    SDL_Window *_window = nullptr;
    SDL_Renderer *_renderer = nullptr;
    SDL_Texture *_texture = nullptr;
    SDL_PixelFormat *_pxFormat = nullptr;
    uint32_t *_fb = nullptr;
    CVICII *_vic = nullptr;

    /**
     * initializes the display (a texture) through SDL
     * @param fullScreen true for full screen
     * @param wndName the window name
     * @param errorString on error, this is filled with a pointer to the SDL
     * error string
     * @return 0 on success, or errno
     */
    int initializeDisplay(bool fullScreen, const char *wndName,
                          char **errorString);
};
