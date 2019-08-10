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
     * to be called first to initialize display
     * @param options window creation options
     * @param errorString on error, this is filled with a pointer to the SDL error string
     * @return 0 on success, or errno
     */
    int init(SDLDisplayCreateOptions* options, char** errorString);

    /**
     * update the display
     */
    void update();

    /**
     * constructor
     * @param vic the vic-ii chip
     */
    CDisplay(CVICII* vic);
    ~CDisplay();
};

#endif //VC64_EMU_DISPLAY_H
