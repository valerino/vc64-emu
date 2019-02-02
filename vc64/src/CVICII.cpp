//
// Created by valerino on 19/01/19.
//

#include "CVICII.h"
#include "CMemory.h"
#include <bitutils.h>
#include <CLog.h>
#include "globals.h"

CVICII::~CVICII() {

}

void CVICII::updateRasterCounter(uint16_t cnt) {
    _rasterCounter = cnt;
    if (cnt <= 0xff) {
        // 8 bit
        _cpu->memory()->writeByte(VIC_REG_RASTERCOUNTER, cnt & 0xff);
    }
    else {
        // 9th bit of the raster counter is set into bit 7 of cr1
        uint8_t cr1;
        _cpu->memory()->readByte(VIC_REG_CR1, &cr1);
        cr1 |= (cnt >> 8) << 7;

        _cpu->memory()->writeByte(VIC_REG_RASTERCOUNTER, cnt & 0xff);
        _cpu->memory()->writeByte(VIC_REG_CR1, cr1);
    }
}

int CVICII::run(int cycleCount) {
    if ((cycleCount % CYCLES_PER_LINE) == 0) {
        // increment the raster counter
        updateRasterCounter(_rasterCounter + 1);
        if (_rasterCounter > VIC_PAL_SCANLINES_PER_VBLANK) {
            // reset raster counter
            updateRasterCounter(0);
            return 1;
        }
    }
    return 0;
}

void CVICII::read(uint16_t address, uint8_t* bt) {
    if (check_unused_address(CPU_MEM_READ, address, bt)) {
        return;
    }

    // check shadow
    uint16_t addr = check_shadow_address(address);

    // finally read
    _cpu->memory()->readByte(addr,bt);
}

void CVICII::write(uint16_t address, uint8_t bt) {
    // on write these are unused
    if (check_unused_address(CPU_MEM_WRITE, address, nullptr)) {
        return;
    }

    if (address == VIC_REG_RASTERCOUNTER) {
        // raster counter
        _rasterCounter = bt;
    }
    else if (address == VIC_REG_CR1) {
        // setting cr1 also affects the raster counter
        if (IS_BIT_SET(bt, 7)) {
            // bit 7 is the 9th bit of the raster counter
            _rasterCounter |= (bt >> 7);
        }
    }
    else if (address == VIC_REG_BASE_ADDR) {

    }

    // check shadow
    uint16_t addr = check_shadow_address(address);

    // finally write
    _cpu->memory()->writeByte(addr,bt);
}

/**
 * some addresses are shadowed and maps to other addresses
 * @param address the input address
 * @return the effective address
 */
uint16_t CVICII::check_shadow_address(uint16_t address) {
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
bool CVICII::check_unused_address(int type, uint16_t address, uint8_t *bt) {
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

void CVICII::updateScreen(uint32_t* frameBuffer) {
    // check cr1 and cr2
    uint8_t cr1;
    uint8_t cr2;
    _cpu->memory()->readByte(VIC_REG_CR1, &cr1);
    _cpu->memory()->readByte(VIC_REG_CR2, &cr2);
    if (!IS_BIT_SET(cr1, 6) && !IS_BIT_SET(cr1, 5)) {
        if (!IS_BIT_SET(cr2, 5)) {
            // https://www.c64-wiki.com/wiki/Standard_Character_Mode
            updateScreenLoRes(frameBuffer);
        }
    }
    else {
        CLog::fatal("screen mode not yet implemented!");
    }
}

/**
 * update screen in low resolution mode (40x25)
 * https://www.c64-wiki.com/wiki/Standard_Character_Mode
 * @param frameBuffer the framebuffer memory to be updated
 */
void CVICII::updateScreenLoRes(uint32_t *frameBuffer) {
    // get the addresses with the help of the cia-2
    uint16_t charsetAddress;
    uint16_t screenAddress;
    uint8_t* colorMem = _cpu->memory()->raw() + MEMORY_COLOR_ADDRESS;
    getTextModeScreenAddress(&screenAddress,&charsetAddress);

    uint8_t* charset = _cpu->memory()->raw() + charsetAddress;
    if (charsetAddress == VIC_REGISTERS_START) {
        // bank3 is used, use the ROM charset
        charset = ((CMemory*)_cpu->memory())->charset();
    }

    int currentColumn = 0;
    int currentRow = 0;
    int borderHSize = (VIC_PAL_SCREEN_W - 320) / 2;
    int borderVSize = (VIC_PAL_SCREEN_H - 200) / 2;

    // get border color
    uint8_t borderColor;
    _cpu->memory()->readByte(VIC_REG_BORDER_COLOR, &borderColor);
    rgbStruct borderRgb = _palette[borderColor];

    // draw the border
    for (int borderX = 0; borderX < VIC_PAL_SCREEN_W; borderX++) {
        for (int borderY = 0; borderY < VIC_PAL_SCREEN_H; borderY++) {
            if (borderX <= borderHSize || borderX >= (VIC_PAL_SCREEN_W - borderHSize) ||
                borderY <= borderVSize || borderY >= (VIC_PAL_SCREEN_H - borderVSize) ) {
                    // draw border
                    int posB = ((borderY*VIC_PAL_SCREEN_W) + borderX);
                    frameBuffer[posB]=SDL_MapRGB(_sdlCtx->pxFormat, borderRgb.r, borderRgb.g, borderRgb.b);
            }
        }
    }

    // 40x25=1000 screen codes total, draw the screen character per character
    for (int i=0; i < 1000; i++) {
        // read foreground color
        uint8_t fgColor = (colorMem[(currentRow * 40) + currentColumn]) & 0xf;
        rgbStruct fRgb = _palette[fgColor];

        // read background color
        uint8_t bgColor;
        _cpu->memory()->readByte(VIC_REG_BG_COLOR_0, &bgColor);
        bgColor &= 0xf;
        rgbStruct bRgb = _palette[bgColor];

        // read screencode in video memory -> each screencode corresponds to a character in the charset memory
        uint8_t screenCode;
        _cpu->memory()->readByte(screenAddress + i, &screenCode);

        // each character in the charset is 8 bytes (8x8 font)
        for (int j=0; j < 8; j ++) {
            // read row
            uint8_t row = charset[(screenCode*8) + j];
            for (int k = 0; k < 8; k ++) {
                // and blit it pixel per pixel in the framebuffer
                int pixX=(currentColumn * 8) + k + borderHSize;
                int pixY=(currentRow * 8) + j+ borderVSize;
                int pos = ((pixY*VIC_PAL_SCREEN_W) + pixX);

                // get the color code (0-15)
                if (IS_BIT_SET(row, 7)) {
                    // bit is set, set foreground color
                    frameBuffer[pos]=SDL_MapRGB(_sdlCtx->pxFormat, fRgb.r, fRgb.g, fRgb.b);
                }
                else {
                    frameBuffer[pos]=SDL_MapRGB(_sdlCtx->pxFormat, bRgb.r, bRgb.g, bRgb.b);
                }

                // next row
                row <<= 1;
            }

            // screen is 40x25
            if (currentColumn == 40) {
                // next screen row
                currentRow++;
                currentColumn = 0;
            }
        }

        // next screen column
        currentColumn++;
    }
}

CVICII::CVICII(CMOS65xx *cpu, CCIA2 *cia2) {
    _cpu = cpu;
    _cia2 = cia2;
    _rasterCounter = 0;

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
 */
void CVICII::setSdlCtx(SDLDisplayCtx *ctx) {
    _sdlCtx = ctx;
}


