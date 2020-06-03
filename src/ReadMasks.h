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
#include "MaskGenerator.h"

namespace Maskuni {
    
/**
 * @brief Read an 8-bits mask list from a file or from the mask given by \a spec and return a MaskGenerator for the masks
 * If a file named \a spec exists then the masks are read from the file
 * Otherwise the content of the mask list is the single mask created from the string \a spec
 * 
 * The list is not validated.
 * 
 * @param spec mask file or mask string
 * @param charsets known charsets
 * @return new MaskGenerator or NULL
 */
MaskGenerator<char> *readMaskListAscii(const char *spec, const CharsetMapAscii &charsets);
/**
 * @brief Read an unicode mask list from a file or from the mask given by \a spec and return a MaskGenerator for the masks
 * If a file named \a spec exists then the masks are read from the file
 * Otherwise the content of the mask list is the single mask created from the string \a spec
 * 
 * The content of the file or the string \a spec must be UTF-8 encoded
 * 
 * The list is not validated.
 * 
 * @param spec mask file or mask string
 * @param charsets known charsets
 * @return new MaskGenerator or NULL
 */
MaskGenerator<uint32_t> *readMaskListUtf8(const char *spec, const CharsetMapUnicode &charsets);

}
