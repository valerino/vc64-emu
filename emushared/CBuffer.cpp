//
// Created by valerino on 10/01/19.
//

#include <CBuffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int CBuffer::fromFile(const char *path, uint8_t **buffer, uint32_t* size) {
    if (!buffer || !size) {
        return EINVAL;
    }
    *size = 0;
    *buffer = nullptr;
    FILE* f = fopen(path, "rb");
    if (!f) {
        return errno;
    }

    // read whole file
    fseek(f,0,SEEK_END);
    long sz = ftell(f);
    rewind(f);
    if (sz == 0) {
        fclose(f);
        return ENODATA;
    }
    // to allocated memory
    uint8_t* b = (uint8_t*)calloc(1, sz);
    if (!b) {
        fclose(f);
        return ENOMEM;
    }
    fread(b, sz, 1, f);
    *size = (uint32_t)sz;
    *buffer = b;
    fclose(f);
    return 0;
}

void CBuffer::hexDump(void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf("  %s\n", buff);

            // Output the offset.
            printf("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
            buff[i % 16] = '.';
        } else {
            buff[i % 16] = pc[i];
        }

        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf("  %s\n", buff);
}