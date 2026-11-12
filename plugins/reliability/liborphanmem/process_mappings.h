/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef LIBORPHANMEM_PROCESS_MAPPING_H
#define LIBORPHANMEM_PROCESS_MAPPING_H

#include "map_allocator.h"

namespace OHOS {

struct Mapping {
    uintptr_t begin;
    uintptr_t end;
    bool read;
    bool write;
    bool execute;
    char name[96];
    explicit Mapping(uintptr_t b, uintptr_t e, bool r, bool w, bool x, const char *name)
        : begin(b),
          end(e),
          read(r),
          write(w),
          execute(x),
          name{}
    {
        strlcpy(this->name, name, sizeof(this->name));
    }
};

class MappingHelper {
public:
    static bool ProcessMappings(pid_t pid, MAP_ALLOCATOR::vector<Mapping> &mappings);

private:
    static bool ReadMapsFile(pid_t pid, MAP_ALLOCATOR::String &content);
    static uint16_t ParsePermissions(const char *perms);
    static void ExtractMappingName(const char *lineStart, const char *lineEnd, char *name, size_t nameSize);
    static void ParseMappingLine(const char *lineStart, const char *lineEnd, MAP_ALLOCATOR::vector<Mapping> &mappings);
};

}  // namespace OHOS

#endif  // LIBORPHANMEM_PROCESS_MAPPING_H
