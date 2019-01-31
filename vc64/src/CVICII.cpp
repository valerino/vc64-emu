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

CVICII::CVICII(CMOS65xx *cpu) {
    _cpu = cpu;
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

CMOS65xx *CVICII::cpu() {
    return _cpu;
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
    /*uint8_t* mem = _cpu->memory()->ram(nullptr);
    uint8_t d019 = mem[0xd019];
    uint8_t d01a = mem[0xd01a];*/
    //_cpu->memory()->readByte(0xd019, &d019);
//    _cpu->memory()->readByte(0xd01a, &d01a);
    //if (mem[1] & 4 && d019 & 80) {
        // if bit 7 is set, trigger an irq
     //   _cpu->irq();
   // }
    //if (d01a >= 1 && d01a <= 8) {
        // irq triggered if bit 0/1/2/3 set
        //_cpu->irq();
    //}
    if ((cycleCount % CYCLES_PER_LINE) == 0) {
        // increment the raster counter
        updateRasterCounter(_rasterCounter + 1);
        if (_rasterCounter > VIC_PAL_LINES) {
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

    bool hit;
    uint16_t addr = check_shadow_address(address, &hit);
    if (hit) {
        // read again
        _cpu->memory()->readByte(addr,bt);
    }
}

void CVICII::write(uint16_t address, uint8_t bt) {
    // on write these are unused
    if (check_unused_address(CPU_MEM_WRITE, address, nullptr)) {
        return;
    }

    bool hit;
    uint16_t addr = check_shadow_address(address,&hit);
    if (hit) {
        // write at another address
        _cpu->memory()->writeByte(addr,bt);
    }
}

/**
 * some addresses are shadowed and maps to other addresses
 * @param address the input address
 * @param hit true if read/write hits a shadowed address (so the memory must be read again)
 * @return the effective address
 */
uint16_t CVICII::check_shadow_address(uint16_t address, bool* hit) {
    // check for shadow addresses
    if (address >= 0xd040 && address <= 0xd3ff) {
        // these are shadows for $d000-$d03f
        *hit = true;
        return (VIC_REGISTERS_START + ((address % VIC_REGISTERS_START) % 0x40));
    }
    *hit = false;
    return address;
}

/**
 * read/write to these addresses has no effect or returns a fixed values
 * @param type
 * @param address
 * @param bt
 * @return true if it's an unused address
 */
bool CVICII::check_unused_address(int type, uint16_t address, uint8_t *bt) {
    if (address >= 0xd02f && address <= 0xd03f) {
        if (type == CPU_MEM_READ) {
            // these locations always return 0xff on read
            *bt = 0xff;
        } else {
            // unused
        }
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
    uint8_t* charset = ((CMemory*)_cpu->memory())->charset();
    uint8_t* colorMem = _cpu->memory()->raw() + MEMORY_COLOR_ADDRESS;
    int currentColumn = 0;
    int currentRow = 0;

    // 40x25=1000 screen codes total
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
        _cpu->memory()->readByte(MEMORY_VIDEO_ADDRESS + i, &screenCode);
        // each character in character memory is made of 8 bytes (8x8 font)
        for (int j=0; j < 8; j ++) {
            // read row
            uint8_t row = charset[(screenCode*8) + j];
            for (int k = 0; k < 8; k ++) {
                // and blit it bit by bit (pixel x pixel) in the framebuffer
                int pixX=(currentColumn * 8) + k;
                int pixY=(currentRow * 8) + j;
                int pos = ((pixY*VIC_PAL_SCREEN_W) + pixX);

                // get the color code (0-15)
                //int color = (A & 0xff) << 24 | (B & 0xff) << 16 | (G & 0xff) << 8 | (R & 0xff);
                if (IS_BIT_SET(row, 7)) {
                    // bit is set, set foreground color
                    frameBuffer[pos] = fRgb.b;
                    frameBuffer[pos+1] = fRgb.g;
                    frameBuffer[pos+2] = fRgb.r;
                    frameBuffer[pos+3] = 0xff;
                }
                else {
                    frameBuffer[pos] = bRgb.b;
                    frameBuffer[pos+1] = bRgb.g;
                    frameBuffer[pos+2] = bRgb.r;
                    frameBuffer[pos+3] = 0xff;
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


