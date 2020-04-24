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

#include <cstdlib>
#include <cstdint>
#include <endian.h>
#include "utf_conv.h"

#ifndef UTF_CONV_IMPL_H_
#define UTF_CONV_IMPL_H_

/*
 * This file defines the following classes :
 * - ReadUTf8Cp : read 1 codepoint from an UTF-8 stream
 * - ReadUTf16leCp / ReadUTf16beCp : read 1 codepoint from an UTF-16 stream
 * - ReadUTf32leCp / ReadUTf32beCp : read 1 codepoint from an UTF-32 stream
 * - CpToUtf8 : write 1 codepoint to an UTF-8 stream
 * - CpToUtf16le / CpToUtf16be : write 1 codepoint to an UTF-16 stream
 * - CpToUtf32le / CpToUtf32be : write 1 codepoint to an UTF-32 stream
 *
 * The Read* classes return the number of bytes read (1 to 4) or -1 on error
 * The CpTo* classes return the number of bytes written (1 to 4) or -1 on error
 *
 * The Read* classes validate the input data (illegal codepoints, overlong encoding)
 * The CpTo* classes do not validate the input
 *
 * Based on these classes, the following templated functions are defined :
 * - stream conversion :
 *   (1) template<typename Read, typename Encode, typename OutputIt> RetCode unicode_conv(const char *input, size_t input_len, OutputIt output, size_t *consumed, size_t *written)
 *   (2) template<typename Read, typename Encode> RetCode unicode_conv(const char *input, size_t input_len, char **output, size_t *output_size, size_t *consumed, size_t *written)
 * - stream decoding :
 *   (1) template<typename Read, typename OutputIt> RetCode unicode_decode(const char *input, size_t input_len, OutputIt output, size_t *consumed, size_t *written)
 *   (2) template<typename Read> RetCode unicode_decode(const char *input, size_t input_len, uint32_t **output, size_t *output_size, size_t *consumed, size_t *written)
 *   (3) template<typename Read> RetCode unicode_decode_one(const char *input, size_t input_len, uint32_t *output, size_t *consumed)
 * - stream encoding :
 *   (1) template<typename Encode, typename OutputIt> RetCode unicode_encode(const uint32_t *input, size_t input_len, OutputIt output, size_t *consumed, size_t *written)
 *   (2) template<typename Encode> RetCode unicode_encode(const uint32_t *input, size_t input_len, char **output, size_t *output_size, size_t *consumed, size_t *written)
 * - stream validation and length counting :
 *   (4) template<typename Read> RetCode unicode_validate(const char *input, size_t input_len, size_t *consumed, size_t *length)
 *
 * template parameters :
 * - Read : a Read* class
 * - Encode : a CpTo* class
 * - OutputIt : an iterator like object (LegacyOutputIterator)
 *
 * Arguments:
 * (1) : the output argument of these functions is an iterator-like object
 *       input : beginning of the input stream
 *       input_len : number of elements in the input stream (!= byte size)
 *       output : beginning of the destination range (must be an LegacyOutputIterator, like a std::back_inserter)
 *                for the stream conversions and encoding functions, output must accept char or unsigned char data
 *                for the decoding functions, output must accept uint32_t data
 *       consumed : store the number of elements read from input. If *consumed == input_len, there was no error
 *       written : store the number of elements written into output
 *       return : error code (OK, E_INVALID, E_TRUNCATED, E_PARAMS)
 * (2) : the output and output_size arguments of these functions have the same semantics as the output arguments of the POSIX.1-2008 getline function
 *       input : beginning of the input stream
 *       input_len : number of elements in the input stream (!= byte size)
 *       output : store the address of the beginning of the output stream *output
 *       output_size : store the size of the malloc-allocated memory for *output
 *         if *output is NULL and *output_size is 0, then the function will malloc a new buffer
 *         if the malloced-size is too small, *output is reallocated and *output_size is updated
 *       consumed : store the number of elements read from input. If *consumed == input_len, there was no error
 *       written : store the number of elements written into output
 *       return : error code (OK, E_INVALID, E_TRUNCATED, E_PARAMS)
 * (3) : 
 *       input : beginning of the input stream
 *       input_len : number of elements in the input stream (!= byte size)
 *       output : store the codepoint
 *       consumed : store the number of elements read from input for this single codepoint.
 *       return : error code (OK, E_INVALID, E_TRUNCATED, E_PARAMS)
 * (4) :
 *       input : beginning of the input stream
 *       input_len : number of elements in the input stream (!= byte size)
 *       consumed : store the number of elements read from input. If *consumed == input_len, there was no error
 *       length : store the number of unicode characters read from the input stream
 *       return : error code (OK, E_INVALID, E_TRUNCATED, E_PARAMS)
 */

