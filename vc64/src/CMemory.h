//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_CMEMORY_H
#define VC64_EMU_CMEMORY_H

#include <cstdint>
#include <IMemory.h>

// the emulated memory size
#define MEMORY_SIZE 0xffff

/**
 * implements the emulated memory
 */
class CMemory: public IMemory {
private:
    uint8_t* _mem;

    uint8_t readByte(uint16_t address, uint8_t *b) override;

    uint16_t readWord(uint16_t address, uint16_t *w) override;

    int writeByte(uint16_t address, uint8_t b) override;

    int writeWord(uint16_t address, uint16_t w) override;

    int writeBytes(uint16_t address, uint8_t *b, uint16_t size) override;

    int readBytes(uint16_t address, uint8_t *b, uint16_t bufferSize, uint16_t readSize) override;

public:
    CMemory();
    ~CMemory();
};


#endif //VC64_EMU_CMEMORY_H
