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

#ifndef UTF_CONV_H_
#define UTF_CONV_H_

#include "utf_conv_impl.h"

namespace UTF {

typedef impl::RetCode RetCode;

#define CHARSET_CONV_FUNC(NAME, READ, CONVERT) \
template<typename OutputIt> \
static inline RetCode NAME (const char *input, size_t input_len, OutputIt output, size_t *consumed, size_t *written) { \
    return impl::unicode_conv<READ, CONVERT, OutputIt>(input, input_len, output, consumed, written); \
} \
static inline RetCode NAME (const char *input, size_t input_len, char **output, size_t *output_size, size_t *consumed, size_t *written) { \
    return impl::unicode_conv<READ, CONVERT>(input, input_len, output, output_size, consumed, written); \
}

#define CHARSET_DECODE_FUNC(NAME, READ) \
template<typename OutputIt> \
static inline RetCode NAME (const char *input, size_t input_len, OutputIt output, size_t *consumed, size_t *written) { \
    return impl::unicode_decode<READ, OutputIt>(input, input_len, output, consumed, written); \
} \
static inline RetCode NAME (const char *input, size_t input_len, uint32_t **output, size_t *output_size, size_t *consumed, size_t *written) { \
    return impl::unicode_decode<READ>(input, input_len, output, output_size, consumed, written); \
}

#define CHARSET_DECODE_ONE_FUNC(NAME, READ) \
static inline RetCode NAME (const char *input, size_t input_len, uint32_t *output, size_t *consumed) { \
    return impl::unicode_decode_one<READ>(input, input_len, output, consumed); \
}

#define CHARSET_ENCODE_FUNC(NAME, WRITE) \
template<typename OutputIt> \
static inline RetCode NAME (const uint32_t *input, size_t input_len, OutputIt output, size_t *consumed, size_t *written) { \
    return impl::unicode_encode<WRITE, OutputIt>(input, input_len, output, consumed, written); \
} \
static inline RetCode NAME (const uint32_t *input, size_t input_len, char **output, size_t *output_size, size_t *consumed, size_t *written) { \
    return impl::unicode_encode<WRITE>(input, input_len, output, output_size, consumed, written); \
}

#define CHARSET_VALIDATE(NAME, READ) \
static inline RetCode NAME (const char *input, size_t input_len, size_t *consumed, size_t *length) { \
    return impl::unicode_validate<READ>(input, input_len, consumed, length); \
}


CHARSET_CONV_FUNC(conv_utf8_to_utf16le, impl::ReadUtf8Cp, impl::CpToUtf16le)
CHARSET_CONV_FUNC(conv_utf8_to_utf16be, impl::ReadUtf8Cp, impl::CpToUtf16be)
CHARSET_CONV_FUNC(conv_utf8_to_utf32le, impl::ReadUtf8Cp, impl::CpToUtf32le)
CHARSET_CONV_FUNC(conv_utf8_to_utf32be, impl::ReadUtf8Cp, impl::CpToUtf32be)
CHARSET_DECODE_FUNC(decode_utf8, impl::ReadUtf8Cp)
CHARSET_DECODE_ONE_FUNC(decode_one_utf8, impl::ReadUtf8Cp)
CHARSET_ENCODE_FUNC(encode_utf8, impl::CpToUtf8)
CHARSET_VALIDATE(validate_utf8, impl::ReadUtf8Cp)

CHARSET_CONV_FUNC(conv_utf16le_to_utf8, impl::ReadUtf16leCp, impl::CpToUtf8)
CHARSET_CONV_FUNC(conv_utf16le_to_utf16be, impl::ReadUtf16leCp, impl::CpToUtf16be)
CHARSET_CONV_FUNC(conv_utf16le_to_utf32le, impl::ReadUtf16leCp, impl::CpToUtf32le)
CHARSET_CONV_FUNC(conv_utf16le_to_utf32be, impl::ReadUtf16leCp, impl::CpToUtf32be)
CHARSET_DECODE_FUNC(decode_utf16le, impl::ReadUtf16leCp)
CHARSET_DECODE_ONE_FUNC(decode_one_utf16le, impl::ReadUtf16leCp)
CHARSET_ENCODE_FUNC(encode_utf16le, impl::CpToUtf16le)
CHARSET_VALIDATE(validate_utf16le, impl::ReadUtf16leCp)

CHARSET_CONV_FUNC(conv_utf16be_to_utf16le, impl::ReadUtf16beCp, impl::CpToUtf16le)
CHARSET_CONV_FUNC(conv_utf16be_to_utf8, impl::ReadUtf16beCp, impl::CpToUtf8)
CHARSET_CONV_FUNC(conv_utf16be_to_utf32le, impl::ReadUtf16beCp, impl::CpToUtf32le)
CHARSET_CONV_FUNC(conv_utf16be_to_utf32be, impl::ReadUtf16beCp, impl::CpToUtf32be)
CHARSET_DECODE_FUNC(decode_utf16be, impl::ReadUtf16beCp)
CHARSET_DECODE_ONE_FUNC(decode_one_utf16be, impl::ReadUtf16beCp)
CHARSET_ENCODE_FUNC(encode_utf16be, impl::CpToUtf16be)
CHARSET_VALIDATE(validate_utf16be, impl::ReadUtf16beCp)

CHARSET_CONV_FUNC(conv_utf32le_to_utf16le, impl::ReadUtf32leCp, impl::CpToUtf16le)
CHARSET_CONV_FUNC(conv_utf32le_to_utf16be, impl::ReadUtf32leCp, impl::CpToUtf16be)
CHARSET_CONV_FUNC(conv_utf32le_to_utf8, impl::ReadUtf32leCp, impl::CpToUtf8)
CHARSET_CONV_FUNC(conv_utf32le_to_utf32be, impl::ReadUtf32leCp, impl::CpToUtf32be)
CHARSET_DECODE_FUNC(decode_utf32le, impl::ReadUtf32leCp)
CHARSET_DECODE_ONE_FUNC(decode_one_utf32le, impl::ReadUtf32leCp)
CHARSET_ENCODE_FUNC(encode_utf32le, impl::CpToUtf32le)
CHARSET_VALIDATE(validate_utf32le, impl::ReadUtf32leCp)

CHARSET_CONV_FUNC(conv_utf32be_to_utf16le, impl::ReadUtf32beCp, impl::CpToUtf16le)
CHARSET_CONV_FUNC(conv_utf32be_to_utf16be, impl::ReadUtf32beCp, impl::CpToUtf16be)
CHARSET_CONV_FUNC(conv_utf32be_to_utf32le, impl::ReadUtf32beCp, impl::CpToUtf32le)
CHARSET_CONV_FUNC(conv_utf32be_to_utf8, impl::ReadUtf32beCp, impl::CpToUtf8)
CHARSET_DECODE_FUNC(decode_utf32be, impl::ReadUtf32beCp)
CHARSET_DECODE_ONE_FUNC(decode_one_utf32be, impl::ReadUtf32beCp)
CHARSET_ENCODE_FUNC(encode_utf32be, impl::CpToUtf32be)
CHARSET_VALIDATE(validate_utf32be, impl::ReadUtf32beCp)

#undef CHARSET_VALIDATE
#undef CHARSET_ENCODE_FUNC
#undef CHARSET_DECODE_FUNC
#undef CHARSET_DECODE_ONE_FUNC
#undef CHARSET_CONV_FUNC

}

#endif /* UTF_CONV_H_ */
