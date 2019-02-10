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
    _rasterCounter = 0;
    _rasterIrqLine = 0;

    // initialize to default values
    _displayH = 200;
    _displayW = 320;
    _borderHSize = (VIC_PAL_SCREEN_W - _displayW) / 2;
    _borderVSize = (VIC_PAL_SCREEN_H - _displayH) / 2;

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

void CVICII::updateRasterCounter() {
    uint8_t cr1;
    _cpu->memory()->readByte(VIC_REG_CR1, &cr1);
    if (_rasterCounter <= 0xff) {
        // raster counter is still 8 bit
        _cpu->memory()->writeByte(VIC_REG_RASTERCOUNTER, _rasterCounter & 0xff);
    }
    else {
        // bit 8 of the raster counter is set into bit 7 of cr1
        cr1 |= (_rasterCounter >> 8) << 7;
        _cpu->memory()->writeByte(VIC_REG_RASTERCOUNTER, _rasterCounter & 0xff);
    }

    // rewrite both
    _cpu->memory()->writeByte(VIC_REG_RASTERCOUNTER, _rasterCounter & 0xff);
    _cpu->memory()->writeByte(VIC_REG_CR1, cr1);
}

void CVICII::drawBorderRow(int row) {
    // get border color
    uint8_t borderColor;
    _cpu->memory()->readByte(VIC_REG_BORDER_COLOR, &borderColor);
    rgbStruct borderRgb = _palette[borderColor];

    // draw border row
    for (int i=0; i < VIC_PAL_SCREEN_W; i++ ) {
        int pos = ((row*VIC_PAL_SCREEN_W) + i);
        _fb[pos]=SDL_MapRGB(_sdlCtx->pxFormat, borderRgb.r, borderRgb.g, borderRgb.b);
    }
}


