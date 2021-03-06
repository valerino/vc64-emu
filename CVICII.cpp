//
// Created by valerino on 19/01/19.
//

#include "CVICII.h"
#include <SDL.h>
#include "CMemory.h"
#include "bitutils.h"

/**
 * @brief initialize color palette
 */
void CVICII::initPalette() {
    // https://www.c64-wiki.com/wiki/Color
    _palette[0] = {0, 0, 0};
    _palette[1] = {0xff, 0xff, 0xff};
    _palette[2] = {0x88, 0, 0};
    _palette[3] = {0xaa, 0xff, 0xee};
    _palette[4] = {0xcc, 0x44, 0xcc};
    _palette[5] = {0x00, 0xcc, 0x55};
    _palette[6] = {0, 0, 0xaa};
    _palette[7] = {0xee, 0xee, 0x77};
    _palette[8] = {0xdd, 0x88, 0x55};
    _palette[9] = {0x66, 0x44, 0};
    _palette[10] = {0xff, 0x77, 0x77};
    _palette[11] = {0x33, 0x33, 0x33};
    _palette[12] = {0x77, 0x77, 0x77};
    _palette[13] = {0xaa, 0xff, 0x66};
    _palette[14] = {0, 0x88, 0xff};
    _palette[15] = {0xbb, 0xbb, 0xbb};
}

CVICII::CVICII(CMOS65xx *cpu, CCIA2 *cia2, CPLA *pla) {
    _cpu = cpu;
    _cia2 = cia2;
    _pla = pla;
    initPalette();

    // init default addresses
    _bitmapAddress = MEMORY_BITMAP_ADDRESS;
    _screenAddress = MEMORY_SCREEN_ADDRESS;
    _charsetAddress = MEMORY_CHARSET_ADDRESS;
}

void CVICII::setBlitCallback(void *display, BlitCallback cb) {
    _cb = cb;
    _displayObj = display;
}

/**
 * blit into the framebuffer
 * @param x x coordinate
 * @param y y coordinate
 * @param rgb rgb color
 */
void CVICII::blit(int x, int y, RgbStruct *rgb) {
    if (y >= VIC_PAL_SCREEN_H) {
        // not visible, vblank
        return;
    }

    // set the pixel
    int pos = (y * VIC_PAL_SCREEN_W) + x;
    _cb(_displayObj, rgb, pos);
}

/**
 * @brief get address of a sprite
 * @param idx the sprite index 0-7
 * @return
 */
uint16_t CVICII::getSpriteDataAddress(int idx) {
    // https://www.c64-wiki.com/wiki/Sprite#Sprite_pointers
    // https://dustlayer.com/vic-ii

    // read sprite pointer for sprite idx
    uint16_t spP = _screenAddress + 0x3f8 + idx;
    uint8_t ptr;
    readVICByte(spP, &ptr);

    // get sprite data address from sprite pointer
    uint16_t addr = 64 * ptr;
    // SDL_Log("sprite address=$%x, idx=%d", addr, idx);
    return addr;
}

/**
 * @brief check if while drawing sprite we're hitting borders
 * @param x x screen coordinate
 * @param y y screen coordinate
 * @return
 */
bool CVICII::isSpriteDrawingOnBorder(int x, int y) {
    // from http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt (PAL c64)
    Rect limits;
    getScreenLimits(&limits);

    // check if we're drawing sprites on border
    if (x < limits.firstVisibleX) {
        return true;
    }
    if (y < limits.firstVisibleLine) {
        return true;
    }
    if (x > limits.lastVisibleX + limits.firstSpriteX) {
        return true;
    }
    if (y > limits.lastVisibleLine) {
        return true;
    }
    return false;
}

void CVICII::setCollisionHandling(bool enableSpriteSprite,
                                  bool enableBackgroundSprite) {
    _sprSprHwCollisionEnabled = enableSpriteSprite;
    _sprBckHwCollisionEnabled = enableBackgroundSprite;
}

/**
 * @brief detect sprite-sprite collision
 * @fixme partially working
 * @see http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt 3.8.2
 * @param idx current drawing sprite idx
 * @param x drawing sprite x coordinates
 * @param y drawing sprite x coordinates
 */
void CVICII::checkSpriteSpriteCollision(int idx, int x, int y) {
    if (!_sprSprHwCollisionEnabled) {
        // disabled
        return;
    }
    Rect limits;
    getScreenLimits(&limits);
    for (int i = 0; i < 8; i++) {
        if (!isSpriteEnabled(i)) {
            continue;
        }
        if (i == idx) {
            continue;
        }
        int xMultiplier = isSpriteXExpanded(i) ? 2 : 1;
        int yMultiplier = isSpriteYExpanded(i) ? 2 : 1;
        if (yMultiplier > 1) {
            // SDL_Log("sprite %d Y expanded", i);
        }
        if (xMultiplier > 1) {
            // SDL_Log("sprite %d X expanded", i);
        }
        int sprX = getSpriteXCoordinate(i);
        int sprY = getSpriteYCoordinate(i);
        if (x == sprX * xMultiplier && y == sprY * yMultiplier) {
            /*SDL_Log("collision sprite-%d(x=%d,y=%d) with "
                    "sprite-%d(x=%d,y=%d)",
                    idx, x, y, i, sprX, sprY);*/

            if (!_regSpriteSpriteCollision) {
                // only the first collision triggers an irq
                if (IS_BIT_SET(getInterruptEnabled(), 2)) {
                    BIT_SET(_regInterrupt, 2);
                    _cpu->irq();
                } else {
                    BIT_CLEAR(_regInterrupt, 2);
                }
            }
            BIT_SET(_regSpriteSpriteCollision, i);
            BIT_SET(_regSpriteSpriteCollision, idx);
        }
    }
    return;
}

/**
 * @brief detect sprite-background collision (technically, its
 * sprite-foreground! :) )
 * @see http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt 3.8.2
 * @param x screen drawing x coordinates
 * @param y screen drawing x coordinates
 */
