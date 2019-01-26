//
// Created by valerino on 10/01/19.
//

#include "CDisplay.h"
#include <CBuffer.h>
#include <stdio.h>
#include <stdlib.h>

CDisplay::CDisplay(CVICII* vic) {
    // allocate the texture memory for the framebuffer
    _fb = (uint32_t*)calloc(1, 320*200*sizeof(uint32_t));//VIC_PAL_SCREEN_W * VIC_PAL_SCREEN_H * sizeof(uint32_t));
    _vic = vic;
}

int CDisplay::init(SDLDisplayCreateOptions* options, char** errorString) {

    // init display
    int res = CSDLUtils::initializeDisplayContext(&_ctx, options, errorString);
    if (res != 0) {
        return res;
    }

    // show!
    CSDLUtils::update(&_ctx, (void*)_fb, 320);//VIC_PAL_SCREEN_W);
    return 0;
}

CDisplay::~CDisplay() {
    CSDLUtils::finalizeDisplayContext(&_ctx);
    SAFE_FREE(_fb);
}

void CDisplay::update() {
    int videoMemSize = 0x400;
    IMemory* imem = _vic->cpu()->memory();
    uint8_t* videoMem = imem->raw(nullptr) + 0x400;
    uint8_t* charMem = imem->raw(nullptr) + 0xd000;
    int currX = 0;
    int currY = 0;
    memset(_fb,0,320*200*sizeof(uint32_t));//VIC_PAL_SCREEN_W * VIC_PAL_SCREEN_H * sizeof(uint32_t));
    for (int i=0; i < videoMemSize; i++) {
        // read screencode in video memory
        uint8_t scode;
        imem->readByte(0x400 + i, &scode);

        // read char from character memory (8x8)
        for (int j=0; j < 8; j ++) {
            uint8_t currentLine;
            imem->readByte(0xd000 + (scode * 8) + j, &currentLine);
            for (int k = 0; k < 8; k ++) {
                bool pixelSet = ((currentLine & 0x80) == 0x80);
                int pixX= (currX * 8) + k;
                int pixY= (currY * 8) + j;
                int pos = ((pixY*320) + pixX);
                if (pixelSet) {
                    _fb[pos] = 0;
                    _fb[pos+1] = 0;
                    _fb[pos+2] = 0;
                    _fb[pos+3] = 0xff;
                }
                else {
                    _fb[pos] = 0xff;
                    _fb[pos+1] = 0xff;
                    _fb[pos+2] = 0xff;
                    _fb[pos+3] = 0xff;
                }
                currentLine <<= 1;
            }
            if (currX == 40) {
                currY++;
                currX = 0;
            }
        }
        currX++;
    }
    CSDLUtils::update(&_ctx,_fb,320);//VIC_PAL_SCREEN_W);
    // read char line

    /*
     *  // Clear screen
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    for(int r = 0; r < PIXEL_ROWS; r++)
    {
        for(int c = 0; c < PIXEL_COLS; c++)
        {
            int rgb = ((int*)vic->screenBuffer())[c+r*320];

            this->vic->fireRasterInterrupt(r*c,memManager);

            SDL_Rect pixel;
            pixel.x = c*PIXELWIDTH;
            pixel.y = r*PIXELWIDTH;
            pixel.w = pixel.h = PIXELWIDTH;
            fillRect(&pixel, (uint)((rgb & 0xFF000000) >> 24), (uint)((rgb & 0x00FF0000) >> 16), (uint)((rgb & 0x0000FF00) >> 8));
        }
    }
    SDL_RenderPresent(renderer);

     */
    /*
    uint32_t videoSize;
    uint8_t* videoMem =_vic->cpu()->memory()->raw(&videoSize);
    videoMem += 0xd000;
    for(int r = 0; r < 200; r++)
    {
        SDL_SetRenderDrawColor(_ctx->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(_ctx->renderer);
        for(int c = 0; c < 320; c++) {
            int rgb = ((int*)videoMem)[c+r*320];
            SDL_Rect pixel;
            pixel.x = c*3;
            pixel.y = r*3;
            pixel.w = pixel.h = 3;
            SDL_SetRenderDrawColor(_ctx->renderer,(uint)((rgb & 0xFF000000) >> 24), (uint)((rgb & 0x00FF0000) >> 16), (uint)((rgb & 0x0000FF00) >> 8), SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(_ctx->renderer, &pixel);
            SDL_RenderPresent(_ctx->renderer);
        }
    }
     */
}
