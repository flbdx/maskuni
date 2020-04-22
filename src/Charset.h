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
#include <cstdlib>
#include <cassert>
#include <algorithm>

namespace Maskgen {

/**
 * @brief Hold a charset and iterate over its content
 * @param T Either char or 8-bit charsets or uint32_t for unicode codepoints
 */
template<typename T>
class Charset
{
    T *m_set;           /*!< characters */
    uint64_t m_len;     /*!< length of \a m_set */
    T *m_set_end;       /*!< m_set + m_len */
    T *m_p;             /*!< current position in the charset */

public:
    
    /**
     * @brief Construct a new charset
     * 
     * @param set characters
     * @param set_len number of characters in \a set
     */
    Charset(const T *set, uint64_t set_len) :
        m_set(nullptr)
        , m_len(set_len)
        , m_set_end(nullptr)
        , m_p(nullptr)
    {
        if (set_len == 0) {
            fprintf(stderr, "Error: trying to define an empty charset\n");
            abort();
        }
        m_set = (T *) malloc(sizeof(T) * m_len);
        m_set_end = m_set + m_len;
        m_p = m_set;
        std::copy_n(set, set_len, m_set);
    }

    ~Charset()
    {
        free(m_set);
    }

    Charset(const Charset &o) :
        m_set(nullptr)
        , m_len(o.m_len)
        , m_set_end(nullptr)
        , m_p(nullptr)
    {
        m_set = (T *) malloc(sizeof(T) * m_len);
        m_set_end = m_set + m_len;
        m_p = m_set + (o.m_p - o.m_set);
        std::copy_n(o.m_set, m_len, m_set);
    }

    Charset<T> &operator = (const Charset &o)
    {
        if (m_set) {
            free(m_set);
        }
        m_set = (T *) malloc(sizeof(T) * m_len);
        m_set_end = m_set + m_len;
        m_p = m_set + (o.m_p - o.m_set);
        std::copy_n(o.m_set, m_len, m_set);
        return *this;
    }

    /**
     * @brief Get the number of characters in the charset
     * 
     * @return Length of the charset
     */
    inline uint64_t getLen() const
    {
        return m_len;
    }

    /**
     * @brief Set the position of the next char to read from the charset
     * If the position is greater than the charset's length, a modulo is applied
     * 
     * @param o position
     */
    void setPosition(uint64_t o)
    {
        if (o >= m_len) {
            o = (o % m_len);
        }
        m_p = m_set + o;
    }

    /**
     * @brief Copy the character at the current position
     * 
     * @param out Store the current character
     */
    inline __attribute__((always_inline)) void getCurrent(T *out)
    {
        *out = *m_p;
    }
    
    /**
     * @brief Increment the charset then copy the character at the resulting position
     * 
     * @param out Store the next character
     */
    inline __attribute__((always_inline)) bool getNext(T *out)
    {
        m_p += 1;
        m_p = (m_p == m_set_end) ? m_set : m_p;
        *out = *m_p;
        return m_p == m_set;
    }
};

}
