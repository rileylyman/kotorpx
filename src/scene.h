#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "kotorpx.h"
#include "log.h"
#include "str.h"

enum scene_CommandType {
    SCENE_COMMAND_UNKNOWN,
    SCENE_COMMAND_CELL_DIMENSIONS,
    SCENE_COMMAND_TILEMAP,
    SCENE_COMMAND_PLACE_TILE,
};

struct scene_Scene {};

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
    uint32_t tilemap_id;
    uint32_t tile_idx;
    int32_t cell_x, cell_y;
};

struct scene_Command {
    enum scene_CommandType cmd_type;
    uint32_t line_number;

    union {
        struct scene_CommandCellDimensions cell_dimensions;
        struct scene_CommandTileMap tilemap;
        struct scene_CommandPlaceTile place_tile;
    } cmd;
};

struct scene_ParsedScnFile {
    OWNED char* loaded_from;
    OWNED struct scene_Command* cmds;
    uint32_t n_cmds;
};

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

    str_split_next(&split);
    char* x_str = str_split_next(&split);
    char* y_str = str_split_next(&split);
    char* endptr = NULL;

    self->x = strtol(x_str, &endptr, 10);
    if (strcmp(endptr, "px") != 0) {
        E("%s:%d expected px suffix for cell dimension x, got %s\n",
          scene_path,
          line_no,
          x_str);
        return false;
    }
    self->y = strtol(y_str, &endptr, 10);
    if (strcmp(endptr, "px") != 0) {
        E("%s:%d expected px suffix for cell dimension y, got %s\n",
          scene_path,
          line_no,
          y_str);
        return false;
    }

    return true;
}

static bool cmd_tilemap_parse(
    struct scene_CommandTileMap* self,
    char* line,
    uint32_t line_no,
    const char* scene_path
) {
    struct str_SplitIter split = str_split(line, ' ');
    if (split.num_tokens != 3) {
        E("%s:%d :tilemap expects 2 arguments, got %d\n",
          scene_path,
          line_no,
          split.num_tokens - 1);
        return false;
    }

    str_split_next(&split);
    char* id_str = str_split_next(&split);
    char* tilemap_path = str_split_next(&split);
    char* endptr = NULL;

    self->id = strtol(id_str, &endptr, 10);
    if (*endptr != 0) {
        E("%s:%d invalid id: %s\n", scene_path, line_no, id_str);
        return false;
    }

    struct fs_FileInfo atlas_fi = fs_file_info(tilemap_path);
    if (!atlas_fi.exists) {
        E("%s:%d invalid tilemap path: %s does not exist\n",
          scene_path,
          line_no,
          tilemap_path);
        return false;
    }
    if (!atlas_fi.is_dir) {
        E("%s:%d invalid tilemap path: %s is not a directory\n",
          scene_path,
          line_no,
          tilemap_path);
        return false;
    }
    self->atlas_path = (char*)malloc(strlen(tilemap_path) + 1);
    strcpy_s(self->atlas_path, strlen(tilemap_path) + 1, tilemap_path);
    strcpy_s(
        self->dir_atlas_extension,
        sizeof(self->dir_atlas_extension),
        ".png"
    );
    self->is_dir_atlas = true;

    return true;
}

static bool cmd_place_tile_parse(
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
    self->tilemap_id = strtol(id_str, &endptr, 10);
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

bool scene_load(struct scene_Scene* self, const char* load_path) {
    struct scene_ParsedScnFile parsed = {};

    struct fs_FileInfo fi = fs_file_info(load_path);
    if (!fi.exists || fi.is_dir) {
        E("could not load scene: file %s does not exist or is a folder\n",
          load_path);
        return false;
    }

    char* scene_file_contents = fs_read_file_to_str(load_path);
    assert(scene_file_contents);

    struct str_SplitIter line_iter = str_split(scene_file_contents, '\n');
    char* line;
    while ((line = str_split_next(&line_iter))) {
        if (str_starts_with(line, ":")) {
            parsed.n_cmds++;
        }
    }

    parsed.cmds = (struct scene_Command*)malloc(
        sizeof(struct scene_Command) * parsed.n_cmds
    );
    assert(parsed.cmds);

    bool error = false;
    str_split_restart(&line_iter);

    while ((line = str_split_next(&line_iter))) {
        struct scene_Command* cur_cmd = &parsed.cmds[line_iter.next_token - 1];
        uint32_t line_no = str_split_seen_seps(&line_iter) + 1;
        cur_cmd->line_number = line_no;
        if (str_starts_with(line, ":cell_dimensions")) {
            cur_cmd->cmd_type = SCENE_COMMAND_CELL_DIMENSIONS;
            error = error
                || !cmd_cell_dimensions_parse(
                        &cur_cmd->cmd.cell_dimensions,
                        line,
                        cur_cmd->line_number,
                        load_path
                );
        } else if (str_starts_with(line, ":tilemap")) {
            cur_cmd->cmd_type = SCENE_COMMAND_TILEMAP;
            error = error
                || !cmd_tilemap_parse(
                        &cur_cmd->cmd.tilemap,
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
                line + 1
            );
        }
    }

    if (error) {
        E("could not parse scene file %s\n", load_path);
        return false;
    }

    return true;
}

void scene_delete(struct scene_Scene* self) {}