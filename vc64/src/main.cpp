//
// Created by valerino on 06/01/19.
//

#include <stdio.h>
#include <CBuffer.h>
#include <CSDLUtils.h>
#include <CLog.h>
#include <CMOS65xx.h>
#include <getopt.h>
#include "CDisplay.h"
#include "CInput.h"
#include "CAudio.h"
#include "CMemory.h"
#include "CCIA1.h"
#include "CCIA2.h"
#include "CVICII.h"
#include "CSID.h"
#include "globals.h"

/**
 * globals
 */
CMOS65xx *cpu = nullptr;
CMemory *mem = nullptr;
CDisplay *display = nullptr;
CInput *input = nullptr;
CAudio *audio = nullptr;
CVICII *vic = nullptr;
CCIA1 *cia1 = nullptr;
CCIA2 *cia2 = nullptr;
CSID *sid = nullptr;
bool running = true;
bool mustBreak = false;
int64_t totalCycles = 0;
char *path = nullptr;

/**
 * @brief when there's clipboard events to process, wait some frames or the SDL
 * queue may still have ctrl-v to process. probably this is not the right way to
 * do it (SDL_PumpEvents ?)
 */
#define CLIPBOARD_WAIT_FRAMES 100
int clipboardWaitFrames = CLIPBOARD_WAIT_FRAMES;

/**
 * @brief when there's clipboard events to process, process one event and skip
 * these many frames, until the queue is empty
 */
#define CLIPBOARD_SKIP_FRAMES 50
int clipboardFrameCount = CLIPBOARD_SKIP_FRAMES;

/**
 * shows banner
 */
void banner() {
    CLog::printRaw("vc64 - a c64 emulator\n");
    CLog::printRaw("(c)opyleft, valerino, y2k19\n\n");
}

/**
 * a callback for memory writes
 */
void cpuCallbackWrite(uint16_t address, uint8_t val) {
    if (address >= VIC_REGISTERS_START && address <= VIC_REGISTERS_END) {
        // accessing vic registers
        vic->write(address, val);
    } else if (address >= CIA2_REGISTERS_START &&
               address <= CIA2_REGISTERS_END) {
        // accessing cia-2 registers
        cia2->write(address, val);
    } else if (address >= CIA1_REGISTERS_START &&
               address <= CIA1_REGISTERS_END) {
        // accessing cia-1 registers
        cia1->write(address, val);
    } else {
        // default
        cpu->memory()->writeByte(address, val);
    }
}

/**
 * a callback for memory reads
 */
void cpuCallbackRead(uint16_t address, uint8_t *val) {
    if (address >= VIC_REGISTERS_START && address <= VIC_REGISTERS_END) {
        // accessing vic registers
        vic->read(address, val);
    } else if (address >= CIA2_REGISTERS_START &&
               address <= CIA2_REGISTERS_END) {
        // accessing cia-2 registers
        cia2->read(address, val);
    } else if (address >= CIA1_REGISTERS_START &&
               address <= CIA1_REGISTERS_END) {
        // accessing cia-1 registers
        cia1->read(address, val);
    } else {
        // default
        cpu->memory()->readByte(address, val);
    }
}

/**
 * callback for sdl events
 * @param event
 */
void sdlEventCallback(SDL_Event *event) {
    if (event->type == SDL_QUIT) {
        // SDL window closed, application must quit asap
        CLog::print("QUIT requested!");
        running = false;
        return;
    }

    switch (event->type) {
    case SDL_KEYUP:
    case SDL_KEYDOWN: {
        // process input
        uint32_t hotkeys = 0;
        input->update(event, &hotkeys);
        if (hotkeys == HOTKEY_DEBUGGER) {
            // we must break!
            CLog::print("DEBUGBREAK requested (works only in debugger mode!)");
            mustBreak = true;
        } else if (hotkeys == HOTKEY_PASTE_TEXT) {
            // fill the clipboard queue with fake events
            input->fillClipboardQueue();
        } else if (hotkeys == HOTKEY_FORCE_EXIT) {
            // force exit
            CLog::print("FORCE exit!");
            running = false;
        }
        break;
    }
    default:
        break;
    }
}

/**
 * prints usage
 * @param argv
 */
void usage(char **argv) {
    CLog::error("usage: %s -f <file> [-dsh]\n"
                "\t-f: file to be loaded (PRG only is supported as now)\n"
                "\t-t: run cpu test in test/6502_functional_test.bin\n"
                "\t-d: debugger\n"
                "\t-s: fullscreen\n"
                "\t-h: this help\n",
                argv[0]);
}

/**
 * just test the cpu
 */
void testCpuMain(bool debugger) {
    while (running) {
        // step the cpu
        int cycles = cpu->step(debugger, debugger ? mustBreak : false);
        if (cycles == -1) {
            // exit loop
            running = false;
            continue;
        }
        totalCycles += cycles;

        // reset break status if any
        mustBreak = false;
    }
}

/**
 * handle keys in the clipboard, one per time. this is called at every frame
 * drawn.
 */
void handleClipboard() {
    // check the clipboard queue, and process one event if not empty
    if (input->hasClipboardEvents()) {
        // we must wait some frames, or ctrl-V may still be in the
        // SDL internal queue
        if (clipboardWaitFrames > 0) {
            clipboardWaitFrames--;
        } else {
            // process clipboard one key at time, then wait some
            // frames
            if (clipboardFrameCount == 0) {
                input->processClipboardQueue();
                clipboardFrameCount = CLIPBOARD_SKIP_FRAMES;
            } else {
                clipboardFrameCount--;
            }
        }
    } else {
        // reset
        clipboardWaitFrames = CLIPBOARD_WAIT_FRAMES;
    }
}

