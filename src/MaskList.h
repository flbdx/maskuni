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

#include "Mask.h"

namespace Maskgen {

/**
 * @brief Hold a list of masks and iterate over this list
 * 
 * Use \a getFirstWord to get the first word from the MaskList then
 * use \a getNext to increment the mask and get the subsequent words
 * 
 * \a setPosition must be called before iterating over the masks
 * 
 * @param T Either char or 8-bit charsets or uint32_t for unicode codepoints
 */
template<typename T>
class MaskList
{
    std::vector<Mask<T>> m_masks;       /*!< List of masks */
    uint64_t m_len;                     /*!< sum of the length of the masks */
    typename decltype(m_masks)::iterator m_current_mask;    /*<! iterator to the current mask */
    uint64_t m_mask_rem;                /*!< number of words remaining in the current mask */
    size_t m_max_width;                 /*!< maximum of the width of the masks */
   
   /**
    * @brief A wrapper around __builtin_uaddl_overflow or __builtin_uaddll_overflow depending of the exact type of uint64_t
    * 
    * @param a first operand
    * @param b second operand
    * @param res a + b
    * @return true if overflow
    */
   static bool uadd64_overflow(uint64_t a, uint64_t b, uint64_t *res) {
        static_assert(std::is_same<uint64_t, unsigned long>::value || std::is_same<uint64_t, unsigned long long>::value, "Expecting either unsigned long or unsigned long long for uint64_t...");
        if (std::is_same<uint64_t, unsigned long>::value) {
            return __builtin_uaddl_overflow(a, b, (unsigned long *) res);
        }
        else if (std::is_same<uint64_t, unsigned long long >::value) {
            return __builtin_uaddll_overflow(a, b, (unsigned long long *) res);
        }
        *res = a + b;
        return false;
    }

public:
    MaskList() : m_masks(), m_len(0), m_current_mask(m_masks.begin()), m_mask_rem(0), m_max_width(0) {}

    /**
     * @brief Add a mask to the list
     * This method will abort if the length of the mask list would not fit in an unsigned 64 bit integer
     * 
     * @param mask mask
     */
    void pushMask(const Mask<T> &mask)
    {
        m_masks.emplace_back(mask);
        if (m_len == 0) {
            m_current_mask = m_masks.begin();
        }
        if (uadd64_overflow(m_len, m_masks.back().getLen(), &m_len)) {
            fprintf(stderr, "Error: the length of the masklist would overflow a 64 bits integer\n");
            abort();
        }
        m_max_width = std::max(m_max_width, m_masks.back().getWidth());
    }

    /**
     * @brief Get the length (number of words) of the MaskList
     * 
     * @return Length of the mask list
     */
    uint64_t getLen() const
    {
        return m_len;
    }

    /**
     * @brief Get the width of the current mask
     * 
     * @return width of the current mask
     */
    size_t getCurrentWidth() const
    {
        return (m_current_mask == nullptr) ? 0 : m_current_mask->getWidth();
    }

    /**
     * @brief Get the maximum width of the masks
     * 
     * @return Maximum width of the masks
     */
    size_t getMaxWidth() const
    {
        return m_max_width;
    }

    /**
     * @brief Set the position of the mask list (word number between 0 and getLen())
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
        
        // first initialize all the masks
        for (auto &mask : m_masks) {
            mask.setPosition(0);
        }
        
        // then get our starting position
        for (auto it = m_masks.begin(); it != m_masks.end(); it++) {
            uint64_t s = it->getLen();
            if (o < s) {
                m_current_mask = it;
                m_mask_rem = s - o;
                it->setPosition(o);
                break;
            }
            o -= s;
        }
    }
    
    /**
     * @brief Copy the first word of the mask list into \a w
     * Do not call \a getNext() for the first word.
     * 
     * @param w word buffer of at least getMaxWidth() characters
     * @param width store the width of this word
     * @return true if its the last word of the masklist
     */
    inline __attribute__((always_inline)) bool getFirstWord(T *w, size_t *width) {
        // our mask can't be empty if properly initialized, get current word
        m_current_mask->getCurrent(w); // first call for this mask, initialize the whole word
        *width = m_current_mask->getWidth();
        m_mask_rem--;
        return m_mask_rem == 0 && std::next(m_current_mask) == m_masks.end();
    }
    
    /**
     * @brief Increment the mask list and update \w with the next word
     * Only the changed characters of the \a w parameter are updated
     * therefore getNext whould always be called with the same parameter
     * and only after initializing the first word with \a getFirstWord.
     * 
     * @param w word buffer of at least getMaxWidth() characters
     * @param width store the width of this word
     * @return true if its the last word of the masklist
     */
    inline __attribute__((always_inline)) bool getNext(T *w, size_t *width) {
        if (m_mask_rem == 0) { // load next mask
            m_current_mask++;
            if (m_current_mask == m_masks.end()) {
                m_current_mask = m_masks.begin();
            }
            m_mask_rem = m_current_mask->getLen();
            m_current_mask->getCurrent(w); // first call for this mask, initialize the whole word
            *width = m_current_mask->getWidth();
            m_mask_rem--;
            return false;
        }
        else {
            m_current_mask->getNext(w); // update the word
            *width = m_current_mask->getWidth();
            m_mask_rem--;
            return m_mask_rem == 0 && std::next(m_current_mask) == m_masks.end();
        }
    }
};

}