namespace UTF {
namespace impl {

/* Used to specialize the UTF-16 and UTF-32 encoders and decoders */
struct BigEndian {
    static inline __attribute__((always_inline)) uint16_t from(uint16_t v) {
        return be16toh(v);
    }
    static inline __attribute__((always_inline)) uint32_t from(uint32_t v) {
        return be32toh(v);
    }
    static inline __attribute__((always_inline)) uint16_t to(uint16_t v) {
        return htobe16(v);
    }
    static inline __attribute__((always_inline)) uint32_t to(uint32_t v) {
        return htobe32(v);
    }
};
struct LittleEndian {
    static inline __attribute__((always_inline)) uint16_t from(uint16_t v) {
        return le16toh(v);
    }
    static inline __attribute__((always_inline)) uint32_t from(uint32_t v) {
        return le32toh(v);
    }
    static inline __attribute__((always_inline)) uint16_t to(uint16_t v) {
        return htole16(v);
    }
    static inline __attribute__((always_inline)) uint32_t to(uint32_t v) {
        return htole32(v);
    }
};

enum RetCode {
    OK = 0,
    E_INVALID = 1,
    E_TRUNCATED = 2,
    E_PARAMS = 3
};

/*
 * UTF-8 decoder
 */
struct ReadUtf8Cp {
    static inline __attribute__((always_inline))
    int read(const char *input, size_t input_len, uint32_t &cp_out) {
        if (input_len == 0) {
            return 0;
        }

        uint8_t byte_0 = *(uint8_t*) input;
        uint8_t byte_1, byte_2, byte_3;
        uint32_t cp;

        if ((byte_0 & 0b10000000) == 0) {
            cp_out = byte_0;
            return 1;
        }

        if ((byte_0 & 0b11100000) == 0b11000000) {
            if (input_len < (size_t) (1 + 1)) {
                return 0;//-1;
            }
            byte_1 = ((uint8_t*) input)[1];
            if ((byte_1 & 0b11000000) != 0b10000000) {
                return -1;
            }
            cp = ((byte_0 & 0b00011111) << 6) | (byte_1 & 0b00111111);

            if (cp < 0x80) { // overlong encoding
                return -1;
            }
            cp_out = cp;
            return 2;
        }

        if ((byte_0 & 0b11110000) == 0b11100000) {
            if (input_len < (size_t) (1 + 2)) {
                return 0;//-1;
            }
            byte_1 = ((uint8_t*) input)[1];
            if ((byte_1 & 0b11000000) != 0b10000000) {
                return -1;
            }
            byte_2 = ((uint8_t*) input)[2];
            if ((byte_2 & 0b11000000) != 0b10000000) {
                return -1;
            }
            cp = ((byte_0 & 0b00001111) << 12) | ((byte_1 & 0b00111111) << 6) | (byte_2 & 0b00111111);

            if (cp < 0x800) { // overlong encoding
                return -1;
            }
            if (cp >= 0xD800 && cp <= 0xDFFF) { // Illegal codepoints, reserved for UTF-16
                return -1;
            }
            cp_out = cp;
            return 3;
        }

        if ((byte_0 & 0b11111000) == 0b11110000) {
            if (input_len < (size_t) (1 + 3)) {
                return 0;//-1;
            }
            byte_1 = ((uint8_t*) input)[1];
            if ((byte_1 & 0b11000000) != 0b10000000) {
                return -1;
            }
            byte_2 = ((uint8_t*) input)[2];
            if ((byte_2 & 0b11000000) != 0b10000000) {
                return -1;
            }
            byte_3 = ((uint8_t*) input)[3];
            if ((byte_3 & 0b11000000) != 0b10000000) {
                return -1;
            }
            cp = ((byte_0 & 0b00000111) << 18) | ((byte_1 & 0b00111111) << 12) | ((byte_2 & 0b00111111) << 6) | (byte_3 & 0b00111111);

            if (cp < 0x10000) { // overlong encoding
                return -1;
            }
            if (cp > 0x10FFFF) { // Maximum codepoint
                return -1;
            }
            cp_out = cp;
            return 4;
        }

        return -1;
    }
};

/*
 * UTF-16 decoder
 */
template<typename endianness>
struct ReadUtf16Cp {
    static inline __attribute__((always_inline))
    int read(const char *input, size_t input_len, uint32_t &cp_out) {
        if (input_len == 0) {
            return 0;
        }
        if (input_len == 1) {
            return 0;
        }

        uint16_t high = endianness::from(*(uint16_t*) input);

        if (high <= 0xD7FF || high >= 0xE000) {
            cp_out = high;
            return 2;
        } else if (high >= 0xD800 && high <= 0xDBFF) {
            if (input_len < 4) {
                return 0;
            }
            uint16_t low = endianness::from(*(uint16_t*) (input + 2));

            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp_out = 0x10000 + (((high - 0xD800) << 10) | (low - 0xDC00));
                return 4;
            }
        }