void CVICII::checkSpriteBackgroundCollision(int x, int y) {
    if (!_sprBckHwCollisionEnabled) {
        // disabled
        return;
    }
    Rect limits;
    getScreenLimits(&limits);
    for (int i = 0; i < 8; i++) {
        if (!isSpriteEnabled(i)) {
            continue;
        }
        int xMultiplier = isSpriteXExpanded(i) ? 2 : 1;
        int yMultiplier = isSpriteYExpanded(i) ? 2 : 1;
        int sprX = getSpriteXCoordinate(i);
        int sprY = getSpriteYCoordinate(i);
        if (x == sprX * xMultiplier && y == sprY * yMultiplier) {
            /*SDL_Log("collision background(x=%d,y=%d) with "
                    "sprite-%d(x=%d,y=%d)",
                    x, y, i, sprX, sprY);*/

            if (!_regSpriteBckCollision) {
                // only the first collision triggers an irq
                if (IS_BIT_SET(getInterruptEnabled(), 1)) {
                    BIT_SET(_regInterrupt, 1);
                    _cpu->irq();
                } else {
                    BIT_CLEAR(_regInterrupt, 1);
                }
            }
            BIT_SET(_regSpriteBckCollision, i);
        }
    }
    return;
}

/**
 * @brief draw sprite row (multicolor sprite)
 * @see http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt 3.7.3.2
 * @param rasterLine the rasterline index to draw
 * @param idx the sprite index 0-7
 * @param x x screen coordinates
 * @param row sprite row number
 */

void CVICII::drawSpriteMulticolor(int rasterLine, int idx, int x, int row) {
    // SDL_Log("drawing multicolor sprite");
    int currentLine = rasterLine;
    uint16_t addr = getSpriteDataAddress(idx);
    bool checkSpriteSpriteColl = true;

    // draw sprite row
    for (int i = 0; i < 3; i++) {
        // get sprite byte
        uint8_t spByte;
        uint16_t a = addr + (row * 3) + i;
        // SDL_Log("reading multicolor sprite byte at $%x", a);
        readVICByte(a, &spByte);

        // read sprite data considering double-wide pixels
        for (int j = 0; j < 4; j++) {
            int color = 0;
            // https://www.c64-wiki.com/wiki/Sprite#High_resolution_or_multicolor_mode
            // bits 0 and 1 determines the color to use
            uint8_t bits = ((spByte >> j * 2) & 0x3);
            switch (bits) {
            case 0:
                break;
            case 1:
                // 01 = multicolor register 0
                color = getSpriteMulticolor(0) & 0xf;
                break;
            case 2:
                // 10  = standard sprite color for sprite idx
                color = getSpriteColor(idx) & 0xf;
                break;
            case 3:
                // 11 = multicolor register 1
                color = getSpriteMulticolor(1) & 0xf;
                break;
            default:
                break;
            }

            if (bits != 0) {
                // non-transparent, blit
                RgbStruct rgb = _palette[color];
                int pixelX = x + (i * 8) + (8 - (j * 2));
                if (!isSpriteDrawingOnBorder(pixelX, currentLine)) {
                    blit(pixelX, currentLine, &rgb);
                    blit(pixelX + 1, currentLine, &rgb);

                    // also check sprite-sprite collision
                    Rect limits;
                    getScreenLimits(&limits);
                    checkSpriteSpriteCollision(idx, pixelX, currentLine);
                }
            }
        }
    }
}

/**
 * @brief draw sprite row (standard sprite)
 * @param rasterLine the rasterline index to draw
 * @param idx the sprite index 0-7
 * @param x x screen coordinates
 * @param row sprite row number
 */
void CVICII::drawSprite(int rasterLine, int idx, int x, int row) {
    // SDL_Log("drawing standard sprite");
    int currentLine = rasterLine;

    // get sprite data address
    uint16_t addr = getSpriteDataAddress(idx);

    // a sprite is 24x21, each row is 3 bytes
    for (int i = 0; i < 3; i++) {
        // read sprite byte
        uint8_t spByte;
        uint16_t a = addr + (row * 3) + i;
        // SDL_Log("reading sprite byte at $%x", a);
        readVICByte(a, &spByte);

        // draw sprite row bit by bit, take into account sprite X expansion
        // (2x multiplier)
        int multiplier = isSpriteXExpanded(idx) ? 2 : 1;
        for (int j = 0; j < 8; j++) {
            if (IS_BIT_SET(spByte, j)) {
                // draw 2 adjacent pixels if the sprite is x-expanded
                // (multiplier=2), either just one pixel
                for (int m = 0; m < multiplier; m++) {
                    // x to screen coordinate
                    int pixelX = x + m + (i * 8 * multiplier) +
                                 (8 * multiplier) - (j * multiplier);

                    // blit pixel
                    int color = getSpriteColor(idx) & 0xf;
                    if (isSpriteDrawingOnBorder(pixelX, currentLine)) {
                        // draw using border color, transparent
                        color = getBorderColor() & 0xf;
                    } else {
                        // if we're not drawing transparent, also check
                        // sprite-sprite collision
                        Rect limits;
                        getScreenLimits(&limits);
                        checkSpriteSpriteCollision(idx, pixelX, currentLine);
                    }
                    RgbStruct rgb = _palette[color & 0xf];
                    blit(pixelX, currentLine, &rgb);
                }
            }
        }
    }
}

/**
 * @brief draw hardware sprites
 * @param rasterLine the rasterline index to draw
 */
