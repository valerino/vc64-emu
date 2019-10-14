//
// Created by valerino on 10/01/19.
//

#pragma once
#include <cstdint>
#include <IMemory.h>

// the whole 64k size
#define MEMORY_SIZE 0x10000

// memory map
#define MEMORY_BASIC_ADDRESS 0xa000
#define MEMORY_BASIC_SIZE 0x2000
#define MEMORY_CHARSET_ADDRESS 0xd000
#define MEMORY_CHARSET_SIZE 0x1000
#define MEMORY_KERNAL_ADDRESS 0xe000
#define MEMORY_KERNAL_SIZE 0x2000

#define MEMORY_SCREEN_ADDRESS 0x400
#define MEMORY_COLOR_ADDRESS 0xd800
#define MEMORY_BITMAP_ADDRESS 0x0

// start address of a basic program
#define BASIC_PRG_START_ADDRESS 0x801

// zeropage
#define ZEROPAGE_REG_DATA_DIRECTION 0
#define ZEROPAGE_REG_IO_PORT 1
#define ZEROPAGE_BASIC_PROGRAM_START                                           \
    0x2b // 0x2b-0x2c, this contains the address of the basic program, usually
         // BASIC_PRG_START_ADDRESS
#define ZEROPAGE_BASIC_VARTAB                                                  \
    0x2d // 0x2d-0x2e, address of the normal variables, right after the basic
         // program end
#define ZEROPAGE_BASIC_ARYTAB                                                  \
    0x2f // 0x2f-0x30, address of array variables, right after the basic program
         // end
#define ZEROPAGE_BASIC_STREND                                                  \
    0x31 // 0x31-0x32, end address +1 of array variables, right after the basic
         // program end

/**
 * @brief implements the emulated memory
 * @see https://www.c64-wiki.com/wiki/Memory_Map
 */
class CMemory : public IMemory {
  private:
    // global/writable memory
    uint8_t *_mem = nullptr;

    // for commodity, we keep the ROMS aliased there in separate buffers (which
    // shadows the respective address in the global memory)
    uint8_t *_charRom = nullptr;
    uint8_t *_kernalRom = nullptr;
    uint8_t *_basicRom = nullptr;

    int loadBios();

  public:
    uint8_t readByte(uint32_t address, uint8_t *b) override;

    /**
     * @brief read memory raw (disregarding mapping)
     * @param address the address in the emulated memory
     * @param bt on successful return, the byte read
     * @return 0 on success, or errno
     */
    int readRawByte(uint16_t addr, uint8_t *bt);

    uint16_t readWord(uint32_t address, uint16_t *w) override;

    int writeByte(uint32_t address, uint8_t b) override;

    int writeWord(uint32_t address, uint16_t w) override;

    int writeBytes(uint32_t address, uint8_t *b, uint32_t size) override;

    int readBytes(uint32_t address, uint8_t *b, uint32_t bufferSize,
                  uint32_t readSize) override;

    int init() override;

    /**
     * load a .PRG in memory
     * @param path path to the prg
     * @return 0 on success
     */
    int loadPrg(const char *path);

    /**
     * get pointer to ram memory
     * @return
     */
    uint8_t *ram();

    uint8_t *raw(uint32_t *size = nullptr) override;

    /**
     * return the charset rom
     * @return the memory pointer
     */
    uint8_t *charset();
    CMemory();
    ~CMemory();
};
