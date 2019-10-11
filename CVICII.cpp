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

CVICII::CVICII(CMOS65xx *cpu, CCIA2 *cia2) {
    _cpu = cpu;
    _cia2 = cia2;
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
    _cpu->memory()->readByte(spP, &ptr);

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
    // @todo this will obviously prevent to draw sprites on border. .... but
    // leave it as is for now
    if (x < limits.firstVisibleX) {
        return true;
    }
    if (y < limits.firstVisibleY) {
        return true;
    }
    if (x > limits.lastVisibleX + limits.firstSpriteX) {
        return true;
    }
    if (y > limits.lastVisibleY) {
        return true;
    }
    return false;
}

/**
 * @brief detect sprite-sprite collision
 * @todo not yet fully working (seems to partially work, i.e. aztec challenge, x
 * messed up)
 * @param idx current drawing sprite idx
 * @param x drawing x coordinates
 * @param y drawing x coordinates
 */
void CVICII::checkSpriteSpriteCollision(int idx, int x, int y) {
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
        int sprX = (getSpriteCoordinate(i) + limits.firstSpriteX) * xMultiplier;
        int sprY = getSpriteCoordinate(i + 1) * yMultiplier;
        if ((x >= sprX && (x <= (sprX + 24 * xMultiplier)) && (y >= sprY) &&
             (y <= sprY + 21 * yMultiplier))) {
            /*SDL_Log("collision sprite-%d(x=%d,y=%d) with "
                    "sprite-%d(x=%d,y=%d)",
                    idx, x, y, i, sprX, sprY);*/

            if (!_regSpriteSpriteCollision) {
                // only the first collision triggers an irq
                if (IS_BIT_SET(_regInterruptEnabled, 2)) {
                    BIT_SET(_regInterrupt, 2);
                    _cpu->irq();
                }
            }
            BIT_SET(_regSpriteSpriteCollision, i);
            BIT_SET(_regSpriteSpriteCollision, idx);

        } else {
            BIT_CLEAR(_regInterrupt, 2);
        }
    }
    return;
}

