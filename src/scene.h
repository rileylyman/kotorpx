#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "fs.h"
#include "kotorpx.h"
#include "log.h"
#include "str.h"

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
    OWNED char* atlas_path;
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
    OWNED char* loaded_from;
    OWNED struct scene_Command* cmds;
    uint32_t n_cmds;
};

static bool parse_px_value(char* s, int32_t* n) {
    char* endptr = s;
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

static bool cmd_cell_dimensions_parse(
    struct scene_CommandCellDimensions* self,
    char* line,
    uint32_t line_no,
    const char* scene_path
) {
    memset(self, 0, sizeof(*self));

    struct str_SplitIter split = str_split(line, ' ');
    if (split.num_tokens != 3) {
        E("%s:%d :cell_dimensions expected 2 arguments, got %d\n",
          scene_path,
          line_no,
          split.num_tokens - 1);
    }

    return true;
}

static bool cmd_tile_map_parse(
    struct scene_CommandTileMap* self,
    char* line,
    uint32_t line_no,
    const char* scene_path
) {
    *self = (struct scene_CommandTileMap) {0};
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

    char* id_str = &line[cmd_end + 1];
    char* id_end_str = id_str;
    int32_t id = strtol(id_str, &id_end_str, 10);
    if (id_end_str != &line[id_end]) {
        E("wrong id format: %s\n", id_str);
        return false;
    }
    self->id = id;

    char* path_str = &line[id_end + 1];
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

bool cmd_place_tile_parse(
    struct scene_CommandPlaceTile* self,
    char* line,
    uint32_t line_no,
    const char* scene_path
) {
    struct str_SplitIter split = str_split(line, ' ');
    if (split.num_tokens != 5) {
        E("%s:%d :place_tile requires 4 arguments, got %d\n",
          scene_path,
          line_no,
          split.num_tokens - 1);
        return false;
    }
    str_split_next(&split);  // skip command
    char* id_str = str_split_next(&split);
    char* tile_idx_str = str_split_next(&split);
    char* x_str = str_split_next(&split);
    char* y_str = str_split_next(&split);

    bool error = false;

    char* endptr = NULL;
    self->tile_map_id = strtol(id_str, &endptr, 10);
    if (*endptr != 0) {
        error = true;
        E("%s:%d invalid id: %s\n", scene_path, line_no, id_str);
    }

    self->tile_idx = strtol(tile_idx_str, &endptr, 10);
    if (*endptr != 0) {
        error = true;
        E("%s:%d invalid tile index: %s\n", scene_path, line_no, tile_idx_str);
    }

    self->cell_x = strtol(x_str, &endptr, 10);
    if (*endptr != 0) {
        error = true;
        E("%s:%d invalid cell_x: %s\n", scene_path, line_no, x_str);
    }

    self->cell_y = strtol(y_str, &endptr, 10);
    if (*endptr != 0) {
        error = true;
        E("%s:%d invalid cell_y: %s\n", scene_path, line_no, y_str);
    }

    return !error;
}

struct scene_Scene scene_new(const char* load_path) {
    struct scene_Scene ret = {0};
    struct scene_ParsedScnFile parsed = {0};

    struct fs_FileInfo fi = fs_file_info(load_path);
    if (!fi.exists || fi.is_dir) {
        ret.error = SCENE_ERROR_FILE_DOES_NOT_EXIST;
        E("could not load scene: file %s does not exist or is a folder\n",
          load_path);
        return ret;
    }

    struct fs_SplitIter iter = fs_split_iter_new(load_path, '\n');
    char* line = fs_split_iter_next(&iter);

    for (; line; line = fs_split_iter_next(&iter)) {
        if (strcmp(line, ":") == 0) {
            parsed.n_cmds++;
        }
    }

    parsed.cmds = (struct scene_Command*)malloc(
        sizeof(struct scene_Command) * parsed.n_cmds
    );
    assert(parsed.cmds);
    uint32_t cmd_offset = 0;
    bool error = false;

    fs_split_iter_restart(&iter);
    line = fs_split_iter_next(&iter);

    for (; line; line = fs_split_iter_next(&iter)) {
        struct scene_Command* cur_cmd = &parsed.cmds[cmd_offset];
        cur_cmd->line_number = iter.token_number;
        if (str_starts_with(line, ":cell_dimensions")) {
            cur_cmd->cmd_type = SCENE_COMMAND_CELL_DIMENSIONS;
            error = error
                || !cmd_cell_dimensions_parse(
                        &cur_cmd->cmd.cell_dimensions,
                        line,
                        cur_cmd->line_number,
                        load_path
                );
        } else if (str_starts_with(line, ":tile_map")) {
            cur_cmd->cmd_type = SCENE_COMMAND_TILE_MAP;
            error = error
                || !cmd_tile_map_parse(
                        &cur_cmd->cmd.tile_map,
                        line,
                        cur_cmd->line_number,
                        load_path
                );
        } else if (str_starts_with(line, ":place_tile")) {
            cur_cmd->cmd_type = SCENE_COMMAND_PLACE_TILE;
            error = error
                || !cmd_place_tile_parse(
                        &cur_cmd->cmd.place_tile,
                        line,
                        cur_cmd->line_number,
                        load_path
                );
        } else {
            error = true;
            int32_t space_idx = str_find_index(line, ' ', 0);
            if (space_idx >= 0) {
                line[space_idx] = 0;
            }
            printf(
                "%s:%d %s: unrecognized command\n",
                load_path,
                cur_cmd->line_number,
                line
            );
        }
        cmd_offset++;
    }

    if (error) {
        E("could not parse scene file %s\n", load_path);
    }

    return ret;
}

void scene_delete(struct scene_Scene* self) {}