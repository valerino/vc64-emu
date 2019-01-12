//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_CDISPLAY_H
#define VC64_EMU_CDISPLAY_H

#include <CSDLUtils.h>

// PAL c64 video output is set at 50hz
#define DISPLAY_PAL_HZ 50

/**
 * implements emulator display (VIC)
 */
class CDisplay {

private:
    SDLDisplayCtx * _ctx;
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
     * @param options window creation options
     * @param errorString on error, this is filled with a pointer to the SDL error string
     * @return 0 on success, or errno
     */
    int init(SDLDisplayCreateOptions* options, char** errorString);

    /**
     * update the display
     * @param frameBuffer pointer to the video memory
     * @param size size of the framebuffer
     * @return 0 on success
     */
    int update(uint8_t* frameBuffer, uint32_t size);

    ~CDisplay();
};


#endif //VC64_EMU_DISPLAY_H
