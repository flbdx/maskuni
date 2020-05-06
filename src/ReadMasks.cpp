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

#include "config.h"

#include "ReadMasks.h"
#include "ExpandCharset.h"
#include "utf_conv.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef HAVE_GETLINE
# include "getline.h"
#endif

namespace Maskgen {

/* create a mask from a string and all the defined charsets
 * Return an empty mask if there was an error (undefined charset)
 * 
 * note that T('?') is valid for unicode as the codepoint of ASCII character is their value
 */
template<typename T, T escapeChar = T('?')>
static Mask<T> readMask(const std::vector<T> &str, const CharsetMap<T> &defined_charsets) {
    Mask<T> mask;
    for (size_t i = 0; i < str.size();) {
        if (str[i] == escapeChar && i + 1 < str.size()) {
            T key = str[i+1];
            if (key == escapeChar) {
                mask.push_charset_right(&(str[i]), 1);
            }
            else {
                auto it_range = defined_charsets.equal_range(key);
                if (it_range.first != it_range.second) {
                    auto it_charset = std::prev(it_range.second);
                    mask.push_charset_right(it_charset->second.cset.data(), it_charset->second.cset.size());
                }
                else {
                    if (std::is_same<T, char>::value) {
                        fprintf(stderr, "Error: charset '?%c' is not defined\n", key);
                    }
                    else {
                        char key_str[5];
                        int l = UTF::impl::CpToUtf8::write(key, key_str);
                        key_str[l] = 0;
                        fprintf(stderr, "Error: charset '?%s' is not defined\n", key_str);
                    }
                    return Mask<T>();
                }
            }
            i += 2;
        }
        else {
            mask.push_charset_right(&(str[i]), 1);
            i++;
        }
    }
    
    return mask;
}

/* Read a line from a mask file
 * 
 * note that T('?') and T(',') are valid for unicode as the codepoint of ASCII character is their value
 */
template<typename T, T charsetEscapeChar = T('?'), T lineEscapeChar = ('\\'), T separatorChar = T(','), T commentChar = T('#')>
static bool readMaskLine(const T *line, size_t line_len, const CharsetMap<T> &charsets, MaskList<T> &ml) {
    CharsetMap<T> effective_charsets = charsets;
                
    std::vector<std::vector<T>> tokens;
    tokens.resize(1);
    
    // remove commented and empty lines
    if (line_len == 0 || line[0] == commentChar) {
        return true;
    }
    
    // split the line on ,
    for (size_t i = 0; i < line_len; ) {
        T c = line[i];
        // escaped characters
        if (c == lineEscapeChar && i + 1 < line_len ) {
            tokens.back().push_back(line[i+1]);
            i += 2;
        }
        else if (c == separatorChar) { // an unescaped ,
            // finish this token and skip the ,
            tokens.resize(tokens.size() + 1);
            i++;
        }
        else {
            tokens.back().push_back(c);
            i++;
        }
    }
    
    // we won't name a charset with 2 digits...
    if (tokens.size() > 10) {
        fprintf(stderr, "Error: too many custom charsets defined (max: 9)\n");
        return false;
    }
    
    // create the user defined charsets without expanding them
    for (size_t n = 0; n + 1 < tokens.size(); n++) {
        if (tokens[n].size() == 0) {
            fprintf(stderr, "Error: empty custom charset\n");
            return false;
        }
        T charset_key = T('1' + n);
        typename decltype(effective_charsets)::mapped_type new_charset;
        new_charset.cset = tokens[n];
        new_charset.final = false;
        effective_charsets.insert(std::make_pair(charset_key, new_charset));
    }
    
    // now expand all the user defined charsets
    // expandCharset checks for recursive charset definitions so we can safely expand all the user defined charsets
    for (size_t n = 0; n + 1 < tokens.size(); n++) {
        T charset_key = T('1' + n);
        if (!expandCharset<T, charsetEscapeChar>(effective_charsets, charset_key)) {
            fprintf(stderr, "Error while reading the inline custom charset '%c'\n", (int) charset_key);
            return false;
        }
    }
    
    Mask<T> mask = readMask<T, charsetEscapeChar>(tokens.back(), effective_charsets);
    if (mask.getWidth() == 0) {
        return false;
    }
    ml.pushMask(mask);
    return true;
}

bool readMaskListAscii(const char *spec, const CharsetMapAscii &charsets, MaskList<char> &ml)
{
#if defined(__WINDOWS__) || defined(__CYGWIN__)
    int fd = open(spec, O_RDONLY | O_BINARY);
#else
    int fd = open(spec, O_RDONLY);
#endif
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
            char *line = NULL;
            size_t line_size = 0;
            FILE *f = fdopen(fd, "rb");
            ssize_t r;
            unsigned int line_number = 0;
            while ((r = getline(&line, &line_size, f))!= -1) {
                line_number++;
                if (r >= 2 && line[r-1] == '\n' && line[r-2] == '\r') {
                    line[r-2] = '\0';
                    r -= 2;
                }
                else if (r >= 1 && line[r-1] == '\n') {
                    line[r-1] = '\0';
                    r -= 1;
                }
                if (r == 0) {
                    continue;
                }
                
                if (!readMaskLine<char>(line, r, charsets, ml)) {
                    fprintf(stderr, "Error while reading '%s' at line %u\n", spec, line_number);
                    free(line);
                    return false;
                }
            }
            
            free(line);
            close(fd);
            return true;
        }
        else {
            close(fd);
        }
    }
    
    std::vector<char> spec_v(spec, spec + strlen(spec));
    Mask<char> mask = readMask<char>(spec_v, charsets);
    if (mask.getWidth() == 0) {
        return false;
    }
    ml.pushMask(mask);
    
    return true;
}

