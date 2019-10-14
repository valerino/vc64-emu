//
// Created by valerino on 10/01/19.
//

#include "CMemory.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <CBuffer.h>
#include <string.h>
#include "bitutils.h"

CMemory::CMemory(CPLA *pla) {
    // allocate main memory and the shadows
    _mem = (uint8_t *)calloc(1, MEMORY_SIZE);
    _kernalRom = (uint8_t *)calloc(1, MEMORY_KERNAL_SIZE);
    _basicRom = (uint8_t *)calloc(1, MEMORY_BASIC_SIZE);
    _charRom = (uint8_t *)calloc(1, MEMORY_CHARSET_SIZE);

    _pla = pla;
}

CMemory::~CMemory(){SAFE_FREE(_mem) SAFE_FREE(_charRom) SAFE_FREE(_basicRom)
                        SAFE_FREE(_kernalRom)}

uint8_t CMemory::readByte(uint32_t address, uint8_t *b, bool raw) {
    if (!b) {
        return EINVAL;
    }
    if (raw) {
        // force raw read
        *b = _mem[address];
        return 0;
    }

    /*
    // get mapping with PLA
    int mapType = _pla->mapAddressToType(address);

    // and map different addresses
    switch (mapType) {
    case PLA_MAP_BASIC_ROM:
        // map basic rom
        *b = _basicRom[address - MEMORY_BASIC_ADDRESS];
        break;
    case PLA_MAP_CHARSET_ROM:
        // map character rom
        *b = _charRom[address - MEMORY_CHARSET_ADDRESS];
        break;
    case PLA_MAP_KERNAL_ROM:
        // map kernal rom
        *b = _kernalRom[address - MEMORY_KERNAL_ADDRESS];
        break;

    default:
        // map ram
        *b = _mem[address];
    }
    */
    // check addresses
    if (address >= MEMORY_BASIC_ADDRESS &&
        address < MEMORY_BASIC_ADDRESS + MEMORY_BASIC_SIZE) {
        // $a000 (basic rom)
        if (IS_BIT_SET(_mem[1], 0)) {
            *b = _basicRom[address - MEMORY_BASIC_ADDRESS];
        } else {
            // basic rom is masked, returning ram
            *b = _mem[address];
        }
    } else if (address >= MEMORY_KERNAL_ADDRESS &&
               address < MEMORY_KERNAL_ADDRESS + MEMORY_KERNAL_SIZE) {
        // $e000 (kernal rom)
        if (IS_BIT_SET(_mem[1], 1)) {
            *b = _kernalRom[address - MEMORY_KERNAL_ADDRESS];
        } else {
            // kernal rom is masked, returning ram
            *b = _mem[address];
        }
    } else if (address >= MEMORY_CHARSET_ADDRESS &&
               address < MEMORY_CHARSET_ADDRESS + MEMORY_CHARSET_SIZE) {
        // $d000 (charset rom)
        if (IS_BIT_SET(_mem[1], 2)) {
            // access the IO registers
            *b = _mem[address];
        } else {
            *b = _charRom[address - MEMORY_CHARSET_ADDRESS];
        }
    } else {
        // ram
        *b = _mem[address];
    }

    return 0;
}

int CMemory::writeByte(uint32_t address, uint8_t b, bool raw) {
    if (raw) {
        // force raw write
        _mem[address] = b;
        return 0;
    }

    // check zeropage addresses
    switch (address) {
    case 0:
        setPageZero00(b);
        // SDL_Log("writing %x to $0", b);
        break;
    case 1:
        setPageZero01(b);
        // SDL_Log("writing %x to $1", b);
        break;
    default:
        // write to ram
        _mem[address] = b;
    }
    return 0;
}

uint16_t CMemory::readWord(uint32_t address, uint16_t *w) {
    if (w) {
        uint8_t lo;
        readByte(address, &lo);
        uint8_t hi;
        readByte(address + 1, &hi);

        uint16_t ww = (hi << 8) | lo;
        *w = ww;
        return 0;
    }
    return EINVAL;
}

int CMemory::writeWord(uint32_t address, uint16_t w) {
    uint8_t lo = (w & 0xff);
    uint8_t hi = (uint8_t)(w >> 8);
    writeByte(address, lo);
    writeByte(address + 1, hi);
    return 0;
}

int CMemory::readBytes(uint32_t address, uint8_t *b, uint32_t bufferSize,
                       uint32_t readSize) {
    int res = 0;
    uint32_t s = readSize;

    if (readSize > bufferSize) {
        s = bufferSize;
        res = EOVERFLOW;
    }
    for (uint16_t i = 0; i < s; i++) {
        uint8_t c;
        readByte(address + i, &c);
        b[i] = c;
    }
    return res;
}

int CMemory::writeBytes(uint32_t address, uint8_t *b, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        writeByte(address + i, b[i]);
    }
    return 0;
}

/**
 * @brief load bios files
 * @return 0 on success, or errno
 */