int CVICII::update(int cycleCount) {
    int occupiedCycles = 0;

    uint8_t d019;
    _cpu->memory()->readByte(VIC_REG_INTERRUPT, &d019);
    uint8_t d01a;
    _cpu->memory()->readByte(VIC_REG_IRQ_ENABLED, &d01a);
    uint8_t cr1;
    _cpu->memory()->readByte(VIC_REG_CR1, &cr1);

    if (IS_BIT_SET(d019,7)) {
        // IRQ is set
        _cpu->irq();
    }

    if ((cycleCount % VIC_CYCLES_PER_LINE) != 0) {
        return 0;
    }

    // increment the raster counter
    _rasterCounter++;
    updateRasterCounter();

    // check if interrupt is enabled
    if ( IS_BIT_SET(d01a, 7)) {
        // interrupt enabled, generate a raster interrupt if needed
        if (_rasterCounter == _rasterIrqLine) {
            _cpu->irq();
        }
            /*if (_rasterCounter ==)
                _cpu->irq();
                */
    }

    if (_rasterCounter >= VIC_FIRST_VISIBLE_LINE && _rasterCounter <=VIC_LAST_VISIBLE_LINE) {
        // draw border row
        int borderRow = _rasterCounter - 14;
        drawBorderRow(borderRow);
    }

    if (_rasterCounter >= VIC_PAL_SCANLINES_PER_VBLANK) {
        // reset raster counter
        _rasterCounter = 0;
        updateRasterCounter();
    }

    // check for bad line, taken from http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
    /*
         A Bad Line Condition is given at any arbitrary clock cycle, if at the
         negative edge of ø0 at the beginning of the cycle RASTER >= $30 and RASTER
         <= $f7 and the lower three bits of RASTER are equal to YSCROLL and if the
         DEN bit was set during an arbitrary cycle of raster line $30.
     */
    int scrollY = cr1 & 7;
    bool isBadLine = ((_rasterCounter >= 0x30) && (_rasterCounter <= 0xf7)) && (_rasterCounter & 7 == scrollY);
    if (_rasterCounter == 0x30) {
        isBadLine = (IS_BIT_SET(cr1, 4));
    }
    if (isBadLine) {
        occupiedCycles = VIC_CYCLES_PER_BADLINE;
    }
    else {
        occupiedCycles = VIC_CYCLES_PER_LINE;
    }
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
        case VIC_REG_RASTERCOUNTER: {
            // sets the raster line at which the interrupt must happen, needs also bit 7 from cr1
            uint8_t cr1;
            _cpu->memory()->readByte(VIC_REG_CR1, &cr1);
            _rasterIrqLine |= (cr1 >> 7);
        }
            break;

        case VIC_REG_CR1:
            // setting cr1 also affects the raster counter (bit 8 of the raster counter is bit 7 of cr1)
            _rasterIrqLine |= (bt >> 7);

            // RSEL
            if (IS_BIT_SET(bt, 3)) {
                _displayH = 200;
            }
            else {
                _displayH = 192;
            }
            _borderVSize = (VIC_PAL_SCREEN_H - _displayH) / 2;

            // YSCROLL
            _scrollY = bt & 0x7;

            break;
        case VIC_REG_CR2:
            // CSEL
            if (IS_BIT_SET(bt, 3)) {
                _displayW = 320;
            }
            else {
                _displayW = 304;
            }
            _borderHSize = (VIC_PAL_SCREEN_W - _displayW) / 2;

            // XSCROLL
            _scrollX = (bt & 0x7);
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

/**
 * update the screen
 */
void CVICII::updateScreen() {
    // check cr1 and cr2
    uint8_t cr1;
    uint8_t cr2;
    _cpu->memory()->readByte(VIC_REG_CR1, &cr1);
    _cpu->memory()->readByte(VIC_REG_CR2, &cr2);
    if (!IS_BIT_SET(cr1, 6) && !IS_BIT_SET(cr1, 5)) {
        if (!IS_BIT_SET(cr2, 5)) {
            updateScreenCharacterMode(_fb);
        }
    }
    else {
        CLog::fatal("screen mode not yet implemented!");
    }
}

/**
 * update screen in character (low resolution) mode (40x25)
 * @param frameBuffer the framebuffer memory to be updated
 */
void CVICII::updateScreenCharacterMode(uint32_t *frameBuffer) {
    // get the addresses with the help of the cia-2
    uint16_t charsetAddress;
    uint16_t screenAddress;
    uint8_t* colorMem = _cpu->memory()->raw() + MEMORY_COLOR_ADDRESS;
    getTextModeScreenAddress(&screenAddress,&charsetAddress);

    // check for multicolor mode
    uint8_t cr2;
    _cpu->memory()->readByte(VIC_REG_CR2, &cr2);
    bool multicolor = IS_BIT_SET(cr2, 4);
    if (multicolor) {
#ifdef VIC_DEBUG
        CLog::print("multicolor mode!");
#endif
    }

    uint8_t* charset = _cpu->memory()->raw() + charsetAddress;
    if (charsetAddress == VIC_REGISTERS_START) {
        // bank3 is used, use the ROM charset
        charset = ((CMemory*)_cpu->memory())->charset();
    }

    // scroll
    uint8_t cr1;
    _cpu->memory()->readByte(VIC_REG_CR1, &cr1);

    // draw the screen character per character, left to right
    int numColumns = 40;
    int numRows = 25;
    for (int r=0; r < numRows; r++) {
        for (int c=0; c < numColumns; c++) {
            // read this screen code color
            uint8_t charColor = (colorMem[(r * numRows) + c]);
            rgbStruct cRgb = _palette[charColor & 0xf];

            // read background color
            uint8_t bgColor;
            _cpu->memory()->readByte(VIC_REG_BG_COLOR_0, &bgColor);
            rgbStruct bRgb = _palette[bgColor & 0xf];

            // read screencode in video memory -> each screencode corresponds to a character in the charset memory
            uint8_t screenCode;
            _cpu->memory()->readByte(screenAddress + (r*numColumns) + c, &screenCode);

            // each character in the charset is 8x8
            for (int charRow=0; charRow < 8; charRow ++) {
                uint8_t rowData = charset[(screenCode*8) + charRow];
                for (int charColumn = 0; charColumn < 8; charColumn ++) {
                    // blit it pixel per pixel in the framebuffer
                    int pixX=(c * 8) + charColumn + _borderHSize;
                    int pixY=(r * 8) + charRow + _borderVSize;
                    int pos = ((pixY*VIC_PAL_SCREEN_W) + pixX);

                    if (multicolor) {
                        // https://www.c64-wiki.com/wiki/Character_set#Defining_a_multi-color_character
                        if (!(IS_BIT_SET(rowData, 7))) {
                            if (!(IS_BIT_SET(rowData, 6))) {
                                // 00 background, use background color
                                frameBuffer[pos] = SDL_MapRGB(_sdlCtx->pxFormat, bRgb.r, bRgb.g, bRgb.b);
                                frameBuffer[pos + 1] = SDL_MapRGB(_sdlCtx->pxFormat, bRgb.r, bRgb.g, bRgb.b);
                            } else {
                                // 01, read from background color 1 register
                                uint8_t d022;
                                _cpu->memory()->readByte(VIC_REG_BG_COLOR_1, &d022);
                                rgbStruct rgb = _palette[d022 & 0xf];
                                frameBuffer[pos] = SDL_MapRGB(_sdlCtx->pxFormat, rgb.r, rgb.g, rgb.b);
                                frameBuffer[pos + 1] = SDL_MapRGB(_sdlCtx->pxFormat, rgb.r, rgb.g, rgb.b);
                            }
                        } else {
                            if (IS_BIT_SET(rowData, 6)) {
                                // 11, use background color
                                // TODO: according to https://www.c64-wiki.com/wiki/Character_set#Defining_a_multi-color_character,
                                //  should be the character color instead (cRgb) ?
                                // but, checking with vice output, this needs to be bRgb.....
                                frameBuffer[pos] = SDL_MapRGB(_sdlCtx->pxFormat, bRgb.r, bRgb.g, bRgb.b);
                                frameBuffer[pos + 1] = SDL_MapRGB(_sdlCtx->pxFormat, bRgb.r, bRgb.g, bRgb.b);
                            } else {
                                // 10, read from background color 2 register
                                uint8_t d023;
                                _cpu->memory()->readByte(VIC_REG_BG_COLOR_2, &d023);
                                rgbStruct rgb = _palette[d023 & 0xf];
                                frameBuffer[pos] = SDL_MapRGB(_sdlCtx->pxFormat, rgb.r, rgb.g, rgb.b);
                                frameBuffer[pos + 1] = SDL_MapRGB(_sdlCtx->pxFormat, rgb.r, rgb.g, rgb.b);
                            }
                        }

                        // next bit pair
                        rowData <<= 2;
                        charColumn++;
                    }
                    else {
                        // standard text mode
                        // https://www.c64-wiki.com/wiki/Standard_Character_Mode
                        if ( IS_BIT_SET(rowData, 7) ) {
                            // bit is set, set foreground color
                            frameBuffer[pos]=SDL_MapRGB(_sdlCtx->pxFormat, cRgb.r, cRgb.g, cRgb.b);
                        }
                        else {
                            frameBuffer[pos]=SDL_MapRGB(_sdlCtx->pxFormat, bRgb.r, bRgb.g, bRgb.b);
                        }

                        // next bit
                        rowData <<= 1;
                    }
                }
            }
        }
    }
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