void CVICII::drawSprites(int rasterLine) {
    if (_regSpriteEnabled == 0) {
        // no sprites enabled
        return;
    }
    if (!_DEN) {
        // display enabled bit must be set
        return;
    }

    int defaultSpriteH = 21;
    Rect limits;
    getScreenLimits(&limits);

    // loop for the 8 hardware sprites, in reverse order to respect sprite
    // priorities (sprite 0 has higher priority than 7)
    // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
    // 3.8.2. Priority and collision detection
    for (int idx = 7; idx >= 0; idx--) {
        bool spriteEnabled = isSpriteEnabled(idx);
        if (!spriteEnabled) {
            // this sprite is not enabled
            continue;
        }

        // get sprite properties
        bool isSpriteHExpanded = isSpriteYExpanded(idx);
        int h = isSpriteHExpanded ? defaultSpriteH * 2 : defaultSpriteH;
        int spriteYCoord = getSpriteYCoordinate(idx);
        int spriteXCoord = getSpriteXCoordinate(idx);
        bool multicolor = isSpriteMulticolor(idx);
        if (rasterLine >= spriteYCoord && (rasterLine < spriteYCoord + h)) {
            // draw sprite row at the current scanline
            int row = rasterLine - spriteYCoord;
            // @fixme apparently, the x coordinate is not enough and must be
            // 'shifted' some pixels to the right ....
            int x = limits.firstSpriteX + spriteXCoord;
            // SDL_Log("xscroll=%d, xcoord=%d, x=%d", _scrollX,
            // spriteXCoord, x);
            if (isSpriteHExpanded) {
                // handle Y expansion
                row /= 2;
            }

            if (multicolor) {
                drawSpriteMulticolor(rasterLine, idx, x, row);
            } else {
                drawSprite(rasterLine, idx, x, row);
            }
        }
    }
}

/**
 * @brief draw screen in character mode (standard or multicolor)
 * @param rasterLine the rasterline index to draw
 */
void CVICII::drawCharacterMode(int rasterLine) {
    // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
    Rect limits;
    getScreenLimits(&limits);

    // check if we're within the display window
    if (rasterLine < limits.firstVisibleLine ||
        rasterLine > limits.lastVisibleLine) {
        // out of display window
        return;
    }
    if (!_DEN) {
        // display enabled bit must be set
        return;
    }

    // draw characters
    int columns = 40;
    for (int c = 0; c < columns; c++) {
        if (!_CSEL) {
            // 38 characters mode, skip drawing column 0 and 39
            if (c == 0 || c == 39) {
                continue;
            }
        }

        // this is the display window line
        int line = rasterLine - limits.firstVisibleLine;

        // this is the first display window column of the character to
        // display
        int x = limits.firstVisibleX + (c * 8);

        // this is the first display window row of the character to display
        int row = line / 8;

        // this is the row of the character itself (8x8 character)
        int charRow = line % 8;

        // read screencode from screen memory
        uint8_t screenCode = getScreenCode(c, row);

        // background color
        RgbStruct backgroundRgb;
        if (_screenMode == VIC_SCREEN_MODE_EXTENDED_BACKGROUND_COLOR) {
            // determine which background color register to be used
            // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
            // 3.7.3.5. ECM text mode (ECM/BMM/MCM=1/0/0)
            if (!IS_BIT_SET(screenCode, 7) && !IS_BIT_SET(screenCode, 6)) {
                backgroundRgb = _palette[getBackgroundColor(0) & 0xf];
            } else if (!IS_BIT_SET(screenCode, 7) &&
                       IS_BIT_SET(screenCode, 6)) {
                backgroundRgb = _palette[getBackgroundColor(1) & 0xf];
            } else if (IS_BIT_SET(screenCode, 7) &&
                       !IS_BIT_SET(screenCode, 6)) {
                backgroundRgb = _palette[getBackgroundColor(2) & 0xf];
            } else if (IS_BIT_SET(screenCode, 7) && IS_BIT_SET(screenCode, 6)) {
                backgroundRgb = _palette[getBackgroundColor(3) & 0xf];
            }
            // clear bits in character
            screenCode &= 0x3f;
        } else {
            // default text mode
            backgroundRgb = _palette[getBackgroundColor(0) & 0xf];
        }

        // read the character data and color
        uint8_t data = getCharacterData(screenCode, charRow);
        uint8_t charColor = getScreenColor(c, row);
        RgbStruct charRgb;

        // draw character bit by bit
        int sMode = _screenMode;
        bool altMultiColor = false;

        // if bit 3 in color memory is set for the character, set standard
        // character mode BUT we will use only lower 4 bits of the color
        if ((charColor & 0x8) == 0 &&
            _screenMode == VIC_SCREEN_MODE_CHARACTER_MULTICOLOR) {
            sMode = VIC_SCREEN_MODE_CHARACTER_STANDARD;
            altMultiColor = true;
        }

        if (sMode == VIC_SCREEN_MODE_CHARACTER_MULTICOLOR) {
            // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
            // 3.7.3.2. Multicolor text mode (ECM/BMM/MCM=0/0/1)
            for (int i = 0; i < 8; i++) {
                // get the data bitpair
                uint8_t bits = data & 3;
                int pixelX = x + 8 - i + _scrollX;
                switch (bits) {
                case 0:
                    // 00
                    charRgb = _palette[getBackgroundColor(0) & 0xf];
                    // drawing background
                    checkSpriteBackgroundCollision(pixelX, rasterLine);
                    break;

                case 1:
                    // 01
                    charRgb = _palette[getBackgroundColor(1) & 0xf];

                    // drawing background
                    checkSpriteBackgroundCollision(pixelX, rasterLine);
                    break;

                case 2:
                    // 10
                    // drawing foreground, check collisions with sprites
                    checkSpriteBackgroundCollision(pixelX, rasterLine);
                    charRgb = _palette[getBackgroundColor(2) & 0xf];
                    break;

                case 3:
                    // 11, default (use default screen matrix color)
                    // drawing foreground, check collisions with sprites
                    checkSpriteBackgroundCollision(pixelX, rasterLine);
                    charRgb = _palette[charColor & 7];
                    break;
                }
                if (pixelX > (320 + limits.firstVisibleX)) {
                    // off screen -> background color
                    charRgb = _palette[getBackgroundColor(0) & 0xf];
                }
                blit(pixelX, rasterLine, &charRgb);
                blit(pixelX + 1, rasterLine, &charRgb);

                // each pixel is doubled, shift data right
                i++;
                data >>= 2;
            }
        } else {
            // default text mode or extended background mode or alternative
            // multicolor mode
            // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt 3.7.3.1.
            // Standard text mode (ECM/BMM/MCM=0/0/0) (c-data here is
            // referred as a 12bit value, the lower 8 bits are the character
            // data, the higher 4 bits the color read from color memory)
            for (int i = 0; i < 8; i++) {
                int pixelX = x + 8 - i + _scrollX;
                if (pixelX > (320 + limits.firstVisibleX)) {
                    // off screen -> background color
                    blit(pixelX, rasterLine, &backgroundRgb);
                    continue;
                }

                if (IS_BIT_SET(data, i)) {
                    // put pixel in foreground color
                    if (altMultiColor) {
                        // bit 3 is set in color read from color memory, use
                        // only colors 0-7
                        charColor &= 7;
                    } else {
                        // full range
                        charColor &= 0xf;
                    }
                    charRgb = _palette[charColor];
                    // drawing foreground, check collisions with sprites
                    checkSpriteBackgroundCollision(pixelX, rasterLine);
                    blit(pixelX, rasterLine, &charRgb);
                } else {
                    // put pixel in background color
                    blit(pixelX, rasterLine, &backgroundRgb);
                }
            }
        }
    }
}

