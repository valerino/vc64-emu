//
// Created by valerino on 11/01/19.
//

#ifndef VC64_EMU_CSDLUTILS_H
#define VC64_EMU_CSDLUTILS_H

#include <stdint.h>
/**
 * sdl utilities
 */
class CSDLUtils {
public:
    /**
      * poll sdl events and return keyboard state
      * @param keys on return, if not NULL is the keyboard state returned by SDL_GetKeyboardState()
      * @param mustExit on return, true if quit is requested
      * @return 0 on success, or errno
      */
    static int pollEvents(uint8_t **keys, bool* mustExit);
};


#endif //VC64_EMU_CSDLUTILS_H
