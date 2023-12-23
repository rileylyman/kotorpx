#pragma once

#include "fs.h"
#include "kotorpx.h"
#include "log.h"
#include "rmath.h"
#include <stdbool.h>
#include <stdlib.h>

enum scene_Error {
  SCENE_ERROR_NONE = 0,
  SCENE_ERROR_FILE_DOES_NOT_EXIST,
};

struct scene_Scene {
  enum scene_Error error;
};

struct scene_Command {
  enum CommandType cmd_type;
  uint32_t line_number;
  union {
    struct scene_CommandCellDimensions cell_dimensions;
    struct scene_CommandTileMap tile_map;
    struct scene_CommandPlaceTile place_tile;
  } cmd;
};

struct scene_ParsedScnFile {
  OWNED char *loaded_from;
  OWNED struct scene_Command *cmds;
};

struct scene_CommandCellDimensions {
  uint32_t x, y;
};

static bool cmd_cell_dimensions_new()

    struct scene_Scene scene_new(const char *load_path) {
  struct scene_Scene ret = {0};

  struct fs_FileInfo fi = fs_file_info(load_path);
  if (!fi.exists || fi.is_dir) {
    ret.error = true;
    E("could not load scene: file %s does not exist or is a folder\n",
      load_path);
    return ret;
  }

  struct fs_SplitIter iter = fs_split_iter_new(load_path, '\n');
  char *line = NULL;
  while ((line = fs_split_iter_next(&iter))) {
    printf("line: %s\n", line);
  }
  fs_split_iter_delete(&iter);

  return ret;
}

void scene_delete(struct scene_Scene *self) { free(self->loaded_from); }