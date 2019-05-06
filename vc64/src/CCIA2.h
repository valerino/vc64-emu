//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CCIA2_H
#define VC64_EMU_CCIA2_H

#include <CMOS65xx.h>
#include "CIACommon.h"

/**
 * registers
 * https://www.c64-wiki.com/wiki/CIA
 */
#define CIA2_REGISTERS_START 0xdd00
#define CIA2_REGISTERS_END   0xddff

/**
 * implements the 2nd CIA 6526, which controls the serial bus, rs232, vic memory and NMI
 */
class CCIA2 {
public:
    CCIA2(CMOS65xx* cpu);
    ~CCIA2();

    /**
     * update the internal state
     * @param current cycle count
     * @return additional cycles used
     */
    int update(int cycleCount);

    /**
     * get the bank connected to vic-ii by looking at the data register
     * https://www.c64-wiki.com/wiki/VIC_bank
     * @param address on return, the vic base address
     * @return bank 0-3
     */
    int getVicBank(uint16_t* address);

    /**
     * read from chip memory
     * @param address
     * @param bt
     */
    void read(uint16_t address, uint8_t* bt);

    /**
     * write to chip memory
     * @param address
     * @param bt
     */
    void write(uint16_t address, uint8_t bt);

private:
    CMOS65xx* _cpu;
    uint16_t _timerALatch = 0;
    uint16_t _timerBLatch = 0;
    uint16_t _timerA = 0;
    uint16_t _timerB = 0;
    int _timerAMode = 0;
    int _timerBMode = 0;
    bool _timerARunning = false;
    bool _timerBRunning = false;
    bool _timerANmiEnabled = false;
    bool _timerBNmiEnabled = false;
    int _prevCycleCount = 0;
};

#endif //VC64_EMU_CCIA2_H
