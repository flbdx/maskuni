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

#include "Mask.h"
#include <memory>

namespace Maskuni {

/**
 * @brief An abstract class for a Mask<T> generator
 * 
 * @param T Either char or 8-bit charsets or uint32_t for unicode codepoints
 */
template<typename T>
class MaskGenerator
{
public:
    // keep this commented out for now
    // it's nice but would be better with C++17 as ranged-based for loop
    // require the same type for the begin() and end() values for <= C++14
//     struct iterator_sentinel {};
//     class iterator;
    
    virtual ~MaskGenerator<T>() {}
    
    /**
     * @brief generation method
     * 
     * @param mask the new mask is copied into \a mask
     * @return true if a mask was generated, false when there is no more mask or if there was an error
     */
    virtual bool operator()(Maskuni::Mask<T> &mask) = 0;
    
    /**
     * @brief generation method, get only the next mask's size and width
     * 
     * Should be overwritten to provide a faster version
     * 
     * @param size the new mask's size is copied into \a size
     * @param width the new mask's width is copied into \a width
     * @return true if a mask was generated, false when there is no more mask or if there was an error
     */
    virtual bool operator()(uint64_t &size, size_t &width) {
        Maskuni::Mask<T> tmpmask;
        if ((*this)(tmpmask)) {
            size = tmpmask.getLen();
            width = tmpmask.getWidth();
            return true;
        }
        return false;
    }
    
    /**
     * @brief Reset the generator
     * 
     */
    virtual void reset() = 0;
    
    /**
     * @brief Test if there was an error
     * 
     * if (o.good() && !o(mask)) then the generator is terminated without error
     * 
     * @return false if there was an error
     */
    virtual bool good() = 0;
    
//     virtual iterator begin() {
//         return {*this};
//     }
//     virtual iterator_sentinel end() {
//         return {};
//     }
//     
//     class iterator {
//     public:
//         typedef Maskuni::Mask<T> value_type;
//         
//         iterator(MaskGenerator<T> &gen) : m_gen(gen), m_current_value(), m_done(false) {
//             m_done = !m_gen(m_current_value);
//         }
//         
//         iterator& operator++() {
//             m_done = !m_gen(m_current_value);
//             return *this;
//         }
//         value_type & operator*() {
//             return m_current_value;
//         }
//         value_type* operator->() {
//             return &m_current_value;
//         }
//         bool operator==(iterator_sentinel) const {
//             return m_done;
//         }
//         bool operator!=(iterator_sentinel s) const {
//             return !operator==(s);
//         }
//         
//     private:
//         MaskGenerator<T> &m_gen;
//         value_type m_current_value;
//         bool m_done;
//     };
};

}