/**
 * @brief draw sprite row (multicolor sprite)
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
        _cpu->memory()->readByte(a, &spByte);

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
                color = getSpriteMulticolor(0);
                break;
            case 2:
                // 10  = standard sprite color for sprite idx
                color = getSpriteColor(idx);
                break;
            case 3:
                // 11 = multicolor register 1
                color = getSpriteMulticolor(1);
                break;
            default:
                break;
            }

            if (bits != 0) {
                // non-transparent, blit
                RgbStruct rgb = _palette[color];
                int pixelX = x + (i * 8) + (8 - (j * 2));
                blit(pixelX, currentLine, &rgb);
                blit(pixelX + 1, currentLine, &rgb);

                // also check sprite-sprite collision
                Rect limits;
                getScreenLimits(&limits);
                checkSpriteSpriteCollision(
                    idx, pixelX,
                    currentLine); // - limits.firstVisibleY);
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
    // SDL_Log("drawing sprite");
    int currentLine = rasterLine;

    // get sprite data address
    uint16_t addr = getSpriteDataAddress(idx);

    // a sprite is 24x21, each row is 3 bytes
    for (int i = 0; i < 3; i++) {
        // read sprite byte
        uint8_t spByte;
        uint16_t a = addr + (row * 3) + i;
        // SDL_Log("reading sprite byte at $%x", a);
        _cpu->memory()->readByte(a, &spByte);

        // draw sprite row bit by bit, take into account sprite X expansion
        // (2x multiplier)
        int multiplier = isSpriteXExpanded(idx) ? 2 : 1;
        for (int m = 0; m < multiplier; m++) {
            for (int j = 0; j < 8; j++) {
                if (IS_BIT_SET(spByte, j)) {
                    // x to screen coordinate
                    int pixelX = x + m + (i * 8 * multiplier) +
                                 (8 * multiplier) - (j * multiplier);

                    // blit pixel
                    int color = getSpriteColor(idx);
                    if (isSpriteDrawingOnBorder(pixelX, currentLine)) {
                        // draw using border color, transparent
                        color = _regBorderColor;
                    } else {
                        // if we're not drawing transparent, also check
                        // sprite-sprite collision
                        Rect limits;
                        getScreenLimits(&limits);
                        checkSpriteSpriteCollision(
                            idx, pixelX,
                            currentLine); // - limits.firstVisibleY);
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
    int defaultSpriteW = 24;
    Rect limits;
    getScreenLimits(&limits);

    // loop for the 8 hardware sprites
    for (int idx = 0; idx < 8; idx++) {
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
            // SDL_Log("xscroll=%d, xcoord=%d, x=%d", _scrollX, spriteXCoord,
            // x);
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
    if (rasterLine < limits.firstVisibleY || rasterLine > limits.lastVisibleY) {
        // out of display window
        return;
    }
    if (!_DEN) {
        // display enabled bit must be set
        return;
    }

    // get screen mode
    int screenMode = getScreenMode();

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
        int line = rasterLine - limits.firstVisibleY;

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
        if (screenMode == VIC_SCREEN_MODE_EXTENDED_BACKGROUND_COLOR) {
            // determine which background color register to be used
            // https://www.c64-wiki.com/wiki/Extended_color_mode
            if (!IS_BIT_SET(screenCode, 7) && !IS_BIT_SET(screenCode, 6)) {
                backgroundRgb = _palette[getBackgoundColor(0)];
            } else if (!IS_BIT_SET(screenCode, 7) &&
                       IS_BIT_SET(screenCode, 6)) {
                backgroundRgb = _palette[getBackgoundColor(1)];
            } else if (IS_BIT_SET(screenCode, 7) &&
                       !IS_BIT_SET(screenCode, 6)) {
                backgroundRgb = _palette[getBackgoundColor(2)];
            } else if (IS_BIT_SET(screenCode, 7) && IS_BIT_SET(screenCode, 6)) {
                backgroundRgb = _palette[getBackgoundColor(3)];
            }
            // clear bits in character
            screenCode &= 0x3f;
        } else {
            // default text mode
            backgroundRgb = _palette[getBackgoundColor(0)];
        }

        // read the character data and color
        uint8_t data = getCharacterData(screenCode, charRow);
        uint8_t charColor = getScreenColor(c, row);
        RgbStruct charRgb = _palette[charColor & 0xf];

        // draw character bit by bit
        if (screenMode == VIC_SCREEN_MODE_CHARACTER_MULTICOLOR) {
            for (int i = 0; i < 8; i++) {
                // only the last 3 bits count
                uint8_t bits = data & 3;
                switch (bits) {
                case 0:
                    // 00
                    charRgb = _palette[getBackgoundColor(0)];
                    break;

                case 1:
                    // 01
                    charRgb = _palette[getBackgoundColor(1)];
                    break;

                case 2:
                    // 10
                    charRgb = _palette[getBackgoundColor(2)];
                    break;

                case 3:
                    // 11, default (use background color, doc says use
                    // character color ??)
                    charRgb = _palette[getBackgoundColor(0)];
                    break;
                }
                int pixelX = x + 8 - i;
                blit(pixelX, rasterLine, &charRgb);
                blit(pixelX + 1, rasterLine, &charRgb);

                // each pixel is doubled
                i++;
                data >>= 2;
            }
        } else {
            // default text mode or extended background mode
            data = (data >> _scrollX);
            for (int i = 0; i < 8; i++) {
                int pixelX = x + 8 - i;
                if (pixelX > (320 + limits.firstVisibleX)) {
                    // do not draw off screen
                    blit(pixelX, rasterLine, &backgroundRgb);
                    continue;
                }

                if (IS_BIT_SET(data, i)) {
                    // put pixel in foreground color
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
    // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt 3.7.3.3
    Rect limits;
    getScreenLimits(&limits);

    if (rasterLine < limits.firstVisibleY || rasterLine > limits.lastVisibleY) {
        // out of display window
        return;
    }
    if (!_DEN) {
        // display enabled bit must be set
        return;
    }

    // get screen mode
    int screenMode = getScreenMode();

    // draw bitmap
    int columns = 40;
    for (int c = 0; c < columns; c++) {
        // this is the display window line
        int line = rasterLine - limits.firstVisibleY;

        // this is the first display window column for this this block
        int x = limits.firstVisibleX + (c * 8);

        // this is the first displaywindow row for this block
        int row = line / 8;

        // this is the row of thebitmap block itself (8x8)
        int bitmapRow = line % 8;

        // read screencode for standard color
        uint8_t screenChar = getScreenCode(c, row);

        // read color memory for multicolor
        uint8_t screenColor = getScreenColor(c, row);

        // read bitmap data
        uint8_t bitmapData = getBitmapData(c, row, bitmapRow);

        // draw bitmap
        if (screenMode == VIC_SCREEN_MODE_BITMAP_STANDARD) {
            // SDL_Log("drawing bitmap");
            // standard bitmap
            RgbStruct fgColor = _palette[(screenChar >> 4) & 0xf];
            RgbStruct bgColor = _palette[screenChar & 0xf];
            for (int i = 0; i < 8; i++) {
                int pixelX = x + 8 - i + _scrollX;
                if (pixelX > (320 + limits.firstVisibleX)) {
                    // do not draw off screen
                    blit(pixelX, rasterLine, &bgColor);
                    continue;
                }

                if (IS_BIT_SET(bitmapData, i)) {
                    blit(pixelX, rasterLine, &fgColor);
                } else {
                    blit(pixelX, rasterLine, &bgColor);
                }
            }
        } else {
            // multicolor bitmap
            // SDL_Log("drawing multicolor bitmap");

            for (int i = 0; i < 8; i++) {
                // only the last 3 bits count
                uint8_t bits = bitmapData & 3;
                RgbStruct rgb;
                switch (bits) {
                case 0:
                    // 00
                    rgb = _palette[getBackgoundColor(0)];
                    break;

                case 1:
                    // 01
                    rgb = _palette[(screenChar >> 4) & 0xf];
                    break;

                case 2:
                    // 10
                    rgb = _palette[screenChar & 0xf];
                    break;

                case 3:
                    rgb = _palette[screenColor & 0xf];
                    break;
                }
                int pixelX = x + 8 - i + _scrollX;
                blit(pixelX, rasterLine, &rgb);
                blit(pixelX + 1, rasterLine, &rgb);

                // each pixel is doubled
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
    RgbStruct borderRgb = _palette[_regBorderColor];
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
    limits->firstVisibleY = 51;
    limits->lastVisibleY = 250;
    if (!_RSEL) {
        limits->firstVisibleY = 55;
        limits->lastVisibleY = 246;
    }

    // @fixme the following X coordinates are merely found around in other
    // emulators..... but it's just a dirty hack

    // limits->firstVisibleX = 24;
    // @fixme: this is wrong, adjusted manually (+18). should be the
    // firstVisibleX taken from documentation.....
    limits->firstVisibleX = 42;

    limits->firstSpriteX = 18;

    limits->lastVisibleX = 343;
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
    if (IS_BIT_SET(_regInterrupt, 7)) {
        // IRQ bit is set, trigger an interrupt

        // and clear the bit
        // https://dustlayer.com/vic-ii
        BIT_CLEAR(_regInterrupt, 7);
        _cpu->irq();
    }

    // check for badline
    int currentRaster = getCurrentRasterLine();
    bool isBadLine = ((currentRaster >= 0x30) && (currentRaster <= 0xf7)) &&
                     ((currentRaster & 7) == (_scrollY & 7));
    if (currentRaster == 0x30 && _scrollY == 0 && IS_BIT_SET(_regCR1, 4)) {
        isBadLine = true;
    }

    // drawing a line always happens every 63 cycles on a PAL c64
    int elapsedCycles = cycleCount - _prevCycles;
    if (elapsedCycles < VIC_PAL_CYCLES_PER_LINE) {
        return 0;
    }

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
    if (currentRaster > limits.firstVblankLine &&
        currentRaster < limits.lastVblankLine) {
        drawBorder(currentRaster - limits.firstVblankLine);

        int screenMode = getScreenMode();
        if (screenMode == VIC_SCREEN_MODE_CHARACTER_STANDARD ||
            screenMode == VIC_SCREEN_MODE_CHARACTER_MULTICOLOR ||
            screenMode == VIC_SCREEN_MODE_EXTENDED_BACKGROUND_COLOR) {
            // draw screen line in character mode
            drawCharacterMode(currentRaster - limits.firstVblankLine);

        } else if (screenMode == VIC_SCREEN_MODE_BITMAP_STANDARD ||
                   screenMode == VIC_SCREEN_MODE_BITMAP_MULTICOLOR) {
            // draw bitmap
            drawBitmapMode(currentRaster - limits.firstVblankLine);
        }

        // draw sprites
        drawSprites(currentRaster - limits.firstVblankLine);
    }

    if (IS_BIT_SET(_regInterruptEnabled, 0)) {
        // handle raster interrupt
        if (currentRaster == _rasterIrqLine) {
            if (IS_BIT_SET(_regInterruptEnabled, 0)) {
                // trigger irq if bits in $d01a is set for the raster
                // interrupt
                BIT_SET(_regInterrupt, 0);
                _cpu->irq();
            } else {
                BIT_CLEAR(_regInterrupt, 0);
            }
        } else {
            BIT_CLEAR(_regInterrupt, 0);
        }
    }

    // update registers accordingly
    currentRaster++;
    setCurrentRasterLine(currentRaster);

    // return how many cycles drawing occupied
    _prevCycles = cycleCount;
    return isBadLine ? VIC_PAL_CYCLES_PER_BADLINE : VIC_PAL_CYCLES_PER_LINE;
}

void CVICII::read(uint16_t address, uint8_t *bt) {
    // check shadow and unused addresses
    uint16_t addr = checkShadowAddress(address);
    if (checkUnusedAddress(CPU_MEM_READ, addr, bt)) {
        return;
    }

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
        *bt = _regCR2;
        break;

    case 0xd017:
        // MnYE: sprite Y expanded
        *bt = _regSpriteYExpansion;
        break;

    case 0xd018:
        // memory pointers
        *bt = _regMemoryPointers;
        break;

    case 0xd019:
        // interrupt latch
        *bt = _regInterrupt;
        break;

    case 0xd01a:
        // interrupt enable register
        *bt = _regInterruptEnabled;
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
        *bt = _regBorderColor;
        break;

    case 0xd021:
    case 0xd022:
    case 0xd023:
    case 0xd024: {
        // BnC = background color 0-3
        int bcIdx = addr - 0xd021;
        *bt = getBackgoundColor(bcIdx);
        break;
    }

    case 0xd025:
    case 0xd026: {
        // MMn = sprite multicolor 0-1
        int mmIdx = addr - 0xd025;
        *bt = getSpriteMulticolor(mmIdx);
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
        *bt = getSpriteColor(mIdx);
        break;
    }

    default:
        // read memory
        _cpu->memory()->readByte(addr, bt);
    }
}

void CVICII::write(uint16_t address, uint8_t bt) {
    // check shadow
    uint16_t addr = checkShadowAddress(address);

    // on write these are unused
    if (checkUnusedAddress(CPU_MEM_WRITE, addr, nullptr)) {
        return;
    }

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
        _DEN = IS_BIT_SET(bt, 4);

        // YSCROLL
        _scrollY = bt & 0x7;

        // RSEL
        _RSEL = IS_BIT_SET(bt, 3);

        // bit 7 also sets bit 8 of the raster irq compare line (bit 8)
        if (IS_BIT_SET(bt, 7)) {
            BIT_SET(_rasterIrqLine, 8);
        }
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
        bt |= 0xc0;
        _regCR2 = bt;

        // XSCROLL (bit 0,1,2)
        _scrollX = bt & 0x7;

        // CSEL
        _CSEL = IS_BIT_SET(bt, 3);
        break;

    case 0xd017:
        // MnYE: sprite Y expanded
        _regSpriteYExpansion = bt;
        break;

    case 0xd018:
        // memory pointers (bit 0 is always set)
        // https://www.c64-wiki.com/wiki/53272
        bt |= 1;
        _regMemoryPointers = bt;

        // set addresses (https://www.c64-wiki.com/wiki/Page_208-211)
        // VM10,VM11,VM12,VM13 = character set address
        // The four most significant bits form a 4-bit number in the range 0
        // thru 15 Multiplied with 1024 this gives the start address for the
        // screen character RAM.
        _screenAddress = (_regMemoryPointers >> 4);
        _screenAddress *= 1024;

        // CB11, CB12, CB13 = character set address
        // Bits 1 thru 3 (weights 2 thru 8) form a 3-bit number in the range
        // 0 thru 7: Multiplied with 2048 this gives the start address for
        // the character set.
        _charsetAddress = (_regMemoryPointers & 0xe) >> 1;
        _charsetAddress *= 2048;

        // CB13 = bitmap address
        // The four most significant bits form a 4-bit number in the range 0
        // thru 15: Multiplied with 1024 this gives the start address for
        // the color information. Bit 3 indicates whether the bitmap begins
        // at start address 0 / $0(bit set to "0"),or at 8192 / $2000(bit
        // set to "1").
        //_bitmapAddress =
        if (IS_BIT_SET(_regMemoryPointers, 3)) {
            _bitmapAddress += 0x2000;
        } else {
            _bitmapAddress = 0;
        }
        break;

    case 0xd019:
        // interrupt latch
        // bit 4,5,6 are always set
        bt |= 0x70;
        _regInterrupt = bt;
        break;

    case 0xd01a:
        // interrupt enabled
        // bit 4,5,6 are always set
        bt |= 0x70;
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
        bt &= 0xf;
        _regBorderColor = bt;
        break;

    case 0xd021:
    case 0xd022:
    case 0xd023:
    case 0xd024: {
        // BnC = background color 0-3
        bt &= 0xf;
        int bcIdx = addr - 0xd021;
        setBackgoundColor(bcIdx, bt);
        break;
    }

    case 0xd025:
    case 0xd026: {
        // MMn = sprite multicolor 0-1
        bt &= 0xf;
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
        bt &= 0xf;
        int mIdx = addr - 0xd027;
        setSpriteColor(mIdx, bt);
        break;
    }

    default:
        break;
    }

    // write anyway
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
 * @brief get the background color from the BnC register
 * @param idx register index in the background color registers array
 * @return the background color 0-3
 */
