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
    char sep;
    uint32_t num_tokens, next_token;
    uint32_t total_len;
};

struct str_SplitIter str_split(char* s, char sep) {
    struct str_SplitIter self = {0};
    self.s = self.cur = s;
    self.sep = sep;
    self.total_len = strlen(s);
    self.num_tokens = 0;
    self.next_token = 0;

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
    self->next_token++;
    while (*self->cur != 0 && self->cur - self->s < self->total_len) {
        self->cur++;
    }
    while (*self->cur == 0 && self->cur - self->s < self->total_len) {
        self->cur++;
    }
    return ret;
}

uint32_t str_split_seen_seps(struct str_SplitIter* self) {
    char* beg = self->s;
    uint32_t ret = 0;
    while (beg != self->cur) {
        if (*beg == 0) {
            ret++;
        }
        beg++;
    }
    return ret;
}

void str_split_restart(struct str_SplitIter* self) {
    self->cur = self->s;
    self->next_token = 0;
}