int CMemory::loadBios() {
    char path[260];
    char bios[] = "./bios";
    bool ok = false;
    int res = 0;
    do {
        // load kernal
        snprintf(path, sizeof(path), "%s/kernal.bin", bios);
        uint32_t kernalSize;
        res = CBuffer::fromFile(path, &_kernalRom, &kernalSize);
        if (res != 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", strerror(res),
                         path);
            break;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "loaded kernal ROM: %s, size=0x%0x", path, kernalSize);

        // load charset
        uint32_t charsetSize;
        snprintf(path, sizeof(path), "%s/charset.bin", bios);
        res = CBuffer::fromFile(path, &_charRom, &charsetSize);
        if (res != 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", strerror(res),
                         path);
            break;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "loaded charset ROM: %s, size=0x%0x", path, charsetSize);

        // load basic
        uint32_t basicSize;
        snprintf(path, sizeof(path), "%s/basic.bin", bios);
        res = CBuffer::fromFile(path, &_basicRom, &basicSize);
        if (res != 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", strerror(res),
                         path);
            break;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "loaded basic ROM: %s, size=0x%0x", path, basicSize);
        ok = true;
    } while (0);
    if (!ok) {
        SAFE_FREE(_charRom)
        SAFE_FREE(_kernalRom)
        SAFE_FREE(_basicRom)
        return res;
    }

    return 0;
}

/**
 * @brief get on-chip port $00 (data direction)
 * @return
 */
uint8_t CMemory::pageZero00() { return _mem[0]; }

/**
 * @brief get on-chip port $01 (controls mapping)
 * @return
 */
uint8_t CMemory::pageZero01() { return _mem[1]; }

/**
 * @brief set on-chip port $01 (data direction)
 * @return
 */
void CMemory::setPageZero00(uint8_t bt) { _mem[0] = bt; }

/**
 * @brief set on-chip port $00 (controls mapping)
 * @return
 */
void CMemory::setPageZero01(uint8_t bt) {
    _mem[1] = bt;

    // setup memory mapping
    _pla->setupMemoryMapping(bt);
}

int CMemory::init() {
    memset(_mem, 0, MEMORY_SIZE);

    // setup zeropage
    // http: // unusedino.de/ec64/technical/aay/c64/zpmain.htm

    /*
    $00/0:   6510 On-chip Data Direction Register

   +----------+---------------------------------------------------+
   | Bit  7   |   Undefined                                       |
   | Bit  6   |   Only available on C128, otherwise undefined     |
   | Bits 0-5 |   1 = Output, 0 = Input (see $01 for description) |
   +----------+---------------------------------------------------+
   Default Value is $2F/47 (%00101111).
    */
    setPageZero00(0x2f);

    /*
     $01/1:   6510 On-chip 8-bit Input/Output Register
   +----------+---------------------------------------------------+
   | Bits 7-6 |   Undefined                                       |
   | Bit  5   |   Cassette Motor Control (0 = On, 1 = Off)        |
   | Bit  4   |   Cassette Switch Sense: 1 = Switch Closed        |
   | Bit  3   |   Cassette Data Output Line                       |
   | Bit  2   |   /CharEn-Signal (see Memory Configuration)       |
   | Bit  1   |   /HiRam-Signal  (see Memory Configuration)       |
   | Bit  0   |   /LoRam-Signal  (see Memory Configuration)       |
   +----------+---------------------------------------------------+
   Default Value is $37/55 (%00110111).
    */
    setPageZero01(0x37);

    // load bios
    return loadBios();
}

uint8_t *CMemory::raw(uint32_t *size) {
    if (size) {
        *size = MEMORY_SIZE;
    }
    return _mem;
}

uint8_t *CMemory::charset() { return _charRom; }

int CMemory::loadPrg(const char *path) {
    if (!path) {
        return EINVAL;
    }

    // load file
    uint8_t *buf;
    uint32_t size;
    int res = CBuffer::fromFile(path, &buf, &size);
    if (res != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "failed to load PRG: %s (%d)", path, res);
        return res;
    }

    // first 2 bytes is the load address
    uint8_t *p = (uint8_t *)buf;
    uint16_t address = *((uint16_t *)p);
    size -= sizeof(uint16_t);
    p += sizeof(uint16_t);
    uint16_t end = address + size;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "loading %s in RAM, start=$%08x, end=$%08x, size=%d", path,
                address, end, size);

    // copy in ram
    res = writeBytes(address, p, size);
    free(buf);
    if (res != 0) {
        return res;
    }

    if (address == BASIC_PRG_START_ADDRESS) {
        // set basic informations in the zeropage, so to be able to start the
        // loaded program
        uint16_t end = address + size;
        writeWord(ZEROPAGE_BASIC_PROGRAM_START, address);
        writeWord(ZEROPAGE_BASIC_VARTAB, end);
        writeWord(ZEROPAGE_BASIC_ARYTAB, end);
        writeWord(ZEROPAGE_BASIC_STREND, end);
    }
    return res;
}
