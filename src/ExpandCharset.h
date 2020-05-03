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
bool expandCharset(CharsetMap<T> &charsets, T charset_name)
{
    // get the charsets matching charset_name
    auto charsets_range = charsets.equal_range(charset_name);
    if (charsets_range.first == charsets.end()) {
        // none found!
        return false;
    }
    
    // last charset matching charset_name
    typename CharsetMap<T>::mapped_type &charset = std::prev(charsets_range.second)->second;
    
    if (charset.final) {
        return true;
    }
    
    // working with a list allows fast random insert and erase without invalidating iterators (except iterators on erased elements...)
    std::list<T> lcharset(charset.cset.begin(), charset.cset.end());
    typedef typename decltype(lcharset)::iterator literator;
    
    // store a range of chars to read, with starting iterator and length
    // we can't use an iterator for the end of the range since it may be invalidated
    // the starting iterator is never invalidated during the loop
    std::list<std::pair<literator, size_t>> queue;
    // store the keys that where previously seen before expanding the ranges defined in 'queue'
    std::list<std::list<T> > keys_histories;
    queue.emplace_back(lcharset.begin(), lcharset.size());
    keys_histories.push_back({charset_name});
    
    while (!queue.empty()) {
        auto boundaries = queue.back();
        auto keys_history = keys_histories.back();
        queue.pop_back();
        keys_histories.pop_back();
        
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
                        int n_repl_avail = charsets.count(key); // how many definitions of the charset are available
                        if (n_repl_avail != 0) {
                            it = lcharset.erase(it);
                            it = lcharset.erase(it);
                            // the number of times we already expanded this charset name
                            int n_replaced = std::count(keys_history.begin(), keys_history.end(), key);
                            // if we still have more previous definition to use
                            if (n_replaced < n_repl_avail) {
                                auto it_repl = charsets.upper_bound(key);// upper_bound is past the last definition
                                std::advance(it_repl, -(1 + n_replaced));
                                it = lcharset.insert(it, it_repl->second.cset.begin(), it_repl->second.cset.end());
                                if (!it_repl->second.final) {
                                    keys_histories.emplace_back(keys_history);
                                    keys_histories.back().push_back(key);
                                    queue.emplace_back(it, it_repl->second.cset.size());
                                }
                                it = std::next(it, it_repl->second.cset.size());
                            }
                            else {
                                // can't recurse anymore, make it fatal
                                return false;
                            }
                        }
                        else {
                            // no charset found, fatal.
                            return false;
                        }
                    }
                    n_chars += 2;
                }
            } else {
                it++;
                n_chars++;
            }
        }
    }
    
    // remove duplicates
    for (auto it = lcharset.begin(); it != lcharset.end(); it++) {
        auto it_dupl = std::find(std::next(it), lcharset.end(), *it);
        while (it_dupl != lcharset.end()) {
            it_dupl = lcharset.erase(it_dupl);
            it_dupl = std::find(it_dupl, lcharset.end(), *it);
        }
    }
    
    charset.cset = std::vector<T>(lcharset.begin(), lcharset.end());
    charset.final = true;
    return true;
}

}
