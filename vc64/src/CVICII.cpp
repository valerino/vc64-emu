//
// Created by valerino on 19/01/19.
//

#include "CVICII.h"
#include "CMemory.h"
#include <bitutils.h>
#include <CLog.h>
#include "globals.h"

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_VIC
#endif

CVICII::CVICII(CMOS65xx *cpu, CCIA2 *cia2) {
    _cpu = cpu;
    _cia2 = cia2;

    // initialize to default values
    _regBorderColor = 0;
    _regCr1 = 0;
    _regCr2 = 0;
    _rasterCounter = 0;
    _rasterIrqLine = 0;
    _regInterruptEnabled = 0;
    _regInterruptLatch = 0;
    _scrollX = 0;
    _scrollY = 0;
    _firstX=24;
    _lastX=343;
    _firstY=51;
    _lastY=250;
    for (int i=0; i < sizeof(_regBackgroundColors) / sizeof(uint8_t); i++) {
        _regBackgroundColors[i]=0;
    }

    // initialize color palette
    // https://www.c64-wiki.com/wiki/Color
    _palette[0] = {0,0,0};
    _palette[1] = {0xff,0xff,0xff};
    _palette[2] = {0x88,0,0};
    _palette[3] = {0xaa,0xff,0xee};
    _palette[4] = {0xcc,0x44,0xcc};
    _palette[5] = {0x00,0xcc,0x55};
    _palette[6] = {0,0,0xaa};
    _palette[7] = {0xee,0xee,0x77};
    _palette[8] = {0xdd,0x88,0x55};
    _palette[9] = {0x66,0x44,0};
    _palette[10] = {0xff,0x77,0x77};
    _palette[11] = {0x33,0x33,0x33};
    _palette[12] = {0x77,0x77,0x77};
    _palette[13] = {0xaa,0xff,0x66};
    _palette[14] = {0,0x88,0xff};
    _palette[15] = {0xbb,0xbb,0xbb};

}

CVICII::~CVICII() {

}

 /**
  * blit into the framebuffer
  * @param x x coordinate
  * @param y y coordinate
  * @param rgb rgb color
  */
void CVICII::blit(int x, int y, CVICII::rgbStruct *rgb) {
    int pos = (((y + _scrollY)*VIC_PAL_SCREEN_W) + (x + _scrollX));
    if (y >= VIC_PAL_SCREEN_H) {
        // not visible, vblank
        return;
    }
    _fb[pos]=SDL_MapRGB(_sdlCtx->pxFormat, rgb->r, rgb->g, rgb->b);
}

/**
 * update the internal representation of the raster counter ($d012) and control register 1 ($d011)
 */
void CVICII::updateRasterCounter() {
    if (_rasterCounter > 0xff) {
        // bit 8 of the raster counter is set into bit 7 of cr1
        _regCr1 |= (_rasterCounter >> 8) << 7;
    }

    // rewrite both
    _cpu->memory()->writeByte(VIC_REG_RASTERCOUNTER, _rasterCounter & 0xff);
    _cpu->memory()->writeByte(VIC_REG_CR1, _regCr1);
}

/**
 * draw a border line
 * @param rasterLine the rasterline index to draw
 */
void CVICII::drawBorder(int rasterLine) {
    // draw border row
    rgbStruct borderRgb = _palette[_regBorderColor];
    for (int i=0; i < VIC_PAL_SCREEN_W; i++ ) {
        blit(i, rasterLine, &borderRgb);
    }
}

/**
 * draw a character mode line
 * @param rasterLine the rasterline index to draw
 */
