//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_CMEMORY_H
#define VC64_EMU_CMEMORY_H

#include <cstdint>
#include <IMemory.h>

// the whole 64k size
#define MEMORY_SIZE 0x10000

// memory map
#define MEMORY_BASIC_ADDRESS    0xa000
#define MEMORY_BASIC_SIZE       0x2000
#define MEMORY_CHARSET_ADDRESS  0xd000
#define MEMORY_CHARSET_SIZE     0x1000
#define MEMORY_KERNAL_ADDRESS   0xe000
#define MEMORY_KERNAL_SIZE      0x2000
#define MEMORY_VIDEO_ADDRESS    0x400
#define MEMORY_VIDEO_SIZE       0x400
#define MEMORY_COLOR_ADDRESS    0xd800
#define MEMORY_COLOR_SIZE       0x400

// zeropage
#define ZEROPAGE_REG_DATA_DIRECTION 0
#define ZEROPAGE_REG_IO_PORT 1


/**
 * implements the emulated memory
 * https://www.c64-wiki.com/wiki/Memory_Map
 */
class CMemory: public IMemory {
private:
    // global/writable memory
    uint8_t* _mem;

    // for commodity, we keep the ROMS aliased there in separate buffers (which shadows the respective address in the global memory)
    uint8_t* _charRom;
    uint8_t* _kernalRom;
    uint8_t* _basicRom;

    /**
     * load bios files
     * @return 0 on success, or errno
     */
    int loadBios();

public:
    uint8_t readByte(uint32_t address, uint8_t *b) override;

    uint16_t readWord(uint32_t address, uint16_t *w) override;

    int writeByte(uint32_t address, uint8_t b) override;

    int writeWord(uint32_t address, uint16_t w) override;

    int writeBytes(uint32_t address, uint8_t *b, uint32_t size) override;

    int readBytes(uint32_t address, uint8_t *b, uint32_t bufferSize, uint32_t readSize) override;

    int init() override;

    /**
     * get pointer to ram memory
     * @return
     */
    uint8_t* ram();

    uint8_t* raw(uint32_t *size = nullptr) override;

    /**
     * return the charset rom
     * @return the memory pointer
     */
    uint8_t* charset();

    CMemory();
    ~CMemory();
};


#endif //VC64_EMU_CMEMORY_H
