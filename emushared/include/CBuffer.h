//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_CBUFFER_H
#define VC64_EMU_CBUFFER_H

#include <stdint.h>

/**
 * utilities to manipulate binary buffers
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
    static int fromFile(const char* path, uint8_t** buffer, uint32_t* size);
};


#endif //VC64_EMU_CBUFFER_H