/**
 * @brief draw screen in bitmap mode (standard or multicolor)
 * @param rasterLine the rasterline index to draw
 */
void CVICII::drawBitmapMode(int rasterLine) {
    Rect limits;
    getScreenLimits(&limits);

    if (rasterLine < limits.firstVisibleLine ||
        rasterLine > limits.lastVisibleLine) {
        // out of display window
        return;
    }
    if (!_DEN) {
        // display enabled bit must be set
        return;
    }

    // draw bitmap
    int columns = 40;
    for (int c = 0; c < columns; c++) {
        // this is the display window line
        int line = rasterLine - limits.firstVisibleLine;

        // this is the first display window column for this this block
        int x = limits.firstVisibleX + (c * 8);

        // this is the first displaywindow row for this block
        int row = line / 8;

        // this is the row of thebitmap block itself (8x8)
        int bitmapRow = line % 8;

        // read screencode for standard color
        uint8_t screenCode = getScreenCode(c, row);

        // read color memory for multicolor
        uint8_t screenColor = getScreenColor(c, row);

        // read bitmap data
        uint8_t bitmapData = getBitmapData(c, row, bitmapRow);

        // draw bitmap
        if (_screenMode == VIC_SCREEN_MODE_BITMAP_STANDARD) {
            // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
            // 3.7.3.3. Standard bitmap mode (ECM/BMM/MCM=0/1/0)
            // SDL_Log("drawing standard bitmap");
            RgbStruct fgColor = _palette[(screenCode >> 4) & 0xf];
            RgbStruct bgColor = _palette[screenCode & 0xf];
            for (int i = 0; i < 8; i++) {
                int pixelX = x + 8 - i + _scrollX;
                if (pixelX > (320 + limits.firstVisibleX)) {
                    // do not draw off screen
                    blit(pixelX, rasterLine, &bgColor);
                    continue;
                }

                if (IS_BIT_SET(bitmapData, i)) {
                    // drawing foreground, check collisions with sprites
                    checkSpriteBackgroundCollision(pixelX, rasterLine);
                    blit(pixelX, rasterLine, &fgColor);
                } else {
                    blit(pixelX, rasterLine, &bgColor);
                }
            }
        } else {
            // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
            // 3.7.3.4. Multicolor bitmap mode (ECM/BMM/MCM=0/1/1)
            // SDL_Log("drawing multicolor bitmap");
            for (int i = 0; i < 8; i++) {
                // get the bitmap data bitpair
                uint8_t bits = bitmapData & 3;
                RgbStruct rgb;
                int pixelX = x + 8 - i + _scrollX;

                switch (bits) {
                case 0:
                    // 00
                    // drawing background
                    rgb = _palette[getBackgroundColor(0) & 0xf];
                    break;

                case 1:
                    // 01
                    // drawing background
                    checkSpriteBackgroundCollision(pixelX, rasterLine);
                    rgb = _palette[(screenCode >> 4) & 0xf];
                    break;

                case 2:
                    // 10
                    // drawing foreground, check collisions with sprites
                    checkSpriteBackgroundCollision(pixelX, rasterLine);
                    rgb = _palette[screenCode & 0xf];
                    break;

                case 3:
                    // 11
                    // drawing foreground, check collisions with sprites
                    checkSpriteBackgroundCollision(pixelX, rasterLine);
                    rgb = _palette[screenColor & 0xf];
                    break;
                }
                blit(pixelX, rasterLine, &rgb);
                blit(pixelX + 1, rasterLine, &rgb);

                // each pixel is doubled, shift bitmapdata right
                i++;
                bitmapData >>= 2;
            }
        }
    }
}

/**
 * @brief draw a border line
 * @param rasterLine the rasterline index to draw
 */
void CVICII::drawBorder(int rasterLine) {
    // draw border row through all screen
    RgbStruct borderRgb = _palette[getBorderColor() & 0xf];
    for (int i = 0; i < VIC_PAL_SCREEN_W; i++) {
        blit(i, rasterLine, &borderRgb);
    }
}

/**
 * @brief get display window (the 'screen' inside border) limits
 * @param limits on return, the display window limits (first/last visible
 * coordinates)
 */
void CVICII::getScreenLimits(Rect *limits) {
    // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
    memset(limits, 0, sizeof(Rect));
    limits->firstVisibleLine = 51;
    limits->lastVisibleLine = 250;
    if (!_RSEL) {
        limits->firstVisibleLine = 55;
        limits->lastVisibleLine = 246;
    }

    // @fixme the following X coordinates are merely found around in other
    // emulators..... but it's just a dirty hack

    // @fixme: this is wrong, adjusted manually (+18). should be the
    // firstVisibleX taken from documentation.....
    // limits->firstVisibleX = 24;
    limits->firstVisibleX = 42;
    limits->lastVisibleX = 343;
    limits->firstSpriteX = 18;
    if (!_CSEL) {
        // limits->firstVisibleX = 31;
        // @fixme: this is wrong, adjusted manually (+18). should be the
        // firstVisibleX taken from documentation.....
        limits->firstVisibleX = 49;
        limits->lastVisibleX = 334;
    }

    limits->firstVblankLine = 15;
    limits->lastVblankLine = 300;
}

