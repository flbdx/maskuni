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

#include "ReadBruteforce.h"
#include "ExpandCharset.h"
#include "utf_conv.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <queue>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef HAVE_GETLINE
# include "getline.h"
#endif

namespace Maskgen {

// describe the constraints on a charset
template<typename T>
struct ConstrainedCharset {
    DefaultCharset<T> m_charset; // a charset
    unsigned int m_min; // minimum number of occurence
    unsigned int m_max; // maximum number of occurence

    ConstrainedCharset() :
        m_charset(), m_min(0), m_max(0) {}
    ConstrainedCharset(DefaultCharset<T> &charset, unsigned int min, unsigned int max) :
        m_charset(charset), m_min(min), m_max(max) {}
};

/*
 * 2nd stage of the mask generation, enumerate over the masks from a valid constraints set
 */
template<typename T>
static void enumerateMasks(
        const std::vector<std::pair<const ConstrainedCharset<T> *, unsigned int>> &constraints,
        unsigned int target_len, MaskList<T> &ml)
{
    // all of this could be written more simply using generators and recursions
    // here we use a queue to maintain a state
    // the items in the queue are:
    //  - vector storing how many times a charset can be used
    //  - the progression of the mask from left to right
    typedef std::pair<std::vector<unsigned int>, std::list<const ConstrainedCharset<T> *>> queue_item;
    std::queue<queue_item> queue;

    {
        std::vector<unsigned int> init_values(constraints.size());
        for (size_t i = 0; i < constraints.size(); i++) {
            init_values[i] = constraints[i].second;
        }
        queue.push(queue_item(init_values, typename queue_item::second_type()));
    }

    while (!queue.empty()) {
        queue_item state = queue.front();
        queue.pop();

        for (size_t i = 0; i < state.first.size(); i++) {
            if (state.first[i] > 0) {
                queue_item nstate = state;
                nstate.first[i]--;
                nstate.second.push_back(constraints[i].first);
                queue.push(nstate);
            }
        }
        if (state.second.size() == target_len) {
            // this mask is done cooking
            Mask<T> mask;
            for (auto cset : state.second) {
                mask.push_charset_right(cset->m_charset.cset.data(), cset->m_charset.cset.size());
            }
            ml.pushMask(mask);
        }
    }
}

/*
 * The first stage of the mask generation is to deduce some valid reduced constraints.
 * For example, if we have:
 *   ?d: 4 to 6
 *   ?l: 0 to 1
 *   length: 6
 *
 * the valid constraints are:
 *   ?d*5, ?l*1
 *   ?d*6, ?l*0
 *
 * For each of those valid constraints, call enumerateMasks to get the associated masks
 */
template<typename T>
static void maskListFromConstraints(
        const std::vector<ConstrainedCharset<T>> &constraints,
        unsigned int target_len, MaskList<T> &ml)
{
    // for each charset, how many
    // initialize this with the minimum number for each charsets
    std::vector<std::pair<const ConstrainedCharset<T> *, unsigned int>> counts(constraints.size());
    unsigned int current_len = 0; // our current word length
    for (size_t i = 0; i < constraints.size(); i++) {
        counts[i].first = &constraints[i];
        counts[i].second = constraints[i].m_min;
        current_len += constraints[i].m_min;
    }

    while (true) {
        if (current_len < target_len) {
            // skip a few invalid combinations
            auto it = counts.begin();
            unsigned int diff = std::min(target_len - current_len, it->first->m_max - it->second);
            it->second += diff;
            current_len += diff;
        }

        if (current_len == target_len) {
            // this one is valid!
            enumerateMasks(counts, target_len, ml);
        }

        bool carry = true;
        for (auto it = counts.begin(); it != counts.end() && carry; it++) {
            // now increment
            it->second++;
            current_len++;
            if (it->second > it->first->m_max || current_len > target_len) {
                // and also skip some other invalid combinations
                current_len -= it->second;
                it->second = it->first->m_min;
                current_len += it->second;
                carry = true;
            }
            else {
                carry = false;
            }
        }
        if (carry) {
            break;
        }
    }
}

struct Helper8bits
{
    static bool readCharset(const char *input, size_t, std::vector<char> &cset)
    {
        while (*input != 0) {
            cset.push_back(*input);
            input++;
        }
        return true;
    }
};

struct HelperUnicode
{
    static bool readCharset(const char *input, size_t input_len, std::vector<uint32_t> &cset)
    {
        size_t conv_consumed = 0, conv_written = 0;
        if (UTF::decode_utf8(input, input_len, std::back_inserter(cset), &conv_consumed, &conv_written) != UTF::RetCode::OK) {
            return false;
        }
        return true;
    }
};

/*
 * The format of the input file is :
 * <word width>
 * <min1> <max1> <charset1>
 * <min2> <max2> <charset2>
 *
 * no comment, no escape char, empty lines are allowed, no extra space
 *
 * For the unicode version, the charset is expected to be in UTF-8
 */
template<typename T>
bool readBruteforce(const char *spec, const CharsetMap<T> &charsets, MaskList<T> &ml)
{
    static_assert(std::is_same<T, char>::value || std::is_same<T, uint32_t>::value, "readBruteforce requires char or uint32_t as template parameter");
    typedef typename std::conditional<std::is_same<T, char>::value, Helper8bits, HelperUnicode>::type Helper;

#if defined(__WINDOWS__) || defined(__CYGWIN__)
    int fd = open(spec, O_RDONLY | O_BINARY);
#else
    int fd = open(spec, O_RDONLY);
#endif

    FILE *f = fdopen(fd, "rb");
    char *line = NULL;
    size_t line_size = 0;
    ssize_t r;
    unsigned int line_number = 0;

    bool got_mask_len = false;
    unsigned int mask_len = 0;

    // the first objective is to build this vector describing the constraints read from the file
    std::vector<ConstrainedCharset<T>> constrained_charsets;

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

        if (!got_mask_len) {
            if (sscanf(line, "%u", &mask_len) != 1) {
                fprintf(stderr, "Error while reading the width from '%s' at line '%u'\n", spec, line_number);
                fclose(f);
                free(line);
                return false;
            }
            got_mask_len = true;
        }
        else {
            int consumed = 0;
            unsigned int min_len = 0, max_len = 0;
            if (sscanf(line, "%u %u %n", &min_len, &max_len, &consumed) != 2) {
                fprintf(stderr, "Error while reading the charset constraints from '%s' at line '%u'\n", spec, line_number);
                fclose(f);
                free(line);
                return false;
            }

            DefaultCharset<T> new_charset;
            new_charset.final = false;

            if (!Helper::readCharset(line + consumed, r - consumed, new_charset.cset)) {
                fprintf(stderr, "Error: the charset at line '%u' is invalid\n", line_number);
                fclose(f);
                free(line);
                return false;
            }

            if (new_charset.cset.size() == 0) {
                fprintf(stderr, "Error: the charset at line '%u' is empty\n", line_number);
                fclose(f);
                free(line);
                return false;
            }

            // now to expand this charset, we need a name...
            // this charset is anonymous, so let's use a forbidden name for it
            // two charset names can't be used by the user: \0 and ?
            if (!expandCharset(charsets, new_charset, T('\0'))) {
                fprintf(stderr, "Error while expanding the charset from '%s' at line '%u'\n", spec, line_number);
                fclose(f);
                free(line);
                return false;
            }

            constrained_charsets.emplace_back(new_charset, min_len, max_len);
        }
    }

    if (constrained_charsets.size() == 0 || !got_mask_len) {
        fprintf(stderr, "Error, expected at least a word width and a charset in '%s'\n", spec);
        fclose(f);
        free(line);
        return false;
    }

    // now let's generate the masks
    maskListFromConstraints<T>(constrained_charsets, mask_len, ml);

    free(line);
    fclose(f);
    return true;
}

bool readBruteforceAscii(const char *spec, const CharsetMapAscii &charsets, MaskList<char> &ml)
{
    return readBruteforce<char>(spec, charsets, ml);
}

bool readBruteforceUtf8(const char *spec, const CharsetMapUnicode &charsets, MaskList<uint32_t> &ml)
{
    return readBruteforce<uint32_t>(spec, charsets, ml);
}
}
