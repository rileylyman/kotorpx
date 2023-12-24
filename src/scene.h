#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "fs.h"
#include "kotorpx.h"
#include "log.h"

enum scene_Error {
  SCENE_ERROR_NONE = 0,
  SCENE_ERROR_FILE_DOES_NOT_EXIST,
};

enum scene_CommandType {
  SCENE_COMMAND_UNKNOWN,
  SCENE_COMMAND_CELL_DIMENSIONS,
  SCENE_COMMAND_TILE_MAP,
  SCENE_COMMAND_PLACE_TILE,
};

struct scene_Scene {
  enum scene_Error error;
};

struct scene_CommandCellDimensions {
  int32_t x, y;
};

struct scene_CommandTileMap {
  uint32_t id;
  OWNED char *atlas_path;
  char dir_atlas_extension[15];
  bool is_dir_atlas;
};

struct scene_CommandPlaceTile {
  uint32_t tile_map_id;
  uint32_t tile_idx;
  int32_t cell_x, cell_y;
};

struct scene_Command {
  enum scene_CommandType cmd_type;
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
  uint32_t n_cmds;
};

static bool str_starts_with(const char *s, const char *pre) {
  return strncmp(s, pre, strlen(pre)) == 0;
}

static int32_t str_find_index(const char *str, char delim, uint32_t start_idx) {
  uint32_t len = strlen(str);
  if (start_idx >= len) {
    return -1;
  }
  for (uint32_t i = start_idx; i < len; i++) {
    if (str[i] == delim) {
      return i;
    }
  }
  return -1;
}

static bool parse_px_value(char *s, int32_t *n) {
  char *endptr = s;
  int32_t ret = strtol(s, &endptr, 10);
  if (endptr == s) {
    E("invalid dimension: nan\n");
    return false;
  }
  if (strlen(endptr) != 2 || strcmp(endptr, "px")) {
    E("expected 'px' suffix\n");
    return false;
  }
  *n = ret;
  return true;
}

static bool cmd_cell_dimensions_new(struct scene_CommandCellDimensions *self,
                                    char *line) {
  *self = (struct scene_CommandCellDimensions){0};
  int32_t cmd_end = str_find_index(line, ' ', 0);
  if (cmd_end < 0 || cmd_end + 1 >= strlen(line)) {
    return false;
  }

  int32_t x_end = str_find_index(line, ' ', cmd_end + 1);
  if (x_end < 0 || x_end + 1 >= strlen(line)) {
    return false;
  }
  int32_t y_end = str_find_index(line, ' ', x_end + 1);

  line[x_end] = 0;
  if (y_end >= 0) {
    line[y_end] = 0;
  }

  char *xstr = &line[cmd_end + 1];
  char *ystr = &line[x_end + 1];

  if (!parse_px_value(xstr, &self->x) || !parse_px_value(ystr, &self->y)) {
    return false;
  }

  return true;
}

static bool cmd_tile_map_new(struct scene_CommandTileMap *self, char *line) {
  *self = (struct scene_CommandTileMap){0};
  int32_t cmd_end = str_find_index(line, ' ', 0);
  int32_t id_end = str_find_index(line, ' ', cmd_end + 1);
  int32_t path_end = str_find_index(line, ' ', id_end + 1);

  if (cmd_end < 0) {
    E("no argument after :tile_map\n");
    return false;
  }

  if (id_end < 0) {
    E("no path found after :tile_map id\n");
    return false;
  }

  if (path_end >= 0) {
    line[path_end] = 0;
  }
  line[id_end] = 0;

  char *id_str = &line[cmd_end + 1];
  char *id_end_str = id_str;
  int32_t id = strtol(id_str, &id_end_str, 10);
  if (id_end_str != &line[id_end]) {
    E("wrong id format: %s\n", id_str);
    return false;
  }
  self->id = id;

  char *path_str = &line[id_end + 1];
  struct fs_FileInfo fi = fs_file_info(path_str);
  if (!fi.exists) {
    E("path '%s' does not exist for tile map\n", path_str);
    return false;
  }
  if (!fi.is_dir) {
    E("path '%s' is not a directory\n", path_str);
    return false;
  }
  self->is_dir_atlas = true;
  strcpy(self->dir_atlas_extension, ".png");
  uint32_t path_size = strlen(path_str) + 1;
  self->atlas_path = malloc(path_size);
  memset(self->atlas_path, 0, path_size);
  strcpy(self->atlas_path, path_str);

  return true;
}

bool cmd_place_tile_new(struct scene_CommandPlaceTile* self, char* line) {
  return false;
}

struct scene_Scene scene_new(const char *load_path) {
  struct scene_Scene ret = {0};
  struct scene_ParsedScnFile parsed = {0};

  struct fs_FileInfo fi = fs_file_info(load_path);
  if (!fi.exists || fi.is_dir) {
    ret.error = true;
    E("could not load scene: file %s does not exist or is a folder\n",
      load_path);
    return ret;
  }

  struct fs_SplitIter iter = fs_split_iter_new(load_path, '\n');
  char *line = fs_split_iter_next(&iter);

  for (; line; line = fs_split_iter_next(&iter)) {
    if (strcmp(line, ":") == 0) {
      parsed.n_cmds++;
    }
  }

  parsed.cmds = malloc(sizeof(struct scene_Command) * parsed.n_cmds);
  assert(parsed.cmds);
  uint32_t cmd_offset = 0;
  bool error = false;

  fs_split_iter_restart(&iter);
  line = fs_split_iter_next(&iter);

  for (; line; line = fs_split_iter_next(&iter)) {
    struct scene_Command *cur_cmd = &parsed.cmds[cmd_offset];
    cur_cmd->line_number = iter.token_number;
    bool error_this_time = false;
    if (str_starts_with(line, ":cell_dimensions")) {
      cur_cmd->cmd_type = SCENE_COMMAND_CELL_DIMENSIONS;
      error_this_time =
          !cmd_cell_dimensions_new(&cur_cmd->cmd.cell_dimensions, line);
    } else if (str_starts_with(line, ":tile_map")) {
      cur_cmd->cmd_type = SCENE_COMMAND_TILE_MAP;
      error_this_time = !cmd_tile_map_new(&cur_cmd->cmd.tile_map, line);
    } else if (str_starts_with(line, ":place_tile")) {
      cur_cmd->cmd_type = SCENE_COMMAND_PLACE_TILE;
      error_this_time = !cmd_place_tile_new(&cur_cmd->cmd.place_tile, line);
    } else {
      error_this_time = true;
      int32_t space_idx = str_find_index(line, ' ', 0);
      if (space_idx >= 0) {
        line[space_idx] = 0;
      }
      E("unrecognized command %s\n", line);
    }
    if (error_this_time) {
      E("error on line %d\n", iter.token_number);
      error = true;
    }
    cmd_offset++;
  }

  if (error) {
    E("could not parse scene file %s\n", load_path);
  }

  return ret;
}

void scene_delete(struct scene_Scene *self) {}