int CVICII::update(long cycleCount) {
    // check for badline
    int currentRaster = getCurrentRasterLine();
    bool isBadLine = ((currentRaster >= 0x30) && (currentRaster <= 0xf7)) &&
                     ((currentRaster & 7) == (_scrollY & 7));
    if (currentRaster == 0x30 && _scrollY == 0 && IS_BIT_SET(_regCR1, 4)) {
        isBadLine = true;
    }

    // drawing a line always happens every 63 cycles on a PAL c64
    int elapsedCycles = cycleCount - _prevCycles;

    if (isBadLine) {
        if (elapsedCycles < VIC_PAL_CYCLES_PER_BADLINE) {
            return 0;
        }
    } else {
        if (elapsedCycles < VIC_PAL_CYCLES_PER_LINE) {
            return 0;
        }
    }

    // are we between in between upper and lower vblanks (= effectively
    // drawing the screen) ?
    Rect limits;
    getScreenLimits(&limits);
    if (currentRaster >= limits.firstVblankLine &&
        currentRaster <= limits.lastVblankLine) {
        drawBorder(currentRaster - limits.firstVblankLine);

        if (_screenMode == VIC_SCREEN_MODE_CHARACTER_STANDARD ||
            _screenMode == VIC_SCREEN_MODE_CHARACTER_MULTICOLOR ||
            _screenMode == VIC_SCREEN_MODE_EXTENDED_BACKGROUND_COLOR) {
            // draw screen line in character mode
            drawCharacterMode(currentRaster - limits.firstVblankLine);

        } else if (_screenMode == VIC_SCREEN_MODE_BITMAP_STANDARD ||
                   _screenMode == VIC_SCREEN_MODE_BITMAP_MULTICOLOR) {
            // draw bitmap
            drawBitmapMode(currentRaster - limits.firstVblankLine);
        }

        // draw sprites
        drawSprites(currentRaster - limits.firstVblankLine);
    }

    if (IS_BIT_SET(getInterruptEnabled(), 0)) {
        // handle raster interrupt
        if ((currentRaster - limits.firstVblankLine) ==
            _rasterIrqLine - limits.firstVblankLine) {
            // trigger irq if bits in $d01a is set for the raster
            // interrupt
            BIT_SET(_regInterrupt, 0);
            _cpu->irq();
        } else {
            BIT_CLEAR(_regInterrupt, 0);
        }
    }

    // update registers accordingly
    currentRaster++;
    setCurrentRasterLine(currentRaster);

    // return how many cycles drawing occupied
    _prevCycles = cycleCount;
    // return elapsedCycles;
    return isBadLine ? VIC_PAL_CYCLES_PER_BADLINE : VIC_PAL_CYCLES_PER_LINE;
}

void CVICII::read(uint16_t address, uint8_t *bt) {
    // check shadow address
    uint16_t addr = handleShadowAddress(address);

    switch (addr) {
    case 0xd000:
    case 0xd001:
    case 0xd002:
    case 0xd003:
    case 0xd004:
    case 0xd005:
    case 0xd006:
    case 0xd007:
    case 0xd008:
    case 0xd009:
    case 0xd00a:
    case 0xd00b:
    case 0xd00c:
    case 0xd00d:
    case 0xd00e:
    case 0xd00f: {
        // MmX/Y = sprite n X/Y coordinates
        int spriteCoordIdx = addr - 0xd000;
        *bt = getSpriteCoordinate(spriteCoordIdx);
        break;
    }

    case 0xd010:
        // MSBx of X sprites X coordinates
        *bt = _regMSBX;
        break;

    case 0xd011:
        // CR1
        *bt = _regCR1;
        break;

    case 0xd012:
        // RASTER
        *bt = _regRASTER;
        break;

    case 0xd013:
    case 0xd014: {
        // lightpen X/Y coordinate
        int lp = addr - 0xd013;
        *bt = _regLP[lp];
        break;
    }

    case 0xd015:
        // MnE: sprite enabled
        *bt = _regSpriteEnabled;
        break;

    case 0xd016:
        // CR2
        *bt = getCR2();
        break;

    case 0xd017:
        // MnYE: sprite Y expanded
        *bt = _regSpriteYExpansion;
        break;

    case 0xd018:
        // memory pointers
        *bt = getMemoryPointers();
        break;

    case 0xd019: {
        // interrupt latch
        *bt = getInterruptLatch();

        // clear on read
        if (IS_BIT_SET(*bt, 0)) {
            BIT_CLEAR(_regInterrupt, 0);
        }
        if (IS_BIT_SET(*bt, 1)) {
            BIT_CLEAR(_regInterrupt, 1);
        }
        if (IS_BIT_SET(*bt, 2)) {
            BIT_CLEAR(_regInterrupt, 2);
        }
        if (IS_BIT_SET(*bt, 3)) {
            BIT_CLEAR(_regInterrupt, 3);
        }
        if (IS_BIT_SET(*bt, 7)) {
            BIT_CLEAR(_regInterrupt, 7);
        }
        break;
    }

    case 0xd01a:
        // interrupt enable register
        *bt = getInterruptEnabled();
        break;

    case 0xd01b:
        // MnDP: sprite data priority
        *bt = _regSpriteDataPriority;
        break;

    case 0xd01c:
        // MnMC: sprite multicolor
        *bt = _regSpriteMultiColor;
        break;

    case 0xd01d:
        // MnXE: sprite X expanded
        *bt = _regSpriteXExpansion;
        break;

    case 0xd01e:
        // sprite-sprite collision
        *bt = _regSpriteSpriteCollision;

        // reset on read
        _regSpriteSpriteCollision = 0;
        break;

    case 0xd01f:
        // sprite-background collision
        *bt = _regSpriteBckCollision;

        // reset on read
        _regSpriteBckCollision = 0;
        break;

    case 0xd020:
        // EC
        // bits 4,5,6,7 are not connected and give 1 on reading
        *bt = getBorderColor() | 0xf0;
        break;

    case 0xd021:
    case 0xd022:
    case 0xd023:
    case 0xd024: {
        // BnC = background color 0-3
        // bits 4,5,6,7 are not connected and give 1 on reading
        int bcIdx = addr - 0xd021;
        *bt = getBackgroundColor(bcIdx) | 0xf0;
        break;
    }

    case 0xd025:
    case 0xd026: {
        // MMn = sprite multicolor 0-1
        // bits 4,5,6,7 are not connected and give 1 on reading
        int mmIdx = addr - 0xd025;
        *bt = getSpriteMulticolor(mmIdx) | 0xf0;
        break;
    }

    case 0xd027:
    case 0xd028:
    case 0xd029:
    case 0xd02a:
    case 0xd02b:
    case 0xd02c:
    case 0xd02d:
    case 0xd02e: {
        // MnC = sprite color 0-7
        // bits 4,5,6,7 are not connected and give 1 on reading
        int mIdx = addr - 0xd027;
        *bt = getSpriteColor(mIdx) | 0xf0;
        break;
    }

    case 0xd02f:
    case 0xd030:
    case 0xd031:
    case 0xd032:
    case 0xd033:
    case 0xd034:
    case 0xd035:
    case 0xd036:
    case 0xd037:
    case 0xd038:
    case 0xd039:
    case 0xd03a:
    case 0xd03b:
    case 0xd03c:
    case 0xd03d:
    case 0xd03e:
    case 0xd03f:
        // these locations returns 0xff on read
        *bt = 0xff;
        break;

    default:
        SDL_Log("unhandled vic read at $%0x", addr);
        break;
    }
}

