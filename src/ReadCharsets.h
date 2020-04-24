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

/**
 * @brief Describe a builtin charset or a user defined charset
 * @param T Either char or 8-bit charsets or uint32_t for unicode codepoints
 * 
 */
template<typename T>
struct DefaultCharset {
    std::vector<T> cset;    /*!< charset */
    bool final;             /*!< true if the charset should not be further expanded */
    
    DefaultCharset() : cset(), final(true) {}
    
    DefaultCharset(const std::vector<T> &cset_, bool final_) : cset(cset_), final(final_) {}
    
    DefaultCharset(const T *s, size_t l, bool final_) : cset(), final(final_) {
        cset.resize(l);
        std::copy_n(s, l, cset.begin());
    }
};

/**
 * @brief A map charset name -> charset
 */
template<typename T> using CharsetMap = std::map<T, DefaultCharset<T>>;
typedef CharsetMap<char> CharsetMapAscii;
typedef CharsetMap<uint32_t> CharsetMapUnicode;

/**
 * @brief Expand an 8-bit charset replacing all the charset references (?X) by their values, then unify the charset
 * This function is protected against recursive charsets definition. The \a charset_name
 * parameter is used for this protection
 * 
 * @param charset Charset to expand
 * @param default_charsets Currently defined charsets
 * @param charset_name This charset name
 * @return Expanded and "uniquified"" charset or empty vector if the charset is invalid
 */
std::vector<char> expandCharsetAscii(const std::vector<char> &charset, const CharsetMapAscii &default_charsets, char charset_name);

/**
 * @brief Expand an unicode codepoint charset replacing all the charset references (?X) by their values, then unify the charset
 * This function is protected against recursive charsets definition. The \a charset_name
 * parameter is used for this protection
 * 
 * @param charset Charset to expand
 * @param default_charsets Currently defined charsets
 * @param charset_name This charset name
 * @return Expanded and "uniquified"" charset or empty vector if the charset is invalid
 */
std::vector<uint32_t> expandCharsetUnicode(const std::vector<uint32_t> &charset, const CharsetMapUnicode &default_charsets, uint32_t charset_name);

/**
 * @brief Init a charset map with the 8-bits built-in charsets
 * 
 * @param charsets charset map
 */
void initDefaultCharsetsAscii(CharsetMapAscii &charsets);

/**
 * @brief Init a charset map with the unicode built-in charsets
 * 
 * @param charsets charset map
 */
void initDefaultCharsetsUnicode(CharsetMapUnicode &charsets);

/**
 * @brief Create an 8-bits charset from a file or from the string \a spec
 * If a file named \a spec exists then the charset is initialized with its content
 * Otherwise the content of the charset is the string \a spec
 * 
 * @param spec charset file name or charset content
 * @param charset output charset
 * @return false if an error occured
 */
bool readCharsetAscii(const char *spec, std::vector<char> &charset);

/**
 * @brief Create an unicode charset from a file or from the string \a spec
 * If a file named \a spec exists then the charset is initialized with its content
 * Otherwise the content of the charset is the string \a spec
 * 
 * The content of the file or the string \a spec must be UTF-8 encoded
 * 
 * @param spec charset file name or charset content
 * @param charset output charset
 * @return false if an error occured
 */
bool readCharsetUtf8(const char *spec, std::vector<uint32_t> &charset);

}
