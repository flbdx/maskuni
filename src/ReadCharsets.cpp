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

#include "ReadCharsets.h"
#include "ExpandCharset.h"
#include "utf_conv.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace Maskgen {
    
static const char default_charset_l[] = "abcdefghijklmnopqrstuvwxyz";
static const char default_charset_u[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char default_charset_d[] = "0123456789";
static const char default_charset_s[] = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
static const char default_charset_h[] = "0123456789abcdef";
static const char default_charset_H[] = "0123456789ABCDEF";
static const char default_charset_n[] = "\n";
static const char default_charset_r[] = "\r";
static const char default_charset_a[] = "?l?u?d?s";
static const unsigned char default_charset_b[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

std::vector<char> expandCharsetAscii(const std::vector<char> &charset, const CharsetMapAscii &default_charsets, char charset_name)
{
    return expandCharset<char>(charset, default_charsets, charset_name);
}

std::vector<uint32_t> expandCharsetUnicode(const std::vector<uint32_t> &charset, const CharsetMapUnicode &default_charsets, uint32_t charset_name)
{
    return expandCharset<uint32_t>(charset, default_charsets, charset_name);
}

void initDefaultCharsetsAscii(CharsetMapAscii &charsets)
{
    charsets['l'] = DefaultCharset<char>(default_charset_l, 26, true);
    charsets['u'] = DefaultCharset<char>(default_charset_u, 26, true);
    charsets['d'] = DefaultCharset<char>(default_charset_d, 10, true);
    charsets['s'] = DefaultCharset<char>(default_charset_s, sizeof(default_charset_s) - 1, true);
    charsets['h'] = DefaultCharset<char>(default_charset_h, 16, true);
    charsets['H'] = DefaultCharset<char>(default_charset_H, 16, true);
    charsets['b'] = DefaultCharset<char>((const char *) default_charset_b, 256, true);
    charsets['n'] = DefaultCharset<char>(default_charset_n, 1, true);
    charsets['r'] = DefaultCharset<char>(default_charset_r, 1, true);
    charsets['a'] = DefaultCharset<char>(default_charset_a, sizeof(default_charset_a) - 1, false);
//     charsets['a'].cset = expandCharsetAscii(charsets['a'].cset, charsets, 'a');
}

void initDefaultCharsetsUnicode(CharsetMapUnicode &charsets)
{
    size_t consumed = 0, written = 0;
    UTF::decode_utf8(default_charset_l, 26, std::back_inserter(charsets['l'].cset), &consumed, &written);
    charsets['l'].final = true;
    
    UTF::decode_utf8(default_charset_u, 26, std::back_inserter(charsets['u'].cset), &consumed, &written);
    charsets['u'].final = true;
    
    UTF::decode_utf8(default_charset_d, 10, std::back_inserter(charsets['d'].cset), &consumed, &written);
    charsets['d'].final = true;
    
    UTF::decode_utf8(default_charset_s, sizeof(default_charset_s) - 1, std::back_inserter(charsets['s'].cset), &consumed, &written);
    charsets['s'].final = true;
    
    UTF::decode_utf8(default_charset_h, 16, std::back_inserter(charsets['H'].cset), &consumed, &written);
    charsets['h'].final = true;
    
    UTF::decode_utf8(default_charset_H, 16, std::back_inserter(charsets['H'].cset), &consumed, &written);
    charsets['H'].final = true;
    
    UTF::decode_utf8(default_charset_n, 1, std::back_inserter(charsets['n'].cset), &consumed, &written);
    charsets['n'].final = true;
    
    UTF::decode_utf8(default_charset_r, 1, std::back_inserter(charsets['r'].cset), &consumed, &written);
    charsets['r'].final = true;
    
    UTF::decode_utf8(default_charset_a, sizeof(default_charset_a) - 1, std::back_inserter(charsets['a'].cset), &consumed, &written);
//     charsets['a'].cset = expandCharsetUnicode(charsets['a'].cset, charsets, uint32_t('a'));
    charsets['a'].final = false;
}

bool readCharsetAscii(const char *spec, std::vector<char> &charset) {
    int fd = open(spec, O_RDONLY);
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
            charset.clear();
            char buffer[512];
            ssize_t r;
            while (true) {
                r = read(fd, buffer, sizeof(buffer));
                if (r < 0) {
                    fprintf(stderr, "Error: can't read the charset file '%s': %m", spec);
                    return false;
                }
                if (r == 0) {
                    break;
                }
                charset.insert(charset.end(), buffer, buffer + r);
            }
            
            close(fd);
            
            if (charset.size() == 0) {
                fprintf(stderr, "Error: the charset '%s' is empty\n", spec);
            }
            
            return charset.size() != 0;
        }
        else {
            close(fd);
        }
    }
    
    charset.clear();
    const char *c = spec;
    while (*c != 0) {
        charset.push_back(*c);
        c++;
    }
    
    if (charset.size() == 0) {
        fprintf(stderr, "Error: the charset '%s' is empty\n", spec);
    }
    
    return charset.size() != 0;
}

bool readCharsetUtf8(const char *spec, std::vector<uint32_t> &charset) {
    int fd = open(spec, O_RDONLY);
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
            charset.clear();
            char *buffer = (char *) malloc(st.st_size);
            ssize_t r = read(fd, buffer, st.st_size);
            if (r < 0) {
                fprintf(stderr, "Error: can't read the charset file '%s': %m", spec);
                free(buffer);
                return false;
            }
            if (r == 0) {
                free(buffer);
                return false;
            }
            size_t conv_consumed = 0, conv_written = 0;
            UTF::decode_utf8(buffer, r, std::back_inserter(charset), &conv_consumed, &conv_written);
            if (conv_consumed != (size_t) r) {
                fprintf(stderr, "Error: the charset '%s' contains invalid UTF-8 chars\n", spec);
                free(buffer);
                return false;
            }
            
            free(buffer);
            
            close(fd);
            
            if (charset.size() == 0) {
                fprintf(stderr, "Error: the charset '%s' is empty\n", spec);
            }
            
            return charset.size() != 0;
        }
        else {
            close(fd);
        }
    }
    
    charset.clear();
    size_t spec_len = strlen(spec);
    size_t conv_consumed = 0, conv_written = 0;
    UTF::decode_utf8(spec, spec_len, std::back_inserter(charset), &conv_consumed, &conv_written);
    if (conv_consumed != spec_len) {
        fprintf(stderr, "Error: the charset '%s' contains invalid UTF-8 chars\n", spec);
        return false;
    }
    
    if (charset.size() == 0) {
        fprintf(stderr, "Error: the charset '%s' is empty\n", spec);
    }
    
    return charset.size() != 0;;
}

}
