//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_CMEMORY_H
#define VC64_EMU_CMEMORY_H

#include <cstdint>
#include <IMemory.h>

// the emulated memory size
#define MEMORY_SIZE 0xffff

// memory map (https://www.c64-wiki.com/wiki/Memory_Map)
#define MEMORY_CART_LO_ADDRESS  0x8000
#define MEMORY_CART_LO_SIZE     0x2000
#define MEMORY_BASIC_ADDRESS    0xa000
#define MEMORY_BASIC_SIZE       0x2000
#define MEMORY_CHARSET_ADDRESS  0xd000
#define MEMORY_CHARSET_SIZE     0x1000
#define MEMORY_KERNAL_ADDRESS   0xe000
#define MEMORY_KERNAL_SIZE      0x2000
#define MEMORY_CART_HI_ADDRESS  MEMORY_KERNAL_ADDRESS
#define MEMORY_CART_HI_SIZE     MEMORY_KERNAL_SIZE

/**
 * implements the emulated memory
 */
class CMemory: public IMemory {
private:
    uint8_t* _mem;

    /**
     * load bios files
     * @return 0 on success, or errno
     */
    int loadBios();

public:
    uint8_t readByte(uint16_t address, uint8_t *b) override;

    uint16_t readWord(uint16_t address, uint16_t *w) override;

    int writeByte(uint16_t address, uint8_t b) override;

    int writeWord(uint16_t address, uint16_t w) override;

    int writeBytes(uint16_t address, uint8_t *b, uint16_t size) override;

    int readBytes(uint16_t address, uint8_t *b, uint16_t bufferSize, uint16_t readSize) override;

    int init() override;

    CMemory();
    ~CMemory();
};


#endif //VC64_EMU_CMEMORY_H
