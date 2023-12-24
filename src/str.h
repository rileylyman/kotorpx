#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

bool str_starts_with(const char* s, const char* pre) {
    return strncmp(s, pre, strlen(pre)) == 0;
}

int32_t str_find_index(const char* str, char delim, uint32_t start_idx) {
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

struct str_SplitIter {
    char *s, *cur;
    uint32_t num_tokens;
    uint32_t total_len;
};

struct str_SplitIter str_split(char* s, char sep) {
    struct str_SplitIter self = {0};
    self.s = self.cur = s;
    self.total_len = strlen(s);
    self.num_tokens = 0;

    bool in_token = false;
    for (uint32_t i = 0; i < self.total_len; i++) {
        if (self.s[i] != sep && !in_token) {
            self.num_tokens++;
            in_token = true;
        }
        if (self.s[i] == sep) {
            self.s[i] = 0;
            in_token = false;
        }
    }
    return self;
}

char* str_split_next(struct str_SplitIter* self) {
    if (self->cur - self->s >= self->total_len) {
        return NULL;
    }
    char* ret = self->cur;
    while (*self->cur != 0 && self->cur - self->s < self->total_len) {
        self->cur++;
    }
    while (*self->cur == 0 && self->cur - self->s < self->total_len) {
        self->cur++;
    }
    return ret;
}