bool readMaskListUtf8(const char *spec, const CharsetMapUnicode &charsets, MaskList<uint32_t> &ml)
{
#if defined(__WINDOWS__) || defined(__CYGWIN__)
    int fd = open(spec, O_RDONLY | O_BINARY);
#else
    int fd = open(spec, O_RDONLY);
#endif
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
            char *line = NULL;
            size_t line_size = 0;
            uint32_t *conv_buf = NULL;
            size_t conv_buf_size = 0;
            size_t conv_consumed = 0, conv_written = 0;
            FILE *f = fdopen(fd, "rb");
            ssize_t r;
            unsigned int line_number = 0;
            while ((r = getline(&line, &line_size, f)) >= 0) {
                line_number++;
                if (r >= 2 && line[r-1] == '\n' && line[r-2] == '\r') {
                    line[r-2] = '\0';
                    r -= 2;
                }
                else if (r >= 1 && line[r-1] == '\n') {
                    line[r-1] = '\0';
                    r -= 1;
                }
                if (r == 0) {
                    continue;
                }
                
                UTF::decode_utf8(line, r, &conv_buf, &conv_buf_size, &conv_consumed, &conv_written);
                if (conv_consumed != (size_t) r) {
                    fprintf(stderr, "Error: the mask file '%s' contains invalid UTF-8 chars at line %u\n", spec, line_number);
                    free(line);
                    free(conv_buf);
                    return false;
                }
                
                if (!readMaskLine<uint32_t>(conv_buf, conv_written, charsets, ml)) {
                    fprintf(stderr, "Error while reading '%s' at line %u\n", spec, line_number);
                    free(line);
                    free(conv_buf);
                    return false;
                }
            }
            
            free(line);
            free(conv_buf);
            close(fd);
            return true;
        }
        else {
            close(fd);
        }
    }
    
    {
        size_t spec_len = strlen(spec);
        uint32_t *conv_buf = NULL;
        size_t conv_buf_size = 0;
        size_t conv_consumed = 0, conv_written = 0;
        UTF::decode_utf8(spec, spec_len, &conv_buf, &conv_buf_size, &conv_consumed, &conv_written);
        if (spec_len != conv_consumed) {
            fprintf(stderr, "Error: the mask '%s' contains invalid UTF-8 chars\n", spec);
            free(conv_buf);
            return false;
        }
        std::vector<uint32_t> spec_v(conv_buf, conv_buf + conv_written);
        free(conv_buf);
        
        Mask<uint32_t> mask = readMask<uint32_t>(spec_v, charsets);
        if (mask.getWidth() == 0) {
            return false;
        }
        ml.pushMask(mask);
    }
    
    return true;
}

}