        return -1;
    }
};
typedef ReadUtf16Cp<LittleEndian> ReadUtf16leCp;
typedef ReadUtf16Cp<BigEndian> ReadUtf16beCp;

/*
 * UTF-32 decoder
 */
template<typename endianness>
struct ReadUtf32Cp {
    static inline __attribute__((always_inline))
    int read(const char *input, size_t input_len, uint32_t &cp_out) {
        if (input_len < 4) {
            return 0;
        }

        uint32_t v = endianness::from(*(uint32_t*) input);

        if (v <= 0xD7FF || (v >= 0xE000 && v <= 0x10FFFF)) {
            cp_out = v;
            return 4;
        }

        return -1;
    }
};
typedef ReadUtf32Cp<LittleEndian> ReadUtf32leCp;
typedef ReadUtf32Cp<BigEndian> ReadUtf32beCp;

/*
 * UTF-8 encoder
 */
struct CpToUtf8 {
    template<typename OutputIt>
    static inline __attribute__((always_inline))
    int write(uint32_t cp, OutputIt output) {
        if (cp <= 0x7F) {
            *output++ = cp & 0b1111111;
            return 1;
        } else if (cp <= 0x7FF) {
            *output++ = 0b11000000 | (cp >> 6);
            *output++ = 0b10000000 | (cp & 0b111111);
            return 2;
        } else if (cp <= 0xFFFF) {
            *output++ = 0b11100000 | (cp >> 12);
            *output++ = 0b10000000 | ((cp >> 6) & 0b111111);
            *output++ = 0b10000000 | (cp & 0b111111);
            return 3;
        } else {
            *output++ = 0b11110000 | (cp >> 18);
            *output++ = 0b10000000 | ((cp >> 12) & 0b111111);
            *output++ = 0b10000000 | ((cp >> 6) & 0b111111);
            *output++ = 0b10000000 | (cp & 0b111111);
            return 4;
        }
    }
};

/*
 * UTF-16 encoder
 */
template<typename endianness>
struct CpToUtf16 {
    template<typename OutputIt>
    static inline __attribute__((always_inline))
    int write(uint32_t cp, OutputIt output) {
        // BMP
        if (cp <= 0xD7FF || (cp >= 0xE000 && cp <= 0xFFFF)) {
            uint16_t high = endianness::to(uint16_t(cp & 0xFFFF));
            *output++ = high & 0xFF;
            *output++ = (high >> 8) & 0xFF;
            return 2;
        } else {
            cp -= 0x10000;
            uint16_t high = endianness::to(uint16_t(0xD800 + (cp >> 10)));
            uint16_t low = endianness::to(uint16_t(0xDC00 + (cp & 0x3FF)));
            *output++ = high & 0xFF;
            *output++ = (high >> 8) & 0xFF;
            *output++ = low & 0xFF;
            *output++ = (low >> 8) & 0xFF;
            return 4;
        }
    }
};
typedef CpToUtf16<LittleEndian> CpToUtf16le;
typedef CpToUtf16<BigEndian> CpToUtf16be;

/*
 * UTF-32 encoder
 */
template<typename endianness>
struct CpToUtf32 {
    template<typename OutputIt>
    static inline __attribute__((always_inline))
    int write(uint32_t cp, OutputIt output) {
        uint32_t v = endianness::to(uint32_t(cp));
        *output++ = v & 0xFF;
        *output++ = (v >> 8) & 0xFF;
        *output++ = (v >> 16) & 0xFF;
        *output++ = (v >> 24) & 0xFF;
        return 4;
    }
};
typedef CpToUtf32<LittleEndian> CpToUtf32le;
typedef CpToUtf32<BigEndian> CpToUtf32be;

/*
 * Generic UTF conversion function, iterator version
 * output must accept char or unsigned char data
 */
template<typename Read, typename Encode, typename OutputIt>
static inline __attribute__((always_inline))
RetCode unicode_conv(const char *input, size_t input_len, OutputIt output, size_t *consumed, size_t *written) {
    RetCode ret = RetCode::OK;
    size_t w = 0;
    if (!input) {
        return RetCode::E_PARAMS;
    }
    if (consumed) {
        *consumed = 0;
    }
    while (input_len != 0) {
        uint32_t cp;
        int removed = Read::read(input, input_len, cp);
        if (removed < 0) {
            ret = RetCode::E_INVALID;
            break;
        }
        if (removed == 0) {
            ret = RetCode::E_TRUNCATED;
            break;
        }
        input += removed;
        input_len -= removed;

        int encoded = Encode::write(cp, output);

        if (consumed) {
            *consumed += removed;
        }

        w += encoded;
    }

    if (written) {
        *written = w;
    }
    return ret;
}

/*
 * Generic UTF conversion function, getline-style version
 */
template<typename Read, typename Encode>
static inline __attribute__((always_inline))
RetCode unicode_conv(const char *input, size_t input_len, char **output, size_t *output_size, size_t *consumed, size_t *written) {
    RetCode ret = RetCode::OK;
    size_t w = 0;
    if (!input || !output || !output_size) {
        return RetCode::E_PARAMS;
    }
    if (*output_size == 0) {
        *output = NULL;
    }
    if (consumed) {
        *consumed = 0;
    }
    while (input_len != 0) {
        uint32_t cp;
        int removed = Read::read(input, input_len, cp);
        if (removed < 0) {
            ret = RetCode::E_INVALID;
            break;
        }
        if (removed == 0) {
            ret = RetCode::E_TRUNCATED;
            break;
        }
        input += removed;
        input_len -= removed;

        // more efficient than the iterator version because the avalaible size is checked less often
        if (w + 4 > *output_size) {
            *output_size += input_len * 2 + 8;
            *output = (char *) realloc(*output, *output_size);
        }

        int encoded = Encode::write(cp, *output + w);

        if (consumed) {
            *consumed += removed;
        }

        w += encoded;
    }

    if (written) {
        *written = w;
    }
    return ret;
}

/*
 * Generic UTF decoder, iterator version
 * output must accept uint32_t data for the codepoints
 */
template<typename Read, typename OutputIt>
static inline __attribute__((always_inline))
RetCode unicode_decode(const char *input, size_t input_len, OutputIt output, size_t *consumed, size_t *written) {
    RetCode ret = RetCode::OK;
    size_t w = 0;
    if (!input) {
        return RetCode::E_PARAMS;
    }
    if (consumed) {
        *consumed = 0;
    }
    while (input_len != 0) {
        uint32_t cp;
        int removed = Read::read(input, input_len, cp);
        if (removed < 0) {
            ret = RetCode::E_INVALID;
            break;
        }
        if (removed == 0) {
            ret = RetCode::E_TRUNCATED;
            break;
        }
        input += removed;
        input_len -= removed;

        *output++ = cp;

        if (consumed) {
            *consumed += removed;
        }

        w += 1;
    }

    if (written) {
        *written = w;
    }
    return ret;
}

/*
 * Generic UTF decoder, getline-style version
 */
template<typename Read>
static inline __attribute__((always_inline))
RetCode unicode_decode(const char *input, size_t input_len, uint32_t **output, size_t *output_size, size_t *consumed, size_t *written) {
    RetCode ret = RetCode::OK;
    size_t w = 0;
    if (!input || !output || !output_size) {
        return RetCode::E_PARAMS;
    }
    if (*output_size == 0) {
        *output = NULL;
    }
    if (consumed) {
        *consumed = 0;
    }
    while (input_len != 0) {
        uint32_t cp;
        int removed = Read::read(input, input_len, cp);
        if (removed < 0) {
            ret = RetCode::E_INVALID;
            break;
        }
        if (removed == 0) {
            ret = RetCode::E_TRUNCATED;
            break;
        }
        input += removed;
        input_len -= removed;

        if (w + 1 > *output_size) {
            *output_size += input_len  + 8;
            *output = (uint32_t *) realloc(*output, (*output_size) * 4);
        }
        (*output)[w] = cp;

        if (consumed) {
            *consumed += removed;
        }

        w += 1;
    }

    if (written) {
        *written = w;
    }
    return ret;
}

/*
 * UTF decoder, read only one sequence
 */
template<typename Read>
static inline __attribute__((always_inline))
RetCode unicode_decode_one(const char *input, size_t input_len, uint32_t *output, size_t *consumed) {
    RetCode ret = RetCode::OK;
    if (!input || input_len == 0) {
        return RetCode::E_PARAMS;
    }
    if (consumed) {
        *consumed = 0;
    }

    uint32_t cp;
    int removed = Read::read(input, input_len, cp);
    if (removed < 0) {
        return RetCode::E_INVALID;
    }
    if (removed == 0) {
        return RetCode::E_TRUNCATED;
    }
    *output = cp;

    if (consumed) {
        *consumed += removed;
    }

    return ret;
}

/*
 * Generic UTF validator and length counter
 */
template<typename Read>
static inline __attribute__((always_inline))
RetCode unicode_validate(const char *input, size_t input_len, size_t *consumed, size_t *length) {
    RetCode ret = RetCode::OK;
    size_t w = 0;
    if (!input) {
        return RetCode::E_PARAMS;
    }
    if (consumed) {
        *consumed = 0;
    }
    while (input_len != 0) {
        uint32_t cp;
        int removed = Read::read(input, input_len, cp);
        if (removed < 0) {
            ret = RetCode::E_INVALID;
            break;
        }
        if (removed == 0) {
            ret = RetCode::E_TRUNCATED;
            break;
        }
        input += removed;
        input_len -= removed;

        if (consumed) {
            *consumed += removed;
        }

        w += 1;
    }

    if (length) {
        *length = w;
    }
    return ret;
}

/*
 * Generic UTF encoder, iterator version
 * output must accept char or unsigned char data
 * The input is checked for validity
 */
template<typename Encode, typename OutputIt>
static inline __attribute__((always_inline))
RetCode unicode_encode(const uint32_t *input, size_t input_len, OutputIt output, size_t *consumed, size_t *written) {
    RetCode ret = RetCode::OK;
    size_t w = 0;
    if (!input) {
        return RetCode::E_PARAMS;
    }
    if (consumed) {
        *consumed = 0;
    }
    while (input_len != 0) {
        uint32_t cp = *input;
        if ((cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
            ret = RetCode::E_INVALID;
            break;
        }
        input++;
        input_len--;

        int encoded = Encode::write(cp, output);

        if (consumed) {
            *consumed += 1;
        }

        w += encoded;
    }

    if (written) {
        *written = w;
    }
    return ret;
}

/*
 * Generic UTF encoder, getline-style version
 * The input is checked for validity
 */
template<typename Encode>
static inline __attribute__((always_inline))
RetCode unicode_encode(const uint32_t *input, size_t input_len, char **output, size_t *output_size, size_t *consumed, size_t *written) {
    RetCode ret = RetCode::OK;
    size_t w = 0;
    if (!input || !output || !output_size) {
        return RetCode::E_PARAMS;;
    }
    if (*output_size == 0) {
        *output = NULL;
    }
    if (consumed) {
        *consumed = 0;
    }
    while (input_len != 0) {
        uint32_t cp = *input;
        if ((cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
            ret = RetCode::E_INVALID;
            break;
        }
        input++;
        input_len--;

        if (w + 4 > *output_size) {
            *output_size += input_len + 8;
            *output = (char *) realloc(*output, *output_size);
        }

        int encoded = Encode::write(cp, *output + w);

        if (consumed) {
            *consumed += 1;
        }

        w += encoded;
    }

    if (written) {
        *written = w;
    }
    return ret;
}

}
}

#endif /* UTF_CONV_IMPL_H_ */
