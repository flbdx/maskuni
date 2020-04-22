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

template<typename T>
class MaskList
{
    std::vector<Mask<T>> m_masks;
    uint64_t m_len;
    typename decltype(m_masks)::iterator m_current_mask;
    uint64_t m_mask_rem;
    size_t m_max_width;
    bool m_init_mask;

public:
    MaskList() : m_masks(), m_len(0), m_current_mask(m_masks.begin()), m_mask_rem(0), m_max_width(0), m_init_mask(true) {}

    void pushMask(const Mask<T> &mask)
    {
        m_masks.emplace_back(mask);
        if (m_len == 0) {
            m_current_mask = m_masks.begin();
        }
        if (__builtin_uaddl_overflow(m_len, m_masks.back().getLen(), &m_len)) {
            fprintf(stderr, "Error: the length of the masklist would overflow a 64 bits integer\n");
            abort();
        }
        m_max_width = std::max(m_max_width, m_masks.back().getWidth());
    }

    uint64_t getLen() const
    {
        return m_len;
    }

    size_t getCurrentWidth() const
    {
        return (m_current_mask == nullptr) ? 0 : m_current_mask->getWidth();
    }

    size_t getMaxWidth() const
    {
        return m_max_width;
    }

    void setPosition(uint64_t o)
    {
        if (m_len == 0) {
            return;
        }
        if (o >= m_len) {
            o = (o % m_len);
        }
        
        for (auto &mask : m_masks) {
            mask.setPosition(0);
        }
        
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
        m_init_mask = true;
    }
    
    inline __attribute__((always_inline)) bool getNext(T *w, size_t *width) {
        if (m_init_mask) { // first call to getNext
            // our mask can't be empty if properly initialized, get current word
            m_current_mask->getCurrent(w); // first call for this mask, initialize the whole word
            *width = m_current_mask->getWidth();
            m_init_mask = false;
            m_mask_rem--;
            return m_mask_rem == 0;
        }
        else {
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
                 return m_mask_rem == 0;
            }
        }
    }
};

}