void CVICII::write(uint16_t address, uint8_t bt) {
    // check shadow
    uint16_t addr = handleShadowAddress(address);

    switch (addr) {
    case 0xd000:
    case 0xd001:
    case 0xd002:
    case 0xd003:
    case 0xd004:
    case 0xd005:
    case 0xd006:
    case 0xd007:
    case 0xd008:
    case 0xd009:
    case 0xd00a:
    case 0xd00b:
    case 0xd00c:
    case 0xd00d:
    case 0xd00e:
    case 0xd00f: {
        // MmX/Y = sprite n X/Y coordinates
        int spriteCoordIdx = addr - 0xd000;
        setSpriteCoordinate(spriteCoordIdx, bt);
        break;
    }
    case 0xd010:
        // MSBx of X sprites X coordinates
        _regMSBX = bt;
        break;

    case 0xd011:
        // CR1
        _regCR1 = bt;

        // display enable (DEN)
        _DEN = IS_BIT_SET(_regCR1, 4);

        // YSCROLL
        _scrollY = _regCR1 & 0x7;

        // RSEL
        _RSEL = IS_BIT_SET(_regCR1, 3);

        // bit 7 also sets bit 8 of the raster irq compare line (bit 8)
        if (IS_BIT_SET(_regCR1, 7)) {
            BIT_SET(_rasterIrqLine, 8);
        }

        // set screen mode (ECM/B MM bits)
        setScreenMode();
        break;

    case 0xd012:
        // RASTER
        _regRASTER = bt;

        // also set the line at which the raster irq happens (bit 0-7)
        _rasterIrqLine = bt;
        break;

    case 0xd013:
    case 0xd014: {
        // lightpen X/Y coordinate
        int lp = addr - 0xd013;
        _regLP[lp] = bt;
        break;
    }

    case 0xd015:
        // MnE: sprite enabled
        _regSpriteEnabled = bt;
        break;

    case 0xd016:
        // CR2
        // bit 6, 7 are always set
        _regCR2 = bt;

        // XSCROLL (bit 0,1,2)
        _scrollX = getCR2() & 0x7;

        // CSEL
        _CSEL = IS_BIT_SET(getCR2(), 3);

        // set screen mode (MCM bit)
        setScreenMode();
        break;

    case 0xd017:
        // MnYE: sprite Y expanded
        _regSpriteYExpansion = bt;
        break;

    case 0xd018:
        // memory pointers
        // https://www.c64-wiki.com/wiki/53272
        _regMemoryPointers = bt;

        // SDL_Log("memory pointers=$%x", _regMemoryPointers);

        // set addresses (https://www.c64-wiki.com/wiki/Page_208-211)
        // VM10,VM11,VM12,VM13 = character set address
        // The four most significant bits form a 4-bit number in the range 0
        // thru 15 Multiplied with 1024 this gives the start address for the
        // screen character RAM.
        _screenAddress = (getMemoryPointers() >> 4);
        _screenAddress *= 1024;
        // SDL_Log("screen memory address=$%x", _screenAddress);

        // CB11, CB12, CB13 = character set address
        // Bits 1 thru 3 (weights 2 thru 8) form a 3-bit number in the range
        // 0 thru 7: Multiplied with 2048 this gives the start address for
        // the character set.
        _charsetAddress = (getMemoryPointers() >> 1) & 0x7;
        _charsetAddress *= 2048;
        // SDL_Log("charset address=$%x", _charsetAddress);

        // CB13 = bitmap address
        // The four most significant bits form a 4-bit number in the range 0
        // thru 15: Multiplied with 1024 this gives the start address for
        // the color information. Bit 3 indicates whether the bitmap begins
        // at start address 0 / $0(bit set to "0"),or at 8192 / $2000(bit
        // set to "1").
        if (IS_BIT_SET(getMemoryPointers(), 3)) {
            _bitmapAddress = 0x2000;
        } else {
            _bitmapAddress = 0;
        }
        // SDL_Log("bitmap address=$%x", _bitmapAddress);
        break;

    case 0xd019:
        // interrupt latch
        if (bt) {
            BIT_SET(_regInterrupt, 7);
        }

        // NAND
        _regInterrupt &= ~bt;
        break;

    case 0xd01a:
        // interrupt enabled
        _regInterruptEnabled = bt;
        break;

    case 0xd01b:
        // MnDP: sprite data priority
        _regSpriteDataPriority = bt;
        break;

    case 0xd01c:
        // MnMC: sprite multicolor
        _regSpriteMultiColor = bt;
        break;

    case 0xd01d:
        // MnXE: sprite X expanded
        _regSpriteXExpansion = bt;
        break;

    case 0xd01e:
        // sprite-sprite collision
        _regSpriteSpriteCollision = bt;
        break;

    case 0xd01f:
        // sprite-background collision
        _regSpriteBckCollision = bt;
        break;

    case 0xd020:
        // EC
        _regBorderColor = bt;
        break;

    case 0xd021:
    case 0xd022:
    case 0xd023:
    case 0xd024: {
        // BnC = background color 0-3
        int bcIdx = addr - 0xd021;
        setBackgoundColor(bcIdx, bt);
        break;
    }

    case 0xd025:
    case 0xd026: {
        // MMn = sprite multicolor 0-1
        int mmIdx = addr - 0xd025;
        setSpriteMulticolor(mmIdx, bt);
        break;
    }

    case 0xd027:
    case 0xd028:
    case 0xd029:
    case 0xd02a:
    case 0xd02b:
    case 0xd02c:
    case 0xd02d:
    case 0xd02e: {
        // MnC = sprite color 0-7
        int mIdx = addr - 0xd027;
        setSpriteColor(mIdx, bt);
        break;
    }

    case 0xd02f:
    case 0xd030:
    case 0xd031:
    case 0xd032:
    case 0xd033:
    case 0xd034:
    case 0xd035:
    case 0xd036:
    case 0xd037:
    case 0xd038:
    case 0xd039:
    case 0xd03a:
    case 0xd03b:
    case 0xd03c:
    case 0xd03d:
    case 0xd03e:
    case 0xd03f:
        // these locations are ignored on write
        break;

    default:
        SDL_Log("unhandled vic write at $%0x", addr);
        break;
    }

    // write to memory anyway (attack of the mutant camels)
    // @fixme this is wrong, shouldn't be needed
    _cpu->memory()->writeByte(addr, bt);
}

