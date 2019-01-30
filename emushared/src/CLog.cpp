//
// Created by valerino on 10/01/19.
//

#include <CLog.h>
#include <errno.h>
#include <cstdlib>
#include <include/CLog.h>


FILE* CLog::_out = stdout;
FILE* CLog::_err = stderr;

void CLog::printInternal(int error, FILE* stream, const char*format,  va_list ap) {
    FILE* s;
    if (stream == nullptr) {
        if (error == 1 || error == 2) {
            s = _err;
        }
        else {
            s = _out;
        }
    }
    else {
        // use given
        s = stream;
    }

    if (error == 0) {
        fprintf(s, "[-] ");
    }
    else if (error == 1){
        fprintf(s, "[x] ");
    }
    else if (error == 2){
        // fatal !
        fprintf(s, "[XXX] ");
    }
    vfprintf(s, format, ap);
    if (error != 3) {
        // on ram, do not append \n
        fprintf(s, "\n");
    }
    fflush(s);
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

void CLog::fprint(FILE *stream, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    printInternal(0, stream, format, ap);
    va_end(ap);
}

void CLog::fprintRaw(FILE *stream, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    printInternal(3, stream, format, ap);
    va_end(ap);
}

void CLog::printRaw(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    printInternal(3, nullptr, format, ap);
    va_end(ap);
}

void CLog::print(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    printInternal(0, nullptr, format, ap);
    va_end(ap);
}

void CLog::ferror(FILE *stream, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    printInternal(1, stream, format, ap);
    va_end(ap);
}

void CLog::error(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    printInternal(1, nullptr, format, ap);
    va_end(ap);
}

void CLog::ffatal(FILE *stream, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    printInternal(2, stream, format, ap);
    va_end(ap);
    exit(1);
}

void CLog::fatal(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    printInternal(2, nullptr, format, ap);
    va_end(ap);
    exit(1);
}
