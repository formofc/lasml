#ifndef __BIGFILE_H__
#define __BIGFILE_H__ 1

// Im not sure interpreter itself could handle programms this complexity, but

#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32 // Im not sure interpreter itself could handle programms this complexity, but
static FILE* fopen_big(const char* filepath, const char* mode) {
    return fopen(filepath, mode);
}
static int fseek_big(FILE* file, long long offset, int mode) {
    return _fseeki64(file, offset, mode);
}
static long long ftell_big(FILE* file) {
    return _ftelli64(file);
}

#else
static FILE* fopen_big(const char* filepath, const char* mode) {
    return fopen64(filepath, mode);
}
static int fseek_big(FILE* file, long long offset, int mode) {
    return fseeko64(file, offset, mode);
}
static long long ftell_big(FILE* file) {
    return ftello64(file);
}
#endif

static bool get_file_size(FILE* file, size_t* result) {
    long long last_offset;
    
    last_offset = ftell_big(file);
    if (fseek_big(file, 0, SEEK_END)) {
        perror("Failed to get size of the file");
        return false;
    }
    *result = ftell_big(file);
    if (fseek_big(file, last_offset, SEEK_SET)) {
        perror("Failed to get size of the file");
        return false;
    }
    return true;

}

#endif // __BIGFILE_H__