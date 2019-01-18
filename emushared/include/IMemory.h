//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_IMEMORY_H
#define VC64_EMU_IMEMORY_H

#include <stdint.h>
#include <errno.h>

/**
 * defines the interface to access emulated memory
 */
class IMemory {
public:
    /**
     * read byte at address
     * @param address the address in the emulated memory
     * @param b on successful return, the byte read
     * @return 0 on success, or errno
     */
    virtual uint8_t readByte(uint32_t address, uint8_t* b) = 0;
    /**
     * read word (2 bytes) at address
     * @param address the address in the emulated memory
     * @param w on successful return, the word read
     * @return 0 on success, or errno
     */
    virtual uint16_t readWord(uint32_t address, uint16_t* w) = 0;

    /**
     * write byte at address
     * @param address the address in the emulated memory
     * @param b byte to be written
     * @return 0 on success, or errno
     */
    virtual int writeByte(uint32_t address, uint8_t b) = 0;

    /**
     * write word (2 bytes) at address
     * @param address the address in the emulated memory
     * @param w the word to be written (big/little endian is handled internally)
     * @return 0 on success, or errno
     */
    virtual int writeWord(uint32_t address, uint16_t w) = 0;

    /**
     * write buffer at address
     * @param address the address in the emulated memory
     * @param b the buffer to be written
     * @param size size of buffer to be written
     * @return 0 on success, or errno
     */
    virtual int writeBytes(uint32_t address, uint8_t* b, uint32_t size) = 0;

    /**
     * read buffer at address
     * @param address the address in the emulated memory
     * @param b the buffer to read into
     * @param bufferSize size of buffer
     * @param readSize size of read
     * @return 0 on success, or errno
     */
    virtual int readBytes(uint32_t address, uint8_t* b, uint32_t bufferSize, uint32_t readSize) = 0;

    /**
     * initializes emulated memory
     * @return 0 on success, or errno
     */
    virtual int init() = 0;
};

#endif //VC64_EMU_IMEMORY_H