uint8_t CVICII::getBackgoundColor(int idx) { return _regBC[idx]; }

/**
 * @brief set the background color in the BnC register (4 bits)
 * @param idx register index in the background color registers array
 * @param val the color to set
 */
void CVICII::setBackgoundColor(int idx, uint8_t val) {
    _regBC[idx] = (val & 0xf);
}

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
 * some addresses are shadowed and maps to other addresses
 * @param address the input address
 * @return the effective address
 */
uint16_t CVICII::checkShadowAddress(uint16_t address) {
    // check for shadow addresses
    if (address >= 0xd040 && address <= 0xd3ff) {
        // these are shadows for $d000-$d03f
        return (VIC_REGISTERS_START + ((address % VIC_REGISTERS_START) % 0x40));
    }
    return address;
}

/**
 * read/write to these addresses has no effect or returns a fixed value
 * @param type CPU_MEM_READ or CPU_MEM_WRITE
 * @param address
 * @param bt
 * @return true if it's an unused address
 */
bool CVICII::checkUnusedAddress(int type, uint16_t address, uint8_t *bt) {
    if (address >= 0xd02f && address <= 0xd03f) {
        if (type == CPU_MEM_READ) {
            // these locations always return 0xff on read
            *bt = 0xff;
        }

        // on write, it's ignored
        return true;
    }
    return false;
}

