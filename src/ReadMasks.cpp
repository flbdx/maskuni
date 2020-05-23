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
bool readMask(const std::vector<T> &str, const CharsetMap<T> &defined_charsets, Mask<T> &mask) {
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
                    return false;
                }
            }
            i += 2;
        }
        else {
            mask.push_charset_right(&(str[i]), 1);
            i++;
        }
    }
    
    return true;
}

/* Read a line from a mask file
 * 
 * note that T('?') and T(',') are valid for unicode as the codepoint of ASCII character is their value
 */
template<typename T, T charsetEscapeChar = T('?'), T lineEscapeChar = ('\\'), T separatorChar = T(','), T commentChar = T('#')>
static bool readMaskLine_(const T *line, size_t line_len, const CharsetMap<T> &charsets, Mask<T> &mask) {
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
    
    mask.clear();
    readMask<T, charsetEscapeChar>(tokens.back(), effective_charsets, mask);
    if (mask.getWidth() == 0) {
        return false;
    }
    return true;
}

template<typename T, T charsetEscapeChar = T('?'), T lineEscapeChar = ('\\'), T separatorChar = T(','), T commentChar = T('#')>
static bool readMaskLine(const T *line, size_t line_len, const CharsetMap<T> &charsets, MaskList<T> &ml) {
    Mask<T> mask;
    if (readMaskLine_<T>(line, line_len, charsets, mask)) {
        ml.pushMask(mask);
        return true;
    }
    return false;
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
                    fclose(f);
                    return false;
                }
            }
            
            free(line);
            fclose(f);
            return true;
        }
        else {
            close(fd);
        }
    }
    
    std::vector<char> spec_v(spec, spec + strlen(spec));
    Mask<char> mask;
    readMask<char>(spec_v, charsets, mask);
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
                    fclose(f);
                    return false;
                }
                
                if (!readMaskLine<uint32_t>(conv_buf, conv_written, charsets, ml)) {
                    fprintf(stderr, "Error while reading '%s' at line %u\n", spec, line_number);
                    free(line);
                    free(conv_buf);
                    fclose(f);
                    return false;
                }
            }
            
            free(line);
            free(conv_buf);
            fclose(f);
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
        
        Mask<uint32_t> mask;
        readMask<uint32_t>(spec_v, charsets, mask);
        if (mask.getWidth() == 0) {
            return false;
        }
        ml.pushMask(mask);
    }
    
    return true;
}

/**
 * @brief Mask generator for a single mask or a mask file
 * 
 * The generator holds the whole file content to avoid strange behavior if the file is modified while opened
 * It can be used with a single mask as a command line argument by passing command_line_mask=true
 * 
 */
template<typename T>
class MaskFileGenerator : public MaskGenerator<T>
{
    char *m_content;            /*!< file content */
    const size_t m_content_len; /*!< file content length */
    bool m_command_line_mask;   /*!< true if content is a command line argument and not the content of a file */
    char *m_filename;           /*!< name of the file for error messages */
    const CharsetMap<T> m_charsets; /*<! predefined charsets */
    const char *m_p;            /*!< read pointer in m_content */
    unsigned int m_line_number; /*!< number of line read for error messages */
    bool m_error;               /*!< error flag */
    
    
    /**
     * @brief read a line from the buffer \a m_content
     * 
     * The data are not copied and must not be modified
     * 
     * @param line set to the begining of the line
     * @param line_len set to the length of the line
     * @return true if there was something to read
     */
    bool readline(const char **line, size_t *line_len) {
        size_t rem = m_content_len - (m_p - m_content);
        if (rem == 0) {
            return false;
        }
        
        char *delim_pos = (char *) memchr(m_p, '\n', rem);
        if (delim_pos) {
            *line = m_p;
            *line_len = delim_pos - m_p + 1;
            m_p += *line_len;
        }
        else {
            *line = m_p;
            *line_len = rem;
            m_p += rem;
        }
        return true;
    }
    
public:
    /**
     * @brief construct a new generator
     * 
     * @param content file content or inline mask. The generator takes ownership of content which will be freed with \a free by the destructor
     * @param content_len length of \a content
     * @param command_line_mask set to true if it's a command line argument and not a mask file
     * @param filename filename for error messages
     * @param charsets predefined charsets
     */
    MaskFileGenerator(char *content, size_t content_len, bool command_line_mask, const char *filename, const CharsetMap<T> &charsets) :
    m_content(content), m_content_len(content_len), m_command_line_mask(command_line_mask),
    m_filename(strdup(filename)), m_charsets(charsets), m_p(m_content), m_line_number(0), m_error(false) {}
    
    ~MaskFileGenerator() {
        free(m_content);
        free(m_filename);
    }
    
