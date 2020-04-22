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

#include "ReadCharsets.h"

#include <list>

namespace Maskgen {

template<typename T, T escapeChar = T('?')>
static std::vector<T> expandCharset(const std::vector<T> &charset, const CharsetMap<T> &default_charsets, T charset_name)
{
    // working with a list allows fast random insert and erase without invalidating iterators (except iterators on erased elements...)
    std::list<T> lcharset(charset.begin(), charset.end());
    typedef typename decltype(lcharset)::iterator literator;
    
    // store a range of chars to read, with starting iterator and length
    // we can't use an iterator for the end of the range since it may be invalidated
    // the starting iterator is never invalidated during the loop
    std::list<std::pair<literator, size_t>> queue;
    std::list<T> keys_history;
    queue.emplace_back(lcharset.begin(), lcharset.size());
    keys_history.push_back(charset_name);
    
    while (!queue.empty()) {
        auto boundaries = queue.back();
        queue.pop_back();
        
        size_t n_chars = 0;
        for (auto it = boundaries.first; n_chars != boundaries.second; ) {
            T c = *it;
            if (c == escapeChar) {
                if (n_chars + 1 == boundaries.second) {
                    it++;
                    n_chars++;
                } else {
                    T key = *std::next(it);
                    if (key == escapeChar) {
                        it = lcharset.erase(it);
                        it ++;
                        
                    } else {
                        auto repl_it = default_charsets.find(key);
                        if (repl_it != default_charsets.end()) {
                            it = lcharset.erase(it);
                            it = lcharset.erase(it);
                            if (std::count(keys_history.begin(), keys_history.end(), key) == 0) {
                                it = lcharset.insert(it, repl_it->second.cset.begin(), repl_it->second.cset.end());
                                if (!repl_it->second.final) {
                                    queue.emplace_back(it, repl_it->second.cset.size());
                                    keys_history.push_back(key);
                                }
                                it = std::next(it, repl_it->second.cset.size());
                            }
                        } else {
                            // no charset found, remove ?
                            it = lcharset.erase(it);
                            it = std::next(it);
                        }
                    }
                    n_chars += 2;
                }
            } else {
                it++;
                n_chars++;
            }
        }
        
        keys_history.pop_back();
    }
    
    // remove duplicates
    for (auto it = lcharset.begin(); it != lcharset.end(); it++) {
        auto it_dupl = std::find(std::next(it), lcharset.end(), *it);
        while (it_dupl != lcharset.end()) {
            it_dupl = lcharset.erase(it_dupl);
            it_dupl = std::find(it_dupl, lcharset.end(), *it);
        }
    }
    
    return std::vector<T>(lcharset.begin(), lcharset.end());
}

}