/**
 * once the cpu has reached enough cycles to have loaded the BASIC interpreter,
 * issue a load of our prg. this trigger only once!
 */
void handlePrgLoading() {
    if (totalCycles > 14570000 && path) {
        // enough cycles passed....
        CLog::print("cycle=%ld", totalCycles);
        // TODO: determine if it's a prg, either fail....
        int res = mem->loadPrg(path);
        if (res == 0) {
            // inject run!
            SDL_SetClipboardText("RUN\n");
            input->fillClipboardQueue();
        }

        // we don't need this to trigger anymore
        path = nullptr;
    }
}

int main(int argc, char **argv) {
    CLog::enableLog(true);

    // prints title
    banner();
    bool debugger = false;
    bool fullScreen = false;
    bool isTestCpu = false;

    // parse commandline
    while (1) {
        int option = getopt(argc, argv, "dshtf:");
        if (option == -1) {
            break;
        }
        switch (option) {
        case '?':
        case 'h':
            usage(argv);
            return 1;
        case 't':
            isTestCpu = true;
            break;
        case 'f':
            path = optarg;
            CLog::print("path=%s", path);
            break;
        case 'd':
            debugger = true;
            break;
        case 's':
            fullScreen = true;
            break;

        default:
            break;
        }
    }

    bool sdlInitialized = false;
    uint32_t startTime = SDL_GetTicks();
    int res = 0;
    do {
        // initialize sdl
        res = SDL_Init(SDL_INIT_EVERYTHING);
        if (res != 0) {
            CLog::error("SDL_Init(): %s", SDL_GetError());
            break;
        }
        sdlInitialized = true;
        CLog::print("SDL initialized OK!");

        // create memory
        mem = new CMemory();
        CLog::print("Memory initialized OK!");

        // create cpu
        cpu = new CMOS65xx(mem, cpuCallbackRead, cpuCallbackWrite);
        if (cpu->reset(isTestCpu) != 0) {
            // failed to load bios
            CLog::error("Failed to load bios files!");
            break;
        }
        CLog::print("CPU initialized OK!");
        if (debugger) {
            CLog::print("debugging mode ACTIVE!");
        }

        // create additional chips
        cia1 = new CCIA1(cpu);
        cia2 = new CCIA2(cpu);
        sid = new CSID(cpu);
        vic = new CVICII(cpu, cia2);

        // create the subsystems (display, input, audio)
        try {
            display = new CDisplay(vic, "vc64-emu", fullScreen);
        } catch (std::exception ex) {
            CLog::error("display->init(): %s", ex.what());
            break;
        }
        input = new CInput(cia1);
        audio = new CAudio(sid);
        CLog::print("Display initialized OK!");

        int timeNow = SDL_GetTicks();
        if (isTestCpu) {
            // running test
            CLog::print("running cpu test mode!");
            testCpuMain(debugger);
            res = 0;
            break;
        }

        /*
        The PAL C64 has 312 scanlines giving 63*312 = 19656 cycles. If the
        display is activated (without sprites), the VIC will steal 40 cycles for
        each badline which gives us 63*312 - (25*40) = 18656 cycles. Add some
        sprites and the free cycles will decrease even more.
        */
        int cyclesPerFrame = 18656;
        int msecPerFrame = 20; // (50 : 1 = 1: x) * 1000
        int cycleCounter = cyclesPerFrame;
        timeNow = SDL_GetTicks();
        while (running) {
            // step the cpu
            int cycles = cpu->step(debugger, debugger ? mustBreak : false);
            if (cycles == -1) {
                // exit loop
                running = false;
                continue;
            }
            totalCycles += cycles;

            // reset break status if any (for the debugger)
            mustBreak = false;

            // update chips
            sid->update(totalCycles);
            int c = vic->update(totalCycles);
            cycles += c;
            cia1->update(totalCycles);
            cia2->update(totalCycles);

            // update cyclecounter
            cycleCounter -= cycles;
            if (cycleCounter <= 0) {
                display->update();
                cycleCounter += cyclesPerFrame;
                CSDLUtils::pollEvents(sdlEventCallback);

                // sleep for the remaining time, if any
                int timeThen = SDL_GetTicks();
                int diff = timeThen - timeNow;
                if (diff < msecPerFrame) {
                    SDL_Delay(msecPerFrame - diff);
                }
                timeNow = timeThen;

                // handle clipboard, if any
                handleClipboard();
            }

            // handle prg loading, if any and if enough cycles has passed, only
            // once
            handlePrgLoading();
        }
    } while (0);

    // calculate some statistics
    uint32_t endTime = SDL_GetTicks() - startTime;
    CLog::print("done, step for %02d:%02d:%02d, total CPU cycles=%" PRId64,
                endTime / 1000 / 60 / 60, endTime / 1000 / 60,
                (endTime / 1000) % 60, totalCycles);

    // done
    SAFE_DELETE(cia1)
    SAFE_DELETE(cia2)
    SAFE_DELETE(vic)
    SAFE_DELETE(sid)
    SAFE_DELETE(display)
    SAFE_DELETE(input)
    SAFE_DELETE(audio)
    SAFE_DELETE(mem)
    SAFE_DELETE(cpu)
    if (sdlInitialized) {
        SDL_Quit();
    }
    return 0;
}
