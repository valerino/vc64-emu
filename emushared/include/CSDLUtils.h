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
 * filled by initializeDisplayContext()
 */
 typedef struct _SDLDisplayCtx {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_PixelFormat* pxFormat;
 } SDLDisplayCtx;

 /**
  * options for initializeDisplayContext()
  */
 typedef struct _SDLDisplayCreateOptions {
     // pointer to window name
     const char* windowName;
     // height
     int h;
     // width
     int w;
     // scaling factor
     int scaleFactor;
     // X position, or SDL_WINDOWPOS_UNDEFINED
     int posX;
     // Y position, or SDL_WINDOWPOS_UNDEFINED
     int posY;
     // flags for renderer (SDL_RendererFlags)
     uint32_t rendererFlags;
     // flags for window (SDL_WindowFlags)
     uint32_t windowFlags;
 } SDLDisplayCreateOptions;

class CSDLUtils {
public:
    /**
      * poll sdl events and return keyboard state
      * @param keys on return, if not NULL is the keyboard state returned by SDL_GetKeyboardState()
      * @param mustExit on return, true if quit is requested
      * @return 0 on success, or errno
      */
    static int pollEvents(uint8_t **keys, bool* mustExit);

    /**
     * initializes an SDL display context with a texture in ARGB8888 format
     * @param ctx an SDLDisplayCtx to be filled
     * @param options window creation options
     * @param errorString on error, this is filled with a pointer to the SDL error string
     * @return 0 on success, or errno
     */
    static int initializeDisplayContext(SDLDisplayCtx *ctx, SDLDisplayCreateOptions *options, char **errorString);

    /**
     * update the underlying texture with the given
     * @param ctx pointer to an initialized SDLDisplayCtx
     * @param texture the new texture buffer, in ARGB8888 format
     * @param w width of the texture
     * @return 0 on success, or errno
     */
    static int update(SDLDisplayCtx* ctx, void* texture, int w);

    /**
     * counterpart of initializeDisplayContext()
     * @param ctx an initialized SDLDisplayCtx
     */
    static void finalizeDisplayContext(SDLDisplayCtx *ctx);
};

#endif //VC64_EMU_CSDLUTILS_H
