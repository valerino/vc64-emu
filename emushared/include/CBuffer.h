//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_CBUFFER_H
#define VC64_EMU_CBUFFER_H

#include <stdint.h>

/**
 * utilities to manipulate buffers/memory
 */
class CBuffer {
public:
    /**
     * load buffer from file
     * @param path path to the buffer to be loaded
     * @param buffer on successful return, the buffer to be freed with free() once used
     * @param buffer on successful return, size of the allocated buffer
     * @return 0 on success, or errno
     */
    static int fromFile(const char *path, uint8_t **buffer, uint32_t *size);

    /**
     * hexdump buffer to stdout
     * @param addr address to dump
     * @param len size to dump
     */
    static void hexDump(void *addr, int len);
};

/**
 * safe delete allocated memory
 */
#define SAFE_DELETE(_x_) if(_x_ != nullptr) { delete _x_; _x_=nullptr; }
#define SAFE_FREE(_x_) if(_x_ != nullptr) { free(_x_); _x_=nullptr; }

#endif //VC64_EMU_CBUFFER_H
