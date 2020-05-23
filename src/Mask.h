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

/**
 * @brief Hold a mask and iterate over its content
 * A mask is a list of charsets.
 * 
 * \a setPosition must be called before iterating over the mask
 * \a getCurrent should be use to get the first word of the mask.
 * \a getNext should be called with the same parameter to get the subsequent words.
 * 
 * @param T Either char or 8-bit charsets or uint32_t for unicode codepoints
 */
template<typename T>
class Mask
{
    std::vector<Charset<T>> m_charsets; /*!< list of charsets from left to right */
    size_t m_n_charsets;                /*!< m_charsets.size() */
    uint64_t m_len;                     /*!< sum of the charsets' length */
    
    /**
    * @brief A wrapper around __builtin_umull_overflow or __builtin_umulll_overflow depending of the exact type of uint64_t
    * 
    * @param a first operand
    * @param b second operand
    * @param res a * b
    * @return true if overflow
    */
    static bool umul64_overflow(uint64_t a, uint64_t b, uint64_t *res) {
        static_assert(std::is_same<uint64_t, unsigned long>::value || std::is_same<uint64_t, unsigned long long>::value, "Expecting either unsigned long or unsigned long long for uint64_t...");
        if (std::is_same<uint64_t, unsigned long>::value) {
            return __builtin_umull_overflow(a, b, (unsigned long *) res);
        }
        else if (std::is_same<uint64_t, unsigned long long >::value) {
            return __builtin_umulll_overflow(a, b, (unsigned long long *) res);
        }
        *res = a * b;
        return false;
    }

public:
    /**
     * @brief Create a new empty mask
     *
     * @param reserve Reserve memory for \a reserve charsets for faster insertions
     */
    Mask(unsigned int reserve = 0) : m_charsets(), m_n_charsets(0), m_len(0)
    {
        m_charsets.reserve(reserve);
    }
    
    /**
     * @brief erase all the content of the mask
     */
    void clear()
    {
        m_charsets.clear();
        m_len = 0;
        m_n_charsets = 0;
    }

    /**
     * @brief Add a charset to the right of the already defined charsets
     * This method will abort if the length of the mask would not fit in an unsigned 64 bit integer
     * 
     * @param set characters
     * @param set_len number of characters
     */
    void push_charset_right(const T *set, uint64_t set_len)
    {
        m_charsets.emplace_back(set, set_len);
        if (m_n_charsets == 0) {
            m_len = m_charsets.back().getLen();
        } else {
            if (umul64_overflow(m_len, m_charsets.back().getLen(), &m_len)) {
                fprintf(stderr, "Error: the length of the mask would overflow a 64 bits integer\n");
                abort();
            }
        }
        m_n_charsets++;
    }

    /**
     * @brief Add a charset to the right of the already defined charsets
     * This method will abort if the length of the mask would not fit in an unsigned 64 bit integer
     * 
     * @param charset charset
     */
    void push_charset_right(const Charset<T> &charset)
    {
        m_charsets.emplace_back(charset);
        if (m_n_charsets == 0) {
            m_len = m_charsets.back().getLen();
        } else {
            if (umul64_overflow(m_len, m_charsets.back().getLen(), &m_len)) {
                fprintf(stderr, "Error: the length of the mask would overflow a 64 bits integer\n");
                abort();
            }
        }
        m_n_charsets++;
    }

    /**
     * @brief Add a charset to the left of the already defined charsets
     * This method will abort if the length of the mask would not fit in an unsigned 64 bit integer
     * 
     * @param set characters
     * @param set_len number of characters
     */
    void push_charset_left(const T *set, uint64_t set_len)
    {
        m_charsets.emplace(m_charsets.begin(), set, set_len);
        if (m_n_charsets == 0) {
            m_len = m_charsets.front().getLen();
        } else {
            if (umul64_overflow(m_len, m_charsets.front().getLen(), &m_len)) {
                fprintf(stderr, "Error: the length of the mask would overflow a 64 bits integer\n");
                abort();
            }
        }
        m_n_charsets++;
    }

    /**
     * @brief Add a charset to the left of the already defined charsets
     * This method will abort if the length of the mask would not fit in an unsigned 64 bit integer
     * 
     * @param charset charset
     */
    void push_charset_left(const Charset<T> &charset)
    {
        m_charsets.emplace(m_charsets.begin(), charset);
        if (m_n_charsets == 0) {
            m_len = m_charsets.front().getLen();
        } else {
            if (umul64_overflow(m_len, m_charsets.front().getLen(), &m_len)) {
                fprintf(stderr, "Error: the length of the mask would overflow a 64 bits integer\n");
                abort();
            }
        }
        m_n_charsets++;
    }

    /**
     * @brief Get the length of this mask (number of words)
     * 
     * @return Length of the mask
     */
    inline __attribute__((always_inline)) uint64_t getLen() const
    {
        return m_len;
    }

    /**
     * @brief Get the width of this mask (number of characters)
     * 
     * @return Width of the mask
     */
    inline __attribute__((always_inline)) size_t getWidth() const
    {
        return m_charsets.size();
    }

    /**
     * @brief Set the current position in the mask (between 0 and \a getLen())
     * Must be called before using \a getCurrent and \a getNext
     * 
     * @param o Position
     */
    void setPosition(uint64_t o)
    {
        if (m_len == 0) {
            return;
        }
        if (o >= m_len) {
            o = (o % m_len);
        }

        // set the position from right to left
        for (auto it = m_charsets.rbegin(); it != m_charsets.rend(); it++) {
            uint64_t s = (*it).getLen();
            uint64_t q = o / s;
            uint64_t r = o - q * s;
            (*it).setPosition(r);
            o = q;
        }
    }

    /**
     * @brief Copy the current word into w without incrementing the mask
     * This method must be called to fully initialize a word.
     * 
     * @param w buffer of at least getWidth() elements
     */
    inline __attribute__((always_inline)) void getCurrent(T *w)
    {
        for (size_t i = 0; i < m_n_charsets; i++) {
             m_charsets[i].getCurrent(w + i);
        }
    }
    
    /**
     * @brief Increment the mask and update a buffer with the next word
     * Only the changed characters of the \a w parameter are updated
     * therefore getNext whould always be called with the same parameter
     * and only after initializing the first word with \a getCurrent.
     * 
     * The word is iterated from right to left.
     * 
     * @param w buffer of at least getWidth() elements
     * @return true if the mask is back to position 0 ("carry")
     */
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
