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

#include <cstdio>

#include "Charset.h"
#include <vector>

namespace Maskgen {

template<typename T>
class Mask
{
    std::vector<Charset<T>> m_charsets;
    size_t m_n_charsets;
    uint64_t m_len;

public:
    Mask() : m_charsets(), m_n_charsets(0), m_len(0) {}

    void push_charset_right(const T *set, uint64_t set_len)
    {
        m_charsets.emplace_back(set, set_len);
        if (m_n_charsets == 0) {
            m_len = m_charsets.back().getLen();
        } else {
            if (__builtin_umull_overflow(m_len, m_charsets.back().getLen(), &m_len)) {
                fprintf(stderr, "Error: the length of the mask would overflow a 64 bits integer\n");
                abort();
            }
        }
        m_n_charsets++;
    }

    void push_charset_right(const Charset<T> &charset)
    {
        m_charsets.emplace_back(charset);
        if (m_n_charsets == 0) {
            m_len = m_charsets.back().getLen();
        } else {
            if (__builtin_umull_overflow(m_len, m_charsets.back().getLen(), &m_len)) {
                fprintf(stderr, "Error: the length of the mask would overflow a 64 bits integer\n");
                abort();
            }
        }
        m_n_charsets++;
    }

    void push_charset_left(const T *set, uint64_t set_len)
    {
        m_charsets.emplace(m_charsets.begin(), set, set_len);
        if (m_n_charsets == 0) {
            m_len = m_charsets.front().getLen();
        } else {
            if (__builtin_umull_overflow(m_len, m_charsets.front().getLen(), &m_len)) {
                fprintf(stderr, "Error: the length of the mask would overflow a 64 bits integer\n");
                abort();
            }
        }
        m_n_charsets++;
    }

    void push_charset_left(const Charset<T> &charset)
    {
        m_charsets.emplace(m_charsets.begin(), charset);
        if (m_n_charsets == 0) {
            m_len = m_charsets.front().getLen();
        } else {
            if (__builtin_umull_overflow(m_len, m_charsets.front().getLen(), &m_len)) {
                fprintf(stderr, "Error: the length of the mask would overflow a 64 bits integer\n");
                abort();
            }
        }
        m_n_charsets++;
    }

    uint64_t getLen() const
    {
        return m_len;
    }

    size_t getWidth() const
    {
        return m_charsets.size();
    }

    void setPosition(uint64_t o)
    {
        if (m_len == 0) {
            return;
        }
        if (o >= m_len) {
            o = (o % m_len);
        }

        for (auto it = m_charsets.rbegin(); it != m_charsets.rend(); it++) {
            uint64_t s = (*it).getLen();
            uint64_t q = o / s;
            uint64_t r = o - q * s;
            (*it).setPosition(r);
            o = q;
        }
    }

    inline __attribute__((always_inline)) void getCurrent(T *w)
    {
        for (size_t i = 0; i < m_n_charsets; i++) {
             m_charsets[i].getCurrent(w + i);
        }
    }
    
    inline __attribute__((always_inline)) bool getNext(T *w)
    {
        bool carry = true;
        for (size_t i = m_n_charsets; carry && i != 0; i--) {
            carry = m_charsets[i - 1].getNext(w + (i - 1));
        }
        return carry;
    }
};

}
