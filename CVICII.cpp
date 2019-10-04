//
// Created by valerino on 19/01/19.
//

#include "CVICII.h"
#include <SDL.h>
#include "CMemory.h"
#include "bitutils.h"

void CVICII::initPalette() {
    // initialize color palette
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
void CVICII::blit(int x, int y, rgbStruct *rgb) {
    if (y >= VIC_PAL_SCREEN_H) {
        // not visible, vblank
        return;
    }

    // set the pixel
    int pos = (y * VIC_PAL_SCREEN_W) + x;
    _cb(_displayObj, rgb, pos);
}

uint16_t CVICII::getSpriteDataAddress(int idx) {
    uint16_t screenAddress;
    getScreenAddress(&screenAddress);

    // https://www.c64-wiki.com/wiki/Sprite#Sprite_pointers
    // https://dustlayer.com/vic-ii

    // read sprite pointer for sprite idx
    uint16_t spP = screenAddress + 0x3f8 + idx;
    uint8_t ptr;
    _cpu->memory()->readByte(spP, &ptr);

    // get sprite data address from sprite pointer
    uint16_t addr = 64 * ptr;
    // SDL_Log("sprite address=$%x, idx=%d", addr, idx);
    return addr;
}

/**
 * @brief check if while drawing sprite we're hitting borders
 * @return
 */
bool CVICII::isSpriteDrawingOnBorder(int x, int y) {
    // from http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt (PAL c64)
    int firstLine = 51;
    int lastLine = 250;
    if (!_RSEL) {
        firstLine = 55;
        lastLine = 246;
    }

    int firstX = 24;
    int lastX = 343;
    if (!_CSEL) {
        firstX = 31;
        lastX = 334;
    }

    // check if we're drawing sprites on border
    // @todo this will obviously prevent to draw sprites on border. .... but
    // leave it as is for now
    if (x <= firstX) {
        return true;
    }
    if (y < firstLine) {
        return true;
    }
    if (x > lastX) {
        return true;
    }
    if (y >= lastLine) {
        return true;
    }
    return false;
}

/**
 * @brief draw sprite row (multicolor sprite)
 * @param rasterLine the rasterline index to draw
 * @param idx the sprite index
 * @param x x screen coordinates
 * @param row sprite row number
 */
void CVICII::drawSpriteMulticolor(int rasterLine, int idx, int x, int row) {
    int currentLine = rasterLine - VIC_PAL_FIRST_VISIBLE_LINE;
    uint16_t addr = getSpriteDataAddress(idx);
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
                // 00 = transparent
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
            }

            if (bits != 0) {
                // blit
                rgbStruct rgb = _palette[color];
                int pixelX = x + (i * 8) + (8 - (j * 2));
                blit(pixelX, currentLine, &rgb);
                blit(pixelX + 1, currentLine, &rgb);
            }
        }
    }
}

/**
 * @brief draw sprite row (mono sprite)
 * @param rasterLine the rasterline index to draw
 * @param idx the sprite index
 * @param x x screen coordinates
 * @param row sprite row number
 */
