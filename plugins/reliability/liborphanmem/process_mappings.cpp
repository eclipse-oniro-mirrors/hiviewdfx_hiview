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

#include "process_mappings.h"

#include <fcntl.h>
#include <securec.h>
#include <sys/mman.h>
#include <unique_fd.h>

#include "map_allocator.h"
#include "log.h"
#include "string_helper.h"

namespace OHOS {
namespace {
constexpr int MAPPING_NAME_FIELD_COUNT = 5;
constexpr int MIN_PARSED_FIELDS = 3;
}

bool MappingHelper::ProcessMappings(pid_t pid, MAP_ALLOCATOR::vector<Mapping> &mappings)
{
    MAP_ALLOCATOR::String content(Allocator<char>(mappings.get_allocator()));
    if (!ReadMapsFile(pid, content)) {
        return false;
    }

    const char *ptr = content.c_str();
    const char *end = ptr + content.length();

    while (ptr < end) {
        while (*ptr == '\n' || *ptr == '\r') {
            ptr++;
        }
        if (ptr >= end) {
            break;
        }

        const char *lineStart = ptr;
        while (*ptr != '\n' && *ptr != '\r' && *ptr != '\0') {
            ptr++;
        }
        const char *lineEnd = ptr;

        ParseMappingLine(lineStart, lineEnd, mappings);

        if (*ptr != '\0') {
            ptr++;
        }
    }

    return true;
}

bool MappingHelper::ReadMapsFile(pid_t pid, MAP_ALLOCATOR::String &content)
{
    char mapsPath[PROC_LENGTH] = "/proc/";
    StringHelper helper;
    if (!helper.ConcatenatePath(mapsPath, pid, "/maps")) {
        return false;
    }

    OHOS::UniqueFd fd(open(mapsPath, O_RDONLY));
    if (fd == -1) {
        return false;
    }

    char buffer[PROC_LENGTH];
    ssize_t n;
    while ((n = TEMP_FAILURE_RETRY(read(fd, buffer, sizeof(buffer)))) > 0) {
        content.append(buffer, n);
    }
    if (n < 0) {
        HILOGE("failed to read maps file, errno: %{public}d, err: %{public}s", errno, strerror(errno));
        return false;
    }

    return true;
}

uint16_t MappingHelper::ParsePermissions(const char *perms)
{
    uint16_t flags = 0;
    if (strchr(perms, 'r')) {
        flags |= PROT_READ;
    }
    if (strchr(perms, 'w')) {
        flags |= PROT_WRITE;
    }
    if (strchr(perms, 'x')) {
        flags |= PROT_EXEC;
    }
    return flags;
}

void MappingHelper::ExtractMappingName(const char *lineStart, const char *lineEnd, char *name, size_t nameSize)
{
    const char *p = lineStart;
    int fieldCount = 0;

    while (p < lineEnd && fieldCount < MAPPING_NAME_FIELD_COUNT) {
        if (*p == ' ') {
            while (p < lineEnd && *p == ' ') {
                p++;
            }
            fieldCount++;
        } else {
            p++;
        }
    }

    const char *nameStart = p;
    const char *nameEnd = lineEnd;
    while (nameEnd > nameStart && (*(nameEnd - 1) == ' ' || *(nameEnd - 1) == '\n' || *(nameEnd - 1) == '\r')) {
        nameEnd--;
    }

    size_t nameLen = nameEnd - nameStart;
    if (nameLen >= nameSize) {
        nameLen = nameSize - 1;
    }
    if (nameLen > 0) {
        errno_t ret = strncpy_s(name, nameSize, nameStart, nameLen);
        if (ret != EOK) {
            name[0] = '\0';
            return;
        }
        name[nameLen] = '\0';
    } else {
        name[0] = '\0';
    }
}

void MappingHelper::ParseMappingLine(const char *lineStart, const char *lineEnd,
                                     MAP_ALLOCATOR::vector<Mapping> &mappings)
{
    uintptr_t start;
    uintptr_t end;
    char perms[5] = {0};
    char name[96] = {0};

    int parsed = sscanf_s(lineStart, "%lx-%lx %4s", &start, &end, perms, sizeof(perms));
    if (parsed < MIN_PARSED_FIELDS) {
        return;
    }

    uint16_t flags = ParsePermissions(perms);
    ExtractMappingName(lineStart, lineEnd, name, sizeof(name));

    mappings.emplace_back(start, end, flags & PROT_READ, flags & PROT_WRITE, flags & PROT_EXEC, name);
}

}  // namespace OHOS
