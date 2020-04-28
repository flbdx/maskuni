/*
 * Copyright 2020 Florent Bondoux
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "getdelim.h"
#include <stdlib.h>
#include <errno.h>

static inline size_t min_alloc() { return 4; }
static inline size_t default_alloc() { return 128; }
static inline size_t grow_alloc(size_t n) { return n + 128; }

ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream) {
    if (n == NULL || lineptr == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }
    
    if (*lineptr == NULL || *n < min_alloc()) {
        size_t init_size = default_alloc();
        char *lp = (char *) realloc(*lineptr, init_size);
        if (lp == NULL) {
            errno = ENOMEM;
            return -1;
        }
        *lineptr = lp;
        *n = init_size;
    }
    
    ssize_t eof = 0;
    size_t len = 0;
    while (1) {
        int c = fgetc(stream);
        if (c == EOF) {
            eof = -1;
            break;
        }
        
        if (*n < len + 2) { // c + \0
            size_t grow_size = grow_alloc(*n);
            char *lp = (char *) realloc(*lineptr, grow_size);
            if (lp == NULL) {
                errno = ENOMEM;
                return -1;
            }
            *lineptr = lp;
            *n = grow_size;
        }
        
        (*lineptr)[len] = c;
        len += 1;
        
        if (c == delim) {
            break;
        }
    }
    (*lineptr)[len] = '\0';
    if (len != 0) {
        return len;
    }
    return eof;
}