/**
 * @brief get X coordinate of sprite at idx, combining value of the MnX
 * with the corresponding bit of $d010 as MSB
 * @return the effective X coordinate
 */
uint16_t CVICII::getSpriteXCoordinate(int idx) {
    // get MnX
    uint16_t mnx = _regM[idx * 2];

    // set 8th bit if the bit is set in $d010
    if (IS_BIT_SET(_regMSBX, idx)) {
        mnx |= 0x100;
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
    return mny;
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
 * @brief get the screen mode and return one of the VIC_SCREEN_MODE_*
 * values
 * @return int one of the VIC_SCREEN_MODE_x, or -1 on error
 */
int CVICII::getScreenMode() {
    // refers to bitmasks at
    // https://www.c64-wiki.com/wiki/Standard_Character_Mode
    if (!IS_BIT_SET(_regCR1, 6) && !IS_BIT_SET(_regCR1, 5) &&
        !IS_BIT_SET(_regCR2, 4)) {
        // SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "selecting standard
        // character mode");
        return VIC_SCREEN_MODE_CHARACTER_STANDARD;
    }
    if (!IS_BIT_SET(_regCR1, 6) && !IS_BIT_SET(_regCR1, 5) &&
        IS_BIT_SET(_regCR2, 4)) {
        // SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "selecting
        // multicolor character mode");
        // @fixme: here returning VIC_SCREEN_MODE_CHARACTER_STANDARD fix
        // some displays..... but it's wrong, there's some bug somewhere
        // else! return VIC_SCREEN_MODE_CHARACTER_STANDARD;
        return VIC_SCREEN_MODE_CHARACTER_MULTICOLOR;
    }
    if (!IS_BIT_SET(_regCR1, 6) && IS_BIT_SET(_regCR1, 5) &&
        !IS_BIT_SET(_regCR2, 4)) {
        // SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "selecting standard
        // bitmap mode");
        return VIC_SCREEN_MODE_BITMAP_STANDARD;
    }
    if (!IS_BIT_SET(_regCR1, 6) && IS_BIT_SET(_regCR1, 5) &&
        IS_BIT_SET(_regCR2, 4)) {
        // SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "selecting
        // multicolor bitmap mode");
        return VIC_SCREEN_MODE_BITMAP_MULTICOLOR;
    }
    if (IS_BIT_SET(_regCR1, 6) && !IS_BIT_SET(_regCR1, 5) &&
        !IS_BIT_SET(_regCR2, 4)) {
        // SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "selecting extended
        // background color mode");
        return VIC_SCREEN_MODE_EXTENDED_BACKGROUND_COLOR;
    }
    // SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "invalid color mode!!
    // CR1=%x, CR2=%x", _regCR1, _regCR2);

    return -1;
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
    _cpu->memory()->readByte(screenCodeAddr, &screenCode);
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
    _cpu->memory()->readByte(colorAddr, &screenColor);
    return screenColor;
}

/**
 * @brief get a row (8x8) of a character from character set
 * @param x x screen coordinate
 * @param y y screen coordinate
 * @param charRow the row inside the character bitmap, 0-8
 * @return
 */
uint8_t CVICII::getCharacterData(int screenCode, int charRow) {
    // get character set address, handling character ROM shadow
    // (https://www.c64-wiki.com/wiki/VIC_bank)
    // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt 2.4.2
    int bank = _cia2->vicBank();
    uint8_t *cAddr = nullptr;
    uint8_t data = 0;
    // SDL_Log("bank=%d, charsetAddress=$%x, vicMemoryAddress=$%x",
    // bank,
    //      _charsetAddress, _cia2->vicMemoryAddress());
    if ((bank == 0 && _charsetAddress == 0x1000) ||
        (bank == 2 && _charsetAddress == 0x9000)) {
        // ROM address
        cAddr = ((CMemory *)_cpu->memory())->charset();

        // read char data
        data = cAddr[(screenCode * 8) + charRow];
    } else {
        // default
        uint16_t addr = _cia2->vicMemoryAddress() + _charsetAddress;
        addr += ((screenCode * 8) + charRow);

        // raw memory read
        uint8_t *raw = _cpu->memory()->raw();
        data = raw[addr];
    }
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
    uint16_t bitmapAddr = _bitmapAddress + (((y * 40) + x) * 8) + bitmapRow;
    uint8_t data;
    _cpu->memory()->readByte(bitmapAddr, &data);
    return data;
}
