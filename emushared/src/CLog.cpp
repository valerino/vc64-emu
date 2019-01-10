//
// Created by valerino on 10/01/19.
//

#include <CLog.h>
#include <errno.h>

FILE* CLog::_out = stdout;
FILE* CLog::_err = stderr;

void CLog::print(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    printInternal(false, format, ap);
    va_end(ap);
}

void CLog::error(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    printInternal(true, format, ap);
    va_end(ap);
}

void CLog::printInternal(bool error, const char*format,  va_list ap) {
    FILE* stream = _out;
    if (error) {
        stream = _err;
    }
    if (error) {
        fprintf(stream, "[x] ");
    }
    else {
        fprintf(stream, "[-] ");
    }
    vfprintf(stream, format, ap);
    fprintf(stream, "\n");
    fflush(stream);
}

int CLog::redirectStdout(const char *path) {
    return redirectInternal(false, path);
}

int CLog::redirectStderr(const char *path) {
    return redirectInternal(true, path);
}

int CLog::redirectInternal(bool error, const char *path) {
    FILE* f = fopen(path, "w+b");
    if (f) {
        if (error) {
            _err = f;
        }
        else {
            _out = f;
        }
        _out = f;
        return 0;
    }
    return errno;
}
