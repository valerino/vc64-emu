//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_CDISPLAY_H
#define VC64_EMU_CDISPLAY_H

#include <CSDLUtils.h>
#include "CVICII.h"

/**
 * implements emulator display (VIC)
 */
class CDisplay {

private:
    SDLDisplayCtx _ctx = {0};
    uint32_t* _fb = nullptr;
    CVICII* _vic = nullptr;
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
    CDisplay(CVICII* vic, const char* wndName, bool fullScreen = false);
    ~CDisplay();

};

#endif //VC64_EMU_DISPLAY_H
