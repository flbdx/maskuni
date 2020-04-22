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
#include <map>
#include <algorithm>

namespace Maskgen {

/**
 * @brief Read an 8-bits mask list from a file or from the mask given by \a spec
 * If a file named \a spec exists then the masks are read from the file
 * Otherwise the content of the mask list is the single mask created from the string \a spec
 * 
 * @param spec mask file or mask string
 * @param charsets known charsets
 * @param ml output mask list
 * @return false an error occured
 */
bool readMaskListAscii(const char *spec, const CharsetMapAscii &charsets, MaskList<char> &ml);
/**
 * @brief Read an unicode mask list from a file or from the mask given by \a spec
 * If a file named \a spec exists then the masks are read from the file
 * Otherwise the content of the mask list is the single mask created from the string \a spec
 * 
 * The content of the file or the string \a spec must be UTF-8 encoded
 * 
 * @param spec mask file or mask string
 * @param charsets known charsets
 * @param ml output mask list
 * @return false an error occured
 */
bool readMaskListUtf8(const char *spec, const CharsetMapUnicode &charsets, MaskList<uint32_t> &ml);

}
