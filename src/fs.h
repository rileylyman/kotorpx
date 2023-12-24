#pragma once

#include "kotorpx.h"
#include "log.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifndef S_ISDIR
#define S_ISDIR(...) false
#endif

#define FS_ITER_BUF_SIZE 64 // 1024

ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *fp);

enum fs_Error {
  FS_ERROR_NONE = 0,
  FS_ERROR_FILE_DOES_NOT_EXIST,
  FS_ERROR_IS_A_DIR,
};

struct fs_FileInfo {
  enum fs_Error error;

  bool exists;
  uint64_t size_bytes;
  bool is_dir;
};

struct fs_FileInfo fs_file_info(const char *path) {
  struct fs_FileInfo fi = {0};

  // Need to do this because for some reason stat crashes on windows if the file
  // doesn't exist.
  int fd = open(path, 0);
  if (fd < 0) {
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

struct fs_SplitIter {
  enum fs_Error error;

  char sep;
  OWNED FILE *file;
  char *line;
  uint32_t token_number;
};

struct fs_SplitIter fs_split_iter_new(const char *path, char sep) {
  struct fs_SplitIter ret = {0};

  struct fs_FileInfo fi = fs_file_info(path);
  if (!fi.exists) {
    ret.error = FS_ERROR_FILE_DOES_NOT_EXIST;
    return ret;
  }

  if (fi.is_dir) {
    ret.error = FS_ERROR_IS_A_DIR;
    return ret;
  }

  FILE *file = fopen(path, "r");
  assert(file);
  ret.file = file;
  ret.sep = sep;
  ret.line = NULL;
  ret.token_number = 0;

  return ret;
}

char *fs_split_iter_next(struct fs_SplitIter *self) {
  uint64_t size;
  int64_t n_bytes = getdelim(&self->line, &size, self->sep, self->file);
  if (n_bytes < 0) {
    return NULL;
  }

  self->token_number++;

  if (self->line[0] == self->sep) {
    return fs_split_iter_next(self);
  }

  if (self->line[n_bytes - 1] == self->sep) {
    self->line[n_bytes - 1] = 0;
  }
  return self->line;
}

void fs_split_iter_restart(struct fs_SplitIter *self) {
  fseek(self->file, 0, SEEK_SET);
  self->token_number = 0;
}

void fs_split_iter_delete(struct fs_SplitIter *self) {
  fclose(self->file);
  free(self->line);
}