void CVICII::drawCharacterMode(int rasterLine) {
    if (_rasterCounter < VIC_PAL_FIRST_DISPLAYWINDOW_LINE) {
        // out of screen
        return;
    }
    if (_rasterCounter >= VIC_PAL_LAST_DISPLAYWINDOW_LINE) {
        // out of screen
        return;
    }
    if (! (IS_BIT_SET(_regCr1, 4))) {
        // display enabled bit must be set
        return;
    }

    // get screen and character set address
    uint16_t charsetAddress;
    uint16_t screenAddress;
    uint8_t *colorMem = _cpu->memory()->raw() + MEMORY_COLOR_ADDRESS;
    getTextModeScreenAddress(&screenAddress, &charsetAddress);
    uint8_t *charset = _cpu->memory()->raw() + charsetAddress;
    if (charsetAddress == VIC_REGISTERS_START) {
        // bank3 is used, use the ROM charset
        charset = ((CMemory *) _cpu->memory())->charset();
    }

    // draw characters
    int columns = VIC_CHAR_MODE_COLUMNS;
    for (int c = 0; c < columns; c++) {
        // check 38 columns
        if (!(IS_BIT_SET(_regCr2, 3))) {
            if (c == 0 || c == VIC_CHAR_MODE_COLUMNS - 1) {
                continue;
            }
        }
        int x = VIC_PAL_FIRST_DISPLAYWINDOW_COLUMN + (c * 8);
        int line = _rasterCounter - VIC_PAL_FIRST_DISPLAYWINDOW_LINE;

        // screen row from raster line
        int row = line / 8;

        // character row
        int charRow = line % 8;

        // read screenchode from screen memory
        uint8_t screenChar;
        _cpu->memory()->readByte(screenAddress + (row * columns) + c, &screenChar);

        // read the character data and color
        uint8_t data = charset[(screenChar * 8) + charRow];
        uint8_t charColor = (colorMem[(row * columns) + c]);
        rgbStruct charRgb = _palette[charColor & 0xf];

        // check for multicolor mode
        bool multicolor = IS_BIT_SET(_regCr2, 4);

        // draw character bit by bit
        if (multicolor) {
            // https://www.c64-wiki.com/wiki/Character_set#Defining_a_multi-color_character
            for(int i=0 ; i < 8 ; i++) {
                // only the last 3 bits count
                uint8_t bits = data & 3;
                switch (bits) {
                    case 0:
                        // 00
                        charRgb = _palette[_regBackgroundColors[0]];
                        break;

                   case 1:
                       // 01
                       charRgb = _palette[_regBackgroundColors[1]];
                       break;

                    case 2:
                        // 10
                        charRgb = _palette[_regBackgroundColors[2]];
                        break;

                    case 3:
                        // 11, default (use background color, doc says use character color ??)
                        charRgb = _palette[_regBackgroundColors[0]];
                        break;
                }
                int pixelX = x + 8 - i;
                blit(pixelX, rasterLine, &charRgb);
                blit(pixelX + 1, rasterLine, &charRgb);

                // each pixel is doubled
                i++;
                data >>= 2;
            }
        }
        else {
            for (int i=0; i < 8; i++) {
                int pixelX = x + 8 - i;
                if (IS_BIT_SET(data, i)) {
                    // put pixel
                    blit(pixelX, rasterLine, &charRgb);
                }
                else {
                    // put pixel in background color
                    rgbStruct backgroundRgb = _palette[_regBackgroundColors[0]];
                    blit(pixelX, rasterLine, &backgroundRgb);
                }
            }
        }
    }
}

int CVICII::update(int cycleCount) {
    int occupiedCycles = 0;

    if (IS_BIT_SET(_regInterruptLatch,7)) {
        // IRQ is set
        _cpu->irq();
    }

    // need to draw a line ?
    if ((cycleCount % VIC_PAL_CYCLES_PER_LINE) != 0) {
        return occupiedCycles;
    }

    // interrupt enabled, generate a raster interrupt if needed
    if ( IS_BIT_SET(_regInterruptEnabled, 0)) {
        if (_rasterCounter == _rasterIrqLine) {
            _cpu->irq();
            // reset the interrupt latch register by hand (http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt)
            /*
             * There are four interrupt sources in the VIC. Every source has a
                corresponding bit in the interrupt latch (register $d019) and a bit in the
                interrupt enable register ($d01a). When an interrupts occurs, the
                corresponding bit in the latch is set. To clear it, the processor has to
                write a "1" there "by hand". The VIC doesn't clear the latch on its own.
             */
            _regInterruptLatch |= 1;
            _cpu->memory()->writeByte(VIC_REG_INTERRUPT_LATCH, _regInterruptLatch);
        }
    }

    if (_rasterCounter >= VIC_PAL_FIRST_VISIBLE_LINE && _rasterCounter <=VIC_PAL_LAST_VISIBLE_LINE) {
        // draw border line
        int rasterLine = _rasterCounter - VIC_PAL_FIRST_VISIBLE_LINE;
        drawBorder(rasterLine);

        if (isCharacterMode()) {
            // character mode
            drawCharacterMode(rasterLine);
        }
    }

    // check for bad line (http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt)
    /*
         A Bad Line Condition is given at any arbitrary clock cycle, if at the
         negative edge of ø0 at the beginning of the cycle RASTER >= $30 and RASTER
         <= $f7 and the lower three bits of RASTER are equal to YSCROLL and if the
         DEN bit was set during an arbitrary cycle of raster line $30.
     */
    bool isBadLine = ((_rasterCounter >= 0x30) && (_rasterCounter <= 0xf7)) && ((_rasterCounter & 7) == (_scrollY & 7));
    if (_rasterCounter == 0x30 && _scrollY == 0 && IS_BIT_SET(_regCr1, 4)) {
        isBadLine = true;
    }
    if (isBadLine) {
        occupiedCycles += VIC_PAL_CYCLES_PER_BADLINE;
    }
    else {
        occupiedCycles += VIC_PAL_CYCLES_PER_LINE;
    }

    // increment the raster counter
    _rasterCounter++;
    if (_rasterCounter >= VIC_PAL_SCANLINES_PER_VBLANK) {
        // reset raster counter
        _rasterCounter = 0;
    }
    updateRasterCounter();
    return occupiedCycles;
}