    bool operator()(Maskgen::Mask<T> &mask);
    
    void reset() {
        m_p = m_content;
        m_line_number = 0;
        m_error = false;
    }
    
    bool good() {
        return !m_error;
    }
    
};

template<> bool MaskFileGenerator<char>::operator()(Maskgen::Mask<char> &mask) {
    const char *line;
    size_t r;
    while (true) {
        if (!readline(&line, &r)) {
            return false;
        }
        m_line_number++;
        
        if (r >= 2 && line[r - 1] == '\n' && line[r - 2] == '\r') {
            r -= 2;
        }
        else if (r >= 1 && line[r - 1] == '\n') {
            r -= 1;
        }
        if (r == 0) {
            continue;
        }
        
        mask.clear();
        if (m_command_line_mask) {
            readMask<char>({m_content, m_content + m_content_len}, m_charsets, mask);
            if (mask.getWidth() == 0) {
                m_error = true;
                return false;
            }
            else {
                return true;
            }
        }
        else {
            // full parser when reading from a file
            if (readMaskLine_<char>(line, r, m_charsets, mask)) {
                return true;
            }
            m_error = true;
            fprintf(stderr, "Error while reading '%s' at line %u\n", m_filename, m_line_number);
            return false;
        }
    }
    
    return false;
}

// compared to the char version, this adds a UTF-8 decoding
template<> bool MaskFileGenerator<uint32_t>::operator()(Maskgen::Mask<uint32_t> &mask) {
    const char *line;
    size_t r;
    uint32_t *conv_buf = NULL;
    size_t conv_buf_size = 0;
    size_t conv_consumed = 0, conv_written = 0;
    while (true) {
        if (!readline(&line, &r)) {
            free(conv_buf);
            return false;
        }
        m_line_number++;
        
        if (r >= 2 && line[r - 1] == '\n' && line[r - 2] == '\r') {
            r -= 2;
        }
        else if (r >= 1 && line[r - 1] == '\n') {
            r -= 1;
        }
        if (r == 0) {
            continue;
        }
        
        UTF::decode_utf8(line, r, &conv_buf, &conv_buf_size, &conv_consumed, &conv_written);
        if (conv_consumed != (size_t) r) {
            if (m_command_line_mask) {
                fprintf(stderr, "Error: the mask argument '%s' contains invalid UTF-8 chars\n", m_filename);
            }
            else {
                fprintf(stderr, "Error: the mask file '%s' contains invalid UTF-8 chars at line %u\n", m_filename, m_line_number);
            }
            free(conv_buf);
            m_error = true;
            return false;
        }
        
        mask.clear();
        if (m_command_line_mask) {
            readMask<uint32_t>({conv_buf, conv_buf + conv_written}, m_charsets, mask);
            if (mask.getWidth() == 0) {
                m_error = true;
                return false;
            }
            else {
                return true;
            }
        }
        else {
            // full parser when reading from a file
            if (readMaskLine_<uint32_t>(conv_buf, conv_written, m_charsets, mask)) {
                free(conv_buf);
                return true;
            }
            m_error = true;
            fprintf(stderr, "Error while reading '%s' at line %u\n", m_filename, m_line_number);
            free(conv_buf);
            return false;
        }
    }
    
    return false;
}

template<typename T>
MaskGenerator<T> *readMaskList__(const char *spec, const CharsetMap<T> &charsets) {
#if defined(__WINDOWS__) || defined(__CYGWIN__)
    int fd = open(spec, O_RDONLY | O_BINARY);
#else
    int fd = open(spec, O_RDONLY);
#endif
    
    char *content = NULL;
    size_t content_len = 0;
    
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
            content_len = st.st_size;
            content = (char *) malloc(content_len);
            ssize_t r = read(fd, content, content_len);
            if (r < 0 || (size_t) r != content_len) {
                fprintf(stderr, "Error while reading '%s'\n", spec);
                free(content);
                close(fd);
                return NULL;
            }
            
            close(fd);
            return new MaskFileGenerator<T>(content, content_len, false, spec, charsets);
        }
        else {
            close(fd);
        }
    }
    
    content_len = strlen(spec);
    content = (char *) malloc(content_len);
    memcpy(content, spec, content_len);
    
    return new MaskFileGenerator<T>(content, content_len, true, spec, charsets);
}

MaskGenerator<char> *readMaskListAscii__(const char *spec, const CharsetMapAscii &charsets) {
    return readMaskList__<char>(spec, charsets);
}

MaskGenerator<uint32_t> *readMaskListUtf8__(const char *spec, const CharsetMapUnicode &charsets) {
    return readMaskList__<uint32_t>(spec, charsets);
}

}
