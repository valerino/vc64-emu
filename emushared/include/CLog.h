//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_CDEBUG_H
#define VC64_EMU_CDEBUG_H

#include <stdio.h>
#include <stdarg.h>

/**
 * logging utilities
 */
class CLog {
private:
    static FILE* _out;
    static FILE* _err;
    static void printInternal(bool error, const char* format, va_list ap);
    static int redirectInternal(bool error, const char *path);

public:
    /**
     * redirect stdout to file
     * @param path path to redirect stdout to
     * @return 0 on success or errno
     */
    static int redirectStdout(const char* path);

    /**
     * redirect stderr to file
     * @param path path to redirect stderr to
     * @return 0 on success or errno
     */
    static int redirectStderr(const char* path);

    /**
     * print log message, defaults to stdout
     * @param format
     * @param ...
     */
    static void print(const char* format, ...);

    /**
     * print error message, defaults to stderr
     * @param format
     * @param ...
     */
    static void error(const char* format, ...);

    /**
     * print error message, defaults to stderr, and abort
     * @param format
     * @param ...
     */
    static void fatal(const char* format, ...);
};


#endif //VC64_EMU_CDEBUG_H