void CVICII::drawSprite(int rasterLine, int idx, int x, int row) {
    int currentLine = rasterLine - VIC_PAL_FIRST_VISIBLE_LINE;

    // get sprite data address
    uint16_t addr = getSpriteDataAddress(idx);

    // a sprite is 24x21, each row is 3 bytes
    for (int i = 0; i < 3; i++) {
        // read sprite byte
        uint8_t spByte;
        uint16_t a = addr + (row * 3) + i;
        // SDL_Log("reading sprite byte at $%x", a);
        _cpu->memory()->readByte(a, &spByte);

        // draw sprite row bit by bit, take into account sprite X expansion (2x
        // multiplier)
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
                        // draw using border color
                        color = _regBorderColor;
                    }
                    rgbStruct rgb = _palette[color & 0xf];
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
    int currentLine = rasterLine - VIC_PAL_FIRST_VISIBLE_LINE;
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
            // @todo i don't know why this works...... probably has something to
            // do with LP registers as explained at
            // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt ??? apparently,
            // the x coordinate is not enough and must be 'shifted' some pixels
            // to the right ....
            int spriteFirstX = 18;
            int x = spriteFirstX + spriteXCoord;
            // SDL_Log("LPX=%d, xcoord=%d, x=%d", _regLP[0], spriteXCoord, x);
            if (isSpriteHExpanded) {
                // handle Y expansion
                row = (rasterLine - spriteYCoord) / 2;
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
 * @brief draw a character mode line
 * @param rasterLine the rasterline index to draw
 */
void CVICII::drawCharacterMode(int rasterLine) {
    // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
    int firstLine = 51;
    int lastLine = 250;
    if (!_RSEL) {
        firstLine = 55;
        lastLine = 246;
    }

    // @fixme this should be 24
    int firstX = 42;
    if (!_CSEL) {
        // @fixme this should be 31
        firstX = 42;
    }
    if (_regRASTER < firstLine) {
        // out of screen
        return;
    }
    if (_regRASTER >= lastLine) {
        // out of screen
        return;
    }
    if (!_DEN) {
        // display enabled bit must be set
        return;
    }

    // get screen and character set address
    uint16_t charsetAddress;
    uint16_t screenAddress;
    uint8_t *colorMem = _cpu->memory()->raw() + MEMORY_COLOR_ADDRESS;
    int bank;
    getScreenAddress(&screenAddress, &charsetAddress, &bank);
    uint8_t *charset = _cpu->memory()->raw() + charsetAddress;

    // get screen mode
    int screenMode = getScreenMode();

    // handle character ROM shadow
    // https://www.c64-wiki.com/wiki/VIC_bank
    if (bank == 0 && charsetAddress == 0x1000) {
        charset = ((CMemory *)_cpu->memory())->charset();
    } else if (bank == 2 && charsetAddress == 0x9000) {
        charset = ((CMemory *)_cpu->memory())->charset();
    }

    // draw characters
    int columns = 40;
    for (int c = 0; c < columns; c++) {
        if (!_CSEL) {
            // 38 characters, skip drawing column 0 and 39
            if (c == 0 || c == 39) {
                continue;
            }
        }
        int x = firstX + (c * 8);
        int line = _regRASTER - firstLine;

        // screen row from raster line
        int row = line / 8;

        // character row
        int charRow = line % 8;

        // read screencode from screen memory
        uint8_t screenCharacterCode;
        _cpu->memory()->readByte(screenAddress + (row * columns) + c,
                                 &screenCharacterCode);

        rgbStruct backgroundRgb;
        if (screenMode == VIC_SCREEN_MODE_EXTENDED_BACKGROUND_COLOR) {
            // determine which background color register to be used
            // https://www.c64-wiki.com/wiki/Extended_color_mode
            if (!IS_BIT_SET(screenCharacterCode, 7) &&
                !IS_BIT_SET(screenCharacterCode, 6)) {
                backgroundRgb = _palette[getBackgoundColor(0)];
            } else if (!IS_BIT_SET(screenCharacterCode, 7) &&
                       IS_BIT_SET(screenCharacterCode, 6)) {
                backgroundRgb = _palette[getBackgoundColor(1)];
            } else if (IS_BIT_SET(screenCharacterCode, 7) &&
                       !IS_BIT_SET(screenCharacterCode, 6)) {
                backgroundRgb = _palette[getBackgoundColor(2)];
            } else if (IS_BIT_SET(screenCharacterCode, 7) &&
                       IS_BIT_SET(screenCharacterCode, 6)) {
                backgroundRgb = _palette[getBackgoundColor(3)];
            }
            // clear bits in character
            screenCharacterCode &= 0x3f;
        } else {
            // default text mode
            backgroundRgb = _palette[getBackgoundColor(0)];
        }

        // read the character data and color
        uint8_t data = charset[(screenCharacterCode * 8) + charRow];
        uint8_t charColor = (colorMem[(row * columns) + c]);
        rgbStruct charRgb = _palette[charColor & 0xf];

        // draw character bit by bit
        if (screenMode == VIC_SCREEN_MODE_CHARACTER_MULTICOLOR) {
            // https://www.c64-wiki.com/wiki/Character_set#Defining_a_multi-color_character
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
 * draw a border line
 * @param rasterLine the rasterline index to draw
 */
void CVICII::drawBorder(int rasterLine) {
    // draw border row
    rgbStruct borderRgb = _palette[_regBorderColor];
    for (int i = 0; i < VIC_PAL_SCREEN_W; i++) {
        blit(i, rasterLine, &borderRgb);
    }
}

int CVICII::update(long cycleCount) {
    if (IS_BIT_SET(_regInterrupt, 7)) {
        // IRQ bit is set, trigger an interrupt
        _cpu->irq();

        // and clear the bit
        // https://dustlayer.com/vic-ii
        BIT_CLEAR(_regInterrupt, 7);
    }

    // check for bad line
    // (http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt)
    int occupiedCycles = 0;
    long elapsed = cycleCount - _prevCycles;
    bool isBadLine = ((_regRASTER >= 0x30) && (_regRASTER <= 0xf7)) &&
                     ((_regRASTER & 7) == (_scrollY & 7));
    if (_regRASTER == 0x30 && _scrollY == 0 && IS_BIT_SET(_regCR1, 4)) {
        isBadLine = true;
    }
    if (isBadLine) {
        occupiedCycles = VIC_PAL_CYCLES_PER_BADLINE;
    } else {
        occupiedCycles = VIC_PAL_CYCLES_PER_LINE;
    }
    if (elapsed < occupiedCycles) {
        // not yet
        return 0;
    }

    // calculate how many cycles passed
    int diff = elapsed - occupiedCycles;
    occupiedCycles += diff;
    _prevCycles = cycleCount;

    // increment the raster counter
    _regRASTER++;
    if (_regRASTER >= VIC_PAL_FIRST_VISIBLE_LINE &&
        _regRASTER <= VIC_PAL_LAST_VISIBLE_LINE) {
        // draw border line
        int line = _regRASTER - VIC_PAL_FIRST_VISIBLE_LINE;
        drawBorder(line);

        int screenMode = getScreenMode();
        if (screenMode == VIC_SCREEN_MODE_CHARACTER_STANDARD ||
            screenMode == VIC_SCREEN_MODE_CHARACTER_MULTICOLOR ||
            screenMode == VIC_SCREEN_MODE_EXTENDED_BACKGROUND_COLOR) {
            // draw screen line in character mode
            drawCharacterMode(line);

            // draw sprites
            drawSprites(_regRASTER);
        }
    }

    // handle raster interrupt
    if (_regRASTER == _rasterIrqLine) {
        if (IS_BIT_SET(_regInterruptEnabled, 0) &&
            IS_BIT_SET(_regInterrupt, 0)) {
            // trigger irq if bits in d019/d01a are set for the raster
            // interrupt
            _cpu->irq();

            // sets bit 7, an interrupt has been triggered
            BIT_SET(_regInterrupt, 7);
        }
    }

    if (_regRASTER > 0xff) {
        // bit 8 of the raster counter is set into bit 7 of cr1
        BIT_SET(_regCR1, 7);
    }
    // write(0xd011, _regCR1);
    if (_regRASTER >= VIC_PAL_SCANLINES) {
        // reset raster counter
        _regRASTER = 0;
    }

    // return how many cycles drawing occupied
    return occupiedCycles;
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
        int lp = 0xd013 - addr;
        *bt = _regLP[lp];
        break;
    }

    case 0xd015:
        // MnE: sprite enabled
        *bt = _regSpriteEnabled;
        break;

    case 0xd016:
        // CR2
        // bit 6,7 are always set
        *bt = (_regCR2 | 0xc0);
        break;

    case 0xd017:
        // MnYE: sprite Y expanded
        *bt = _regSpriteYExpansion;
        break;

    case 0xd018:
        // memory pointers
        // bit 0 is always set
        *bt = (_regMemoryPointers | 1);
        break;

    case 0xd019:
        // interrupt latch
        // bit 4,5,6 are always set
        // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
        *bt = (_regInterrupt | 0x70);
        break;
    case 0xd01a:
        // interrupt enable register
        // bit 4,5,6 are always set
        // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
        *bt = (_regInterruptEnabled | 0x70);
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
        // Sprite-sprite collision, cannot be read
        *bt = 0;
        break;
    case 0xd01f:
        // Sprite-data collision
        *bt = 0;
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

        // setting cr1 also affects the raster irq line (sets 8th bit)
        _rasterIrqLine = _regRASTER | (bt >> 7);

        // display enable (DEN)
        _DEN = IS_BIT_SET(bt, 4);

        // YSCROLL
        _scrollY = bt & 0x7;

        // RSEL
        _RSEL = IS_BIT_SET(bt, 3);
        break;

    case 0xd012:
        // RASTER
        _regRASTER = bt;

        // set the raster line at which an IRQ must happen, using RASTER and
        // bit 7 of CR1 if set
        _rasterIrqLine = bt;
        if (IS_BIT_SET(_regCR1, 7)) {
            BIT_SET(_rasterIrqLine, 8);
        }
        break;

    case 0xd013:
    case 0xd014: {
        // lightpen X/Y coordinate
        int lp = 0xd013 - addr;
        _regLP[lp] = bt;
        break;
    }
    case 0xd015:
        // MnE: sprite enabled
        _regSpriteEnabled = bt;
        break;

    case 0xd016:
        // CR2
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
        // memory pointers
        _regMemoryPointers = bt;
        break;

    case 0xd019:
        // interrupt latch
        _regInterrupt = bt;
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
        // MnM: sprite-sprite collision
        _regSpriteSpriteCollision = bt;
        if (_regSpriteSpriteCollision) {
            // some sprite collide!
            BIT_SET(_regInterrupt, 2);
            if (IS_BIT_SET(_regInterruptEnabled, 2)) {
                _cpu->irq();
            }
        }
        break;

    case 0xd01f:
        // MnD: sprite-background collision
        _regSpriteDataCollision = bt;
        if (_regSpriteDataCollision) {
            // some sprite collide with background!
            BIT_SET(_regInterrupt, 1);
            if (IS_BIT_SET(_regInterruptEnabled, 1)) {
                _cpu->irq();
            }
        }
        break;

    case 0xd020:
        // EC
        _regBorderColor = bt & 0xf;
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

    default:
        break;
    }

    // finally write
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
void CVICII::setSpriteColor(int idx, uint8_t val) { _regM[idx] = (val & 0xf); }

/**
 * @brief get color of sprite n from the MnC register
 * @param idx register index in the sprite color registers array
 */
uint8_t CVICII::getSpriteColor(int idx) { return _regM[idx]; }

/**
 * @brief get color from sprite multicolor register MMn
 * @param idx register index in the sprite multicolor registers array
 * @param val the color to set
 */
void CVICII::setSpriteMulticolor(int idx, uint8_t val) {
    _regMM[idx] = (val & 0xf);
}

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

void CVICII::getScreenAddress(uint16_t *screenAddress, uint16_t *charsetAddress,
                              int *bank) {
    // read base register
    uint8_t d018;
    _cpu->memory()->readByte(0xd018, &d018);

    // get screen address
    uint16_t base = _cia2->vicAddress();

    if (bank) {
        // get bank
        *bank = _cia2->vicBank();
    }

    if (charsetAddress) {
        // get charset address
        *charsetAddress = ((((d018 & 0xf) >> 1) & 0x7) * 2048) + base;
    }

    // get screen address
    *screenAddress = ((d018 >> 4) & 0xf) * 1024;
}

void CVICII::getBitmapModeScreenAddress(uint16_t *colorInfoAddress,
                                        uint16_t *bitmapAddress) {
    // TODO: implement
}

/**
 * @brief get X coordinate of sprite at idx, combining value of the MnX with
 * the corresponding bit of $d010 as MSB
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
 * @brief get the screen mode and return one of the VIC_SCREEN_MODE_* values
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
        // SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "selecting multicolor
        // character mode");
        // @todo: here returning VIC_SCREEN_MODE_CHARACTER_STANDARD fix some
        // displays..... but it's wrong
        // return VIC_SCREEN_MODE_CHARACTER_STANDARD;
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
        // SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "selecting multicolor
        // bitmap mode");
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
