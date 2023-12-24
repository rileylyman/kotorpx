#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "kotorpx.h"
#include "log.h"

#ifndef S_ISDIR
    #define S_ISDIR(...) false
#endif

#define FS_ITER_BUF_SIZE 64  // 1024

int64_t getdelim(char** lineptr, size_t* n, int delimiter, FILE* fp);

struct fs_FileInfo {
    bool exists;
    uint64_t size_bytes;
    bool is_dir;
};

struct fs_FileInfo fs_file_info(const char* path) {
    struct fs_FileInfo fi = {};

    // Need to do this because for some reason stat crashes on windows if the file
    // doesn't exist.
    FILE* fp = fopen(path, "r");
    if (!fp) {
        fi.exists = false;
        return fi;
    }

    struct stat statbuf;
    int res = stat(path, &statbuf);
    if (res < 0) {
        fi.exists = false;
        return fi;
    }

    fi.exists = true;
    fi.size_bytes = statbuf.st_size;
    fi.is_dir = S_ISDIR(statbuf.st_mode);
    return fi;
}

OWNED char* fs_read_file_to_str(const char* path) {
    struct fs_FileInfo fi = fs_file_info(path);
    if (!fi.exists || fi.is_dir) {
        return NULL;
    }

    FILE* fp = fopen(path, "r");
    assert(fp);

    char* ret = (char*)malloc(fi.size_bytes + 1);
    ret[fi.size_bytes] = 0;
    fread(ret, 1, fi.size_bytes, fp);
    fclose(fp);
    return ret;
}