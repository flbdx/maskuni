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
#include <type_traits>

namespace Maskuni {

/**
* @brief A wrapper around __builtin_umull_overflow or __builtin_umulll_overflow depending of the exact type of uint64_t
* 
* @param a first operand
* @param b second operand
* @param res a * b
* @return true if overflow
*/
static inline bool umul64_overflow(uint64_t a, uint64_t b, uint64_t *res) {
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

/**
* @brief A wrapper around __builtin_uaddl_overflow or __builtin_uaddll_overflow depending of the exact type of uint64_t
* 
* @param a first operand
* @param b second operand
* @param res a + b
* @return true if overflow
*/
static inline bool uadd64_overflow(uint64_t a, uint64_t b, uint64_t *res) {
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

}