/**
 * @brief set sprite x or y coordinate, affecting MnX/Y registers
 * @param idx register index in the sprites coordinates registers array
 * @param val the value to be set
 */
void CVICII::setSpriteCoordinate(int idx, uint8_t val) { _regM[idx] = val; }

/**
 * @brief get sprite x or y coordinate
 * @param idx register index in the sprites coordinates registers array
 * @return x or y coordinate
 */
uint8_t CVICII::getSpriteCoordinate(int idx) { return _regM[idx]; }

/**
 * @brief get X coordinate of sprite at idx, combining value of the MnX
 * with the corresponding bit of $d010 as MSB
 * @return the effective X coordinate
 */
uint16_t CVICII::getSpriteXCoordinate(int idx) {
    // get MnX
    uint16_t mnx = _regM[idx * 2];

    // set 8th bit if the corresponding sprite index bit is set in $d010
    if (IS_BIT_SET(_regMSBX, idx)) {
        BIT_SET(mnx, 8);
    }
    return mnx;
}

/**
 * @brief get Y coordinate of sprite at idx
 * @return
 */
uint8_t CVICII::getSpriteYCoordinate(int idx) {
    // get MnY
    uint8_t mny = _regM[(idx * 2) + 1];
    return mny + 1;
}

/**
 * @brief check if sprite n is enabled
 * @return
 */
bool CVICII::isSpriteEnabled(int idx) {
    bool enabled = IS_BIT_SET(_regSpriteEnabled, idx);
    return enabled;
}

/**
 * @brief check if sprite n is Y expanded (Y size * 2)
 * @return
 */
bool CVICII::isSpriteYExpanded(int idx) {
    bool expanded = IS_BIT_SET(_regSpriteYExpansion, idx);
    return expanded;
}

/**
 * @brief check if the sprite n is multicolor
 * @return
 */
bool CVICII::isSpriteMulticolor(int idx) {
    bool mc = IS_BIT_SET(_regSpriteMultiColor, idx);
    return mc;
}

/**
 * @brief check if sprite n is Y expanded (Y size * 2)
 * @return
 */
bool CVICII::isSpriteXExpanded(int idx) {
    bool expanded = IS_BIT_SET(_regSpriteXExpansion, idx);
    return expanded;
}

/**
 * @brief get the background color from the BnC register
 * @param idx register index in the background color registers array
 * @return the background color 0-3
 */
uint8_t CVICII::getBackgroundColor(int idx) { return _regBC[idx]; }

/**
 * @brief set the background color in the BnC register (4 bits)
 * @param idx register index in the background color registers array
 * @param val the color to set
 */
void CVICII::setBackgoundColor(int idx, uint8_t val) { _regBC[idx] = val; }

/**
 * @brief set color of sprite n in the MnC register (4 bits)
 * @param idx register index in the sprite color registers array
 * @param val the color to set
 */
void CVICII::setSpriteColor(int idx, uint8_t val) { _regMC[idx] = val; }

/**
 * @brief get color of sprite n from the MnC register
 * @param idx register index in the sprite color registers array
 */
uint8_t CVICII::getSpriteColor(int idx) { return _regMC[idx]; }

/**
 * @brief get color from sprite multicolor register MMn
 * @param idx register index in the sprite multicolor registers array
 * @param val the color to set
 */
void CVICII::setSpriteMulticolor(int idx, uint8_t val) { _regMM[idx] = val; }

uint8_t CVICII::getSpriteMulticolor(int idx) { return _regMM[idx]; }

/**
 * @brief addresses between 0xd040 and 0xd3ff are shadows of 0xd000-0xd03f
 * @param address the input address
 * @return the effective address
 */
uint16_t CVICII::handleShadowAddress(uint16_t address) {
    address &= 0xff;
    address &= 0x3f;
    return address + VIC_REGISTERS_START;
}

/**
 * @brief set the screen mode depending on CR1 and CR2
 */
