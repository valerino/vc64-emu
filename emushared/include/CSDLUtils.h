//
// Created by valerino on 11/01/19.
//

#ifndef VC64_EMU_CSDLUTILS_H
#define VC64_EMU_CSDLUTILS_H
#include <SDL.h>
#include <stdint.h>

/**
 * sdl utilities
 */

/**
 * @brief callback to receive events via CSDLUtils::pollEvents()
 * @param event
 */
typedef void (*SDLEventCallback)(SDL_Event *event);

class CSDLUtils {
  public:
    /**
     * poll sdl events until queue is empty, and call a callback on each
     * @param cb callback to receive events
     * @param mustExit on return, true if quit is requested
     * @return 0 on success, or errno
     */
    static void pollEvents(SDLEventCallback cb);
};

#endif // VC64_EMU_CSDLUTILS_H
