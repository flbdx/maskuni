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
#include "MaskList.h"

#include <cstdint>
#include <vector>

namespace Maskgen {

/**
 * @brief Read a bruteforce description file, for 8-bit masks
 *
 * @param bruteforce file
 * @param charsets known charsets
 * @param ml output mask list
 * @return false an error occured
 */
bool readBruteforceAscii(const char *spec, const CharsetMapAscii &charsets, MaskList<char> &ml);
/**
 * @brief Read a bruteforce description file, for unicode masks. The file must be UTF-8 encoded
 *
 * The content of the file or the string \a spec must be UTF-8 encoded
 *
 * @param bruteforce file
 * @param charsets known charsets
 * @param ml output mask list
 * @return false an error occured
 */
bool readBruteforceUtf8(const char *spec, const CharsetMapUnicode &charsets, MaskList<uint32_t> &ml);

}