void CVICII::read(uint16_t address, uint8_t* bt) {
    // check shadow
    uint16_t addr = checkShadowAddress(address);

    if (checkUnusedAddress(CPU_MEM_READ, addr, bt)) {
        return;
    }

    // finally read
    _cpu->memory()->readByte(addr,bt);
}

void CVICII::write(uint16_t address, uint8_t bt) {
    // check shadow
    uint16_t addr = checkShadowAddress(address);

    // on write these are unused
    if (checkUnusedAddress(CPU_MEM_WRITE, addr, nullptr)) {
        return;
    }

    switch(addr) {
        case VIC_REG_RASTERCOUNTER:
            // sets the raster line at which the interrupt must happen, needs also bit 7 from cr1
            _rasterIrqLine = bt | (_regCr1 >> 7);
            break;

        case VIC_REG_BG_COLOR_0:
            _regBackgroundColors[0] = bt & 0xf;
            break;

        case VIC_REG_BG_COLOR_1:
            _regBackgroundColors[1] = bt & 0xf;
            break;

        case VIC_REG_BG_COLOR_2:
            _regBackgroundColors[2] = bt & 0xf;
            break;

        case VIC_REG_BG_COLOR_3:
            _regBackgroundColors[3] = bt & 0xf;
            break;

        case VIC_REG_BORDER_COLOR:
            _regBorderColor = bt & 0xf;
            break;

        case VIC_REG_INTERRUPT_LATCH:
            _regInterruptLatch = bt;
            break;

        case VIC_REG_IRQ_ENABLED:
            _regInterruptEnabled = bt;
            break;

        case VIC_REG_CR1:
            _regCr1 = bt;

            // setting cr1 also affects the raster counter (bit 8 of the raster counter is bit 7 of cr1)
            _rasterIrqLine |= (bt >> 7);

            // RSEL
            if (IS_BIT_SET(bt,3)) {
                _firstY=51;
                _lastY=250;
            }
            else {
                _firstY=55;
                _lastY=246;
            }

            // YSCROLL
            _scrollY = bt & 0x7;
            break;

        case VIC_REG_CR2:
            _regCr2 = bt;

            // CSEL
            if (IS_BIT_SET(bt,3)) {
                _firstX=24;
                _lastX=343;
            }
            else {
                _firstX=31;
                _lastX=334;
            }

            // XSCROLL
            _scrollX = bt & 0x7;
            break;

        default:
            break;
    }

    // finally write
    _cpu->memory()->writeByte(addr,bt);
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

void CVICII::getTextModeScreenAddress(uint16_t *screenCharacterRamAddress, uint16_t *charsetAddress) {
    // read base register
    uint8_t d018;
    _cpu->memory()->readByte(VIC_REG_BASE_ADDR, &d018);

    // get addresses
    uint16_t base;
    int bank = _cia2->getVicBank(&base);
    *screenCharacterRamAddress = ((d018 >> 4) & 0xf) * 1024;
    *charsetAddress = ((((d018 & 0xf) >> 1) & 0x7) * 2048) + base;
}

void CVICII::getBitmapModeScreenAddress(uint16_t *colorInfoAddress, uint16_t *bitmapAddress) {
    // TODO: implement
}

/**
 * set the SDL display context
 * @param ctx
 * @param frameBuffer
 */
void CVICII::setSdlCtx(SDLDisplayCtx *ctx, uint32_t* frameBuffer) {
    _fb = frameBuffer;
    _sdlCtx = ctx;
}

/**
 * check for character mode
 * @return
 */
bool CVICII::isCharacterMode() {
    if (!IS_BIT_SET(_regCr1, 6) && !IS_BIT_SET(_regCr1, 5)) {
        if (!IS_BIT_SET(_regCr2, 5)) {
            return true;
        }
    }
    return false;
}
