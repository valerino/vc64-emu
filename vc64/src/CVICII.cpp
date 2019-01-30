//
// Created by valerino on 19/01/19.
//

#include "CVICII.h"
#include "CMemory.h"

#include <CLog.h>
#include "globals.h"
CVICII::~CVICII() {

}

CVICII::CVICII(CMOS65xx *cpu) {
    _cpu = cpu;
    _rasterCounter = 0;
}

CMOS65xx *CVICII::cpu() {
    return _cpu;
}

void CVICII::setRasterCounter(uint16_t cnt) {
    _rasterCounter = cnt;
    if (cnt <= 0xff) {
        // 8 bit
        _cpu->memory()->writeByte(0xd012, cnt & 0xff);
    }
    else {
        // 9th bit of the raster counter is set into bit 7 of cr1
        uint8_t cr1;
        _cpu->memory()->readByte(0xd011, &cr1);
        cr1 |= (cnt >> 8) << 7;

        _cpu->memory()->writeByte(0xd012, cnt & 0xff);
        _cpu->memory()->writeByte(0xd011, cr1);
    }
}

int CVICII::run(int cycleCount) {
    uint8_t* mem = _cpu->memory()->ram(nullptr);
    uint8_t d019 = mem[0xd019];
    uint8_t d01a = mem[0xd01a];
//    _cpu->memory()->readByte(0xd019, &d019);
//    _cpu->memory()->readByte(0xd01a, &d01a);
    //if (d019 & 80) {
    //    // if bit 7 is set, trigger an irq
    //    _cpu->irq();
    //}
    //if (d01a >= 1 && d01a <= 8) {
        // irq triggered if bit 0/1/2/3 set
        //_cpu->irq();
    //}
    if ((cycleCount % CYCLES_PER_LINE) == 0) {
        // increment the raster counter
        setRasterCounter(_rasterCounter + 1);
        if (_rasterCounter > VIC_PAL_LINES) {
            // reset raster counter
            setRasterCounter(0);
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
    uint16_t videoMemAddress = MEMORY_VIDEO_ADDRESS;
    uint16_t charMemAddress = MEMORY_CHARSET_ADDRESS;
    int videoMemSize = 0x400;
    int currX = 0;
    int currY = 0;
    for (int i=0; i < videoMemSize; i++) {
        // read screencode in video memory -> each screencode corresponds to a character in the charset memory
        uint8_t screenCode;
        _cpu->memory()->readByte(videoMemAddress + i, &screenCode);
        // each character in character memory is made of 8 bytes (8x8 font)
        for (int j=0; j < 8; j ++) {
            // read row
            uint8_t row;
            _cpu->memory()->readByte(charMemAddress + (screenCode*8) + j, &row);
            for (int k = 0; k < 8; k ++) {
                // and blit it bit by bit (pixel x pixel) in the framebuffer
                int pixX=(currX * 8) + k;
                int pixY=(currY * 8) + j;
                int pos = ((pixY*VIC_PAL_SCREEN_W) + pixX);
                if (row & 0x80) {
                    // bit is set
                    frameBuffer[pos] = 0;
                    frameBuffer[pos+1] = 0;
                    frameBuffer[pos+2] = 0;
                    frameBuffer[pos+3] = 0xff;
                }
                else {
                    frameBuffer[pos] = 0xff;
                    frameBuffer[pos+1] = 0xff;
                    frameBuffer[pos+2] = 0xff;
                    frameBuffer[pos+3] = 0xff;
                }

                // next row
                row <<= 1;
            }

            // screen is 40x25
            if (currX == 40) {
                // next screen row
                currY++;
                currX = 0;
            }
        }

        // next screen column
        currX++;
    }
}


