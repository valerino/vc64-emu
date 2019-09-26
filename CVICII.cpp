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

/**
 * update the internal representation of the raster counter ($d012) and control
 * register 1 ($d011)
 */
void CVICII::updateRasterCounter() {
    if (_regRASTER > 0xff) {
        // bit 8 of the raster counter is set into bit 7 of cr1
        _regCR1 |= (_regRASTER >> 8) << 7;
    }

    // rewrite both
    _cpu->memory()->writeByte(0xd012, _regRASTER & 0xff);
    _cpu->memory()->writeByte(0xd011, _regCR1);
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

/**
 * draw a character mode line
 * @param rasterLine the rasterline index to draw
 */
void CVICII::drawCharacterMode(int rasterLine) {
    if (_regRASTER < VIC_PAL_FIRST_DISPLAYWINDOW_LINE) {
        // out of screen
        return;
    }
    if (_regRASTER >= VIC_PAL_LAST_DISPLAYWINDOW_LINE) {
        // out of screen
        return;
    }
    if (!(IS_BIT_SET(_regCR1, 4))) {
        // display enabled bit must be set
        return;
    }

    // get screen and character set address
    uint16_t charsetAddress;
    uint16_t screenAddress;
    uint8_t *colorMem = _cpu->memory()->raw() + MEMORY_COLOR_ADDRESS;
    int bank;
    getCharacterModeScreenAddress(&screenAddress, &charsetAddress, &bank);
    uint8_t *charset = _cpu->memory()->raw() + charsetAddress;

    // handle character shadows
    // https://www.c64-wiki.com/wiki/VIC_bank
    if (bank == 0 && charsetAddress == 0x1000) {
        charset = ((CMemory *)_cpu->memory())->charset();
    } else if (bank == 2 && charsetAddress == 0x9000) {
        charset = ((CMemory *)_cpu->memory())->charset();
    }

    // draw characters
    int columns = VIC_CHAR_MODE_COLUMNS;
    for (int c = 0; c < columns; c++) {
        if (!(IS_BIT_SET(_regCR2, 3))) {
            // 38 columns, skip drawing column 0 and 39
            if (c == 0 || c == VIC_CHAR_MODE_COLUMNS - 1) {
                continue;
            }
        }
        int x = VIC_PAL_FIRST_DISPLAYWINDOW_COLUMN + (c * 8);
        int line = _regRASTER - VIC_PAL_FIRST_DISPLAYWINDOW_LINE;

        // screen row from raster line
        int row = line / 8;

        // character row
        int charRow = line % 8;

        // read screenchode from screen memory
        uint8_t screenChar;
        _cpu->memory()->readByte(screenAddress + (row * columns) + c,
                                 &screenChar);

        // read the character data and color
        uint8_t data = charset[(screenChar * 8) + charRow];
        uint8_t charColor = (colorMem[(row * columns) + c]);
        rgbStruct charRgb = _palette[charColor & 0xf];

        // check for multicolor mode
        bool multicolor = IS_BIT_SET(_regCR2, 4);

        // draw character bit by bit
        if (multicolor) {
            // https://www.c64-wiki.com/wiki/Character_set#Defining_a_multi-color_character
            for (int i = 0; i < 8; i++) {
                // only the last 3 bits count
                uint8_t bits = data & 3;
                switch (bits) {
                case 0:
                    // 00
                    charRgb = _palette[_regBackgroundColor0];
                    break;

                case 1:
                    // 01
                    charRgb = _palette[_regBackgroundColor1];
                    break;

                case 2:
                    // 10
                    charRgb = _palette[_regBackgroundColor2];
                    break;

                case 3:
                    // 11, default (use background color, doc says use character
                    // color ??)
                    charRgb = _palette[_regBackgroundColor0];
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
            data = (data >> _scrollX);
            for (int i = 0; i < 8; i++) {
                int pixelX = x + 8 - i;
                if (IS_BIT_SET(data, i)) {
                    // put pixel
                    blit(pixelX, rasterLine, &charRgb);
                } else {
                    // put pixel in background color
                    rgbStruct backgroundRgb = _palette[_regBackgroundColor0];
                    blit(pixelX, rasterLine, &backgroundRgb);
                }
            }
        }
    }
}

int CVICII::update(long cycleCount) {
    if (IS_BIT_SET(_regInterrupt, 7)) {
        // IRQ is set
        _cpu->irq();
    }

    long elapsed = cycleCount - _prevCycles;

    // check for bad line (http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt)
    /*
         A Bad Line Condition is given at any arbitrary clock cycle, if at the
         negative edge of ø0 at the beginning of the cycle RASTER >= $30 and
       RASTER
         <= $f7 and the lower three bits of RASTER are equal to YSCROLL and if
       the DEN bit was set during an arbitrary cycle of raster line $30.
     */
    int occupiedCycles = 0;
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
    if (elapsed >= occupiedCycles) {
        int diff = elapsed - occupiedCycles;
        occupiedCycles += diff;
        _prevCycles = cycleCount;
    } else {
        return 0;
    }

    // generate a raster interrupt if needed
    // FIXME: this is probably wrong, but removing the regInterruptEnabled check
    // makes the cursor blink normally ....
    // if (IS_BIT_SET(_regInterruptEnabled, 0)) {
    if (_regRASTER == _rasterIrqLine) {
        _cpu->irq();
        // reset the interrupt latch register by hand
        // (http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt)
        /*
         * There are four interrupt sources in the VIC. Every source has a
            corresponding bit in the interrupt latch (register $d019) and a
         bit in the interrupt enable register ($d01a). When an interrupts
         occurs, the corresponding bit in the latch is set. To clear it, the
         processor has to write a "1" there "by hand". The VIC doesn't clear
         the latch on its own.
         */
        _regInterrupt |= 1; // set bit 0, IRST
        _cpu->memory()->writeByte(0xd019, _regInterrupt);
        //}
    }

    if (_regRASTER >= VIC_PAL_FIRST_VISIBLE_LINE &&
        _regRASTER <= VIC_PAL_LAST_VISIBLE_LINE) {
        // draw border line
        int rasterLine = _regRASTER - VIC_PAL_FIRST_VISIBLE_LINE;
        drawBorder(rasterLine);

        if (isCharacterMode()) {
            // character mode
            drawCharacterMode(rasterLine);
        }
    }

    // increment the raster counter
    _regRASTER++;
    if (_regRASTER >= VIC_PAL_SCANLINES) {
        // reset raster counter
        _regRASTER = 0;
    }

    updateRasterCounter();
    return occupiedCycles;
}

void CVICII::read(uint16_t address, uint8_t *bt) {
    // check shadow
    uint16_t addr = checkShadowAddress(address);

    if (checkUnusedAddress(CPU_MEM_READ, addr, bt)) {
        return;
    }

    switch (addr) {
    // control register 1
    case 0xd016:
        // bit 6 and 7 are always set
        _cpu->memory()->readByte(addr, bt);
        *bt |= 0xc0;
        break;

        // interrupt and interrupt enabled registers
    case 0xd019:
        // bit 4,5,6 are always set
        // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
        _cpu->memory()->readByte(addr, bt);
        *bt |= 0x70;
    case 0xd01a:
        // bit 4,5,6,7 are always set
        // http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
        _cpu->memory()->readByte(addr, bt);
        *bt |= 0xf0;
        break;

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
    case 0xd011:
        // CR1
        _regCR1 = bt;

        // setting cr1 also affects the raster counter (bit 8 of the raster
        // counter is bit 7 of cr1)
        _rasterIrqLine |= (bt >> 7);

        // YSCROLL
        _scrollY = bt & 0x7;
        break;

        // control register 2
    case 0xd016:
        _regCR2 = bt | 0xc0;

        // XSCROLL
        _scrollX = bt & 0x7;
        break;

    // interrupt register
    case 0xd019:
        _regInterrupt = bt;
        break;

        // interrupt enabled
    case 0xd01a:
        _regInterruptEnabled = bt;
        break;

        // raster counter
    case 0xd012:
        // sets the raster line at which the interrupt must happen, needs
        // also bit 7 from cr1
        _rasterIrqLine = bt | (_regCR1 >> 7);
        //_regRASTER = bt | (_regCR1 >> 7);
        break;

    case 0xd020:
        // EC
        _regBorderColor = bt & 0xf;
        break;

    case 0xd021:
        //  B0C
        _regBackgroundColor0 = bt & 0xf;
        break;

    case 0xd022:
        //  B1C
        _regBackgroundColor1 = bt & 0xf;
        break;

        // background color 2
    case 0xd023:
        //  B2C
        _regBackgroundColor2 = bt & 0xf;
        break;

        // background color 3
    case 0xd024:
        //  B2C
        _regBackgroundColor2 = bt & 0xf;
        break;

    default:
        break;
    }

    // finally write
    _cpu->memory()->writeByte(addr, bt);
}

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

void CVICII::getCharacterModeScreenAddress(uint16_t *screenCharacterRamAddress,
                                           uint16_t *charsetAddress,
                                           int *bank) {
    // read base register
    uint8_t d018;
    _cpu->memory()->readByte(0xd018, &d018);

    // get screen address
    uint16_t base = _cia2->vicAddress();
    *bank = _cia2->vicBank();
    *screenCharacterRamAddress = ((d018 >> 4) & 0xf) * 1024;
    *charsetAddress = ((((d018 & 0xf) >> 1) & 0x7) * 2048) + base;
}

void CVICII::getBitmapModeScreenAddress(uint16_t *colorInfoAddress,
                                        uint16_t *bitmapAddress) {
    // TODO: implement
}

/**
 * @brief check for character mode display
 * @return
 */
bool CVICII::isCharacterMode() {
    if (!IS_BIT_SET(_regCR1, 6) && !IS_BIT_SET(_regCR1, 5)) {
        if (!IS_BIT_SET(_regCR2, 5)) {
            return true;
        }
    }
    return false;
}

int CVICII::getScreenMode() {
    /*

Table 2 - Graphics Modes VIC-II Bit Combinations
Mode	ECM	BMM	MCM	Result
0	0	0	0	Standard Character Mode
1	0	0	1	Multicolor Character Mode
2	0	1	0	Standard Bitmap Mode
3	0	1	1	Multicolor Bitmap Mode
4	1	0	0	Extended Background Color Mode
    */
    return 0;
}
