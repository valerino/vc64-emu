//
// Created by valerino on 06/01/19.
//

#include <stdio.h>
#include <CMOS65xx.h>
#include <SDL.h>
#include <getopt.h>
#include "CDisplay.h"
#include "CMemory.h"
#include <CLog.h>

/**
 * shows banner
 */
void banner() {
    printf("vc64 - a c64 emulator\n");
    printf("(c)opyleft, valerino, y2k19\n\n");
}

/**
 * prints usage
 * @param argv
 */
void usage(char** argv) {
    printf("usage: %s -f [file to play]\n", argv[0]);
}

int main (int argc, char** argv) {
    // prints title
    banner();

    // TODO: getopt commandline
    char* path = nullptr;
    int option = getopt(argc, argv, "f:");
    switch (option) {
        case '?':
            usage(argv);
            return 1;
        case 'f':
            path = optarg;
            break;
        default:
            break;
    }

    // initialize sdl
    int res = SDL_Init(SDL_INIT_EVERYTHING);
    if (res != 0) {
        CLog::error("%s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    CLog::print("SDL initialized OK!");

    // TODO: check bios

    // create display
    CDisplay* display = CDisplay::instance();
    bool initialized = display->init();
    if (!initialized) {
        CLog::error("%s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    CLog::print("Display initialized OK!");

    // create memory
    CMemory* mem = new CMemory();
    CLog::print("Memory initialized OK!");

    // create cpu
    CMOS65xx* cpu = new CMOS65xx(mem);
    CLog::print("CPU initialized OK!");

    // TODO: start the emulator
    if (path != nullptr) {
        CLog::print("loading file: %s", path);
    }

    // done
    delete mem;
    delete cpu;
    SDL_Quit();
    return res;
}