void CVICII::setScreenMode() {
    // refers to bitmasks at
    // https://www.c64-wiki.com/wiki/Standard_Character_Mode
    if (!IS_BIT_SET(_regCR1, 6) && !IS_BIT_SET(_regCR1, 5) &&
        !IS_BIT_SET(getCR2(), 4)) {
        /*SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "selecting standard character mode");*/
        _screenMode = VIC_SCREEN_MODE_CHARACTER_STANDARD;
        return;
    }
    if (!IS_BIT_SET(_regCR1, 6) && !IS_BIT_SET(_regCR1, 5) &&
        IS_BIT_SET(getCR2(), 4)) {
        /*SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "selecting multicolor character mode");*/
        _screenMode = VIC_SCREEN_MODE_CHARACTER_MULTICOLOR;
        return;
    }
    if (!IS_BIT_SET(_regCR1, 6) && IS_BIT_SET(_regCR1, 5) &&
        !IS_BIT_SET(getCR2(), 4)) {
        /*SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "selecting standard bitmap mode");*/
        _screenMode = VIC_SCREEN_MODE_BITMAP_STANDARD;
        return;
    }
    if (!IS_BIT_SET(_regCR1, 6) && IS_BIT_SET(_regCR1, 5) &&
        IS_BIT_SET(getCR2(), 4)) {
        /*SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "selecting multicolor bitmap mode");*/
        _screenMode = VIC_SCREEN_MODE_BITMAP_MULTICOLOR;
        return;
    }
    if (IS_BIT_SET(_regCR1, 6) && !IS_BIT_SET(_regCR1, 5) &&
        !IS_BIT_SET(getCR2(), 4)) {
        /*SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "selecting extended background color mode");*/
        _screenMode = VIC_SCREEN_MODE_EXTENDED_BACKGROUND_COLOR;
        return;
    }

    /*SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "invalid color mode!! CR1=%x, CR2=%x", _regCR1, getCR2());*/
}

/**
 * @brief get the current raster line indicated by $d012(RASTER) and bit
 * 7 of $d011(CR1)
 * @return
 */
int CVICII::getCurrentRasterLine() {
    int line = _regRASTER;
    if (IS_BIT_SET(_regCR1, 7)) {
        BIT_SET(line, 8);
    }
    return line;
}

/**
 * @brief update the current raster line ($d012 and bit7 of $d011)
 * @param line the current line
 */
void CVICII::setCurrentRasterLine(int line) {
    if (line > VIC_PAL_SCANLINES) {
        // reset
        _regRASTER = 0;
        BIT_CLEAR(_regCR1, 7);
        return;
    }

    // set $d012
    _regRASTER = line & 0xff;

    // set bit 7 of $d011 accordingly
    if (line > 0xff) {
        BIT_SET(_regCR1, 7);
    } else {
        BIT_CLEAR(_regCR1, 7);
    }
}

/**
 * @brief get the screencode for a position in the 40x25 video matrix
 * @param x x coordinate
 * @param y y coordinate
 * @return
 */
uint8_t CVICII::getScreenCode(int x, int y) {
    uint16_t screenCodeAddr = _screenAddress + (y * 40) + x;
    uint8_t screenCode;
    // SDL_Log("reading screencode from address=%x", screenCodeAddr);
    readVICByte(screenCodeAddr, &screenCode);
    return screenCode;
}

/**
 * @brief get the colors for a position in the 40x25 video matrix from
 * the color memory
 * @param x x coordinate
 * @param y y coordinate
 * @return
 */
uint8_t CVICII::getScreenColor(int x, int y) {
    uint16_t colorAddr = MEMORY_COLOR_ADDRESS + (y * 40) + x;
    uint8_t screenColor;

    // read from raw memory
    _cpu->memory()->readByte(colorAddr, &screenColor, true);
    return screenColor;
}

void CVICII::readVICByte(uint16_t address, uint8_t *bt) {
    bool readFromCharsetRom = false;

    uint16_t addr = _cia2->vicMemoryAddress() + address;

    // these addresses are shadowed with the rom character set
    if ((addr >= 0x1000 && addr <= 0x1fff) ||
        (addr >= 0x9000 && addr <= 0x9fff)) {
        // shadows ROM address
        readFromCharsetRom = true;
    }

    if (readFromCharsetRom) {
        // SDL_Log("vic read from ROM charset");
        uint8_t *cAddr = ((CMemory *)_cpu->memory())->charset();
        if (IS_BIT_SET(addr, 11)) {
            // select the alternate character set in rom
            cAddr += 0x800;
        }

        // read char data from rom
        // SDL_Log("vic read character from ROM");
        *bt = cAddr[(addr - _charsetAddress) & 0xfff];
    } else {
        // raw memory read
        // SDL_Log("vic read character from RAM");
        _cpu->memory()->readByte(addr, bt, true);
    }
}

/**
 * @brief get a row (8x8) of a character from character set
 * @param x x screen coordinate
 * @param y y screen coordinate
 * @param charRow the row inside the character bitmap, 0-8
 * @return
 */
uint8_t CVICII::getCharacterData(int screenCode, int charRow) {

    uint8_t data = 0;
    int addr = _charsetAddress + ((screenCode * 8) + charRow);
    readVICByte(addr, &data);
    return data;
}

/**
 * @brief get a row (8x8) of bitmap
 * @param x x screen coordinate
 * @param y y screen coordinate
 * @param bitmapRow the row inside bitmap, 0-8
 * @return
 */
uint8_t CVICII::getBitmapData(int x, int y, int bitmapRow) {

    uint16_t addr = _bitmapAddress + ((((y * 40) + x) * 8) + bitmapRow);
    uint8_t data;
    readVICByte(addr, &data);
    return data;
}

/**
 * @brief get border color, $d020
 * @return
 */
uint8_t CVICII::getBorderColor() {
    // bit 4,5,6,7 always set
    return (_regBorderColor | 0xf0);
}

/**
 * @brief get control register 2, $d016
 * @return
 */
uint8_t CVICII::getCR2() {
    // bit 6,7 always set
    return (_regCR2 | 0xc0);
}

/**
 * @brief get interrupt register, $d019
 * @return
 */
uint8_t CVICII::getInterruptLatch() {
    // bit 4,5,6 always set
    return (_regInterrupt | 0x70);
}

/**
 * @brief get interrupt enabled register, $d01a
 * @return
 */
uint8_t CVICII::getInterruptEnabled() {
    // bit 4,5,6,7 always set
    return (_regInterruptEnabled | 0xf0);
}

/**
 * @brief get memory pointers register, $d018
 * @return
 */
uint8_t CVICII::getMemoryPointers() {
    // bit 0 always set
    return (_regMemoryPointers | 1);
}
