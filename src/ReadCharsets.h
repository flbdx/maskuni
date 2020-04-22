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

#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <map>

namespace Maskgen {

template<typename T>
struct DefaultCharset {
    std::vector<T> cset;
    bool final;
    
    DefaultCharset() : cset(), final(true) {}
    
    DefaultCharset(const std::vector<T> &cset_, bool final_) : cset(cset_), final(final_) {}
    
    DefaultCharset(const T *s, size_t l, bool final_) : cset(), final(final_) {
        cset.resize(l);
        std::copy_n(s, l, cset.begin());
    }
};

template<typename T> using CharsetMap = std::map<T, DefaultCharset<T>>;
typedef CharsetMap<char> CharsetMapAscii;
typedef CharsetMap<uint32_t> CharsetMapUnicode;

std::vector<char> expandCharsetAscii(const std::vector<char> &charset, const CharsetMapAscii &default_charsets, char charset_name);
std::vector<uint32_t> expandCharsetUnicode(const std::vector<uint32_t> &charset, const CharsetMapUnicode &default_charsets, uint32_t charset_name);

void initDefaultCharsetsAscii(CharsetMapAscii &charsets);
void initDefaultCharsetsUnicode(CharsetMapUnicode &charsets);

bool readCharsetAscii(const char *spec, std::vector<char> &charset);
bool readCharsetUtf8(const char *spec, std::vector<uint32_t> &charset);

}
