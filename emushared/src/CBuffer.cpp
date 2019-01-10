//
// Created by valerino on 10/01/19.
//

#include <CBuffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int CBuffer::fromFile(const char *path, uint8_t **buffer) {
    if (!buffer) {
        return EINVAL;
    }
    *buffer = nullptr;
    FILE* f = fopen(path, "rb");
    if (!f) {
        return errno;
    }

    // read whole file
    fseek(f,0,SEEK_END);
    size_t sz = ftell(f);
    rewind(f);
    if (sz == 0) {
        fclose(f);
        return ENODATA;
    }

    // to allocated memory
    uint8_t* b = (uint8_t*)calloc(1,sz);
    if (!b) {
        fclose(f);
        return ENOMEM;
    }
    fread(b,sz,1,f);
    fclose(f);
    return 0;
}
