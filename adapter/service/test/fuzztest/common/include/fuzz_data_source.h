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

#ifndef FUZZ_DATA_SOURCE_H
#define FUZZ_DATA_SOURCE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
class FuzzDataSource {
public:
    FuzzDataSource(const uint8_t* data, size_t size) : data_(data), size_(size), offset_(0) {}

    ~FuzzDataSource() = default;

    bool HasEnoughData(size_t needLen) const
    {
        return offset_ + needLen <= size_;
    }

    size_t GetRemainingLength() const
    {
        return offset_ >= size_ ? 0 : size_ - offset_;
    }

    template<typename T>
    bool GetValue(T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        if (!HasEnoughData(sizeof(T))) {
            return false;
        }
        errno_t err = memcpy_s(&value, sizeof(T), data_ + offset_, sizeof(T));
        if (err != EOK) {
            return false;
        }
        offset_ += sizeof(T);
        return true;
    }

    bool GetString(std::string& str, size_t maxLen)
    {
        size_t remaining = GetRemainingLength();
        if (remaining == 0) {
            return false;
        }
        if (maxLen > remaining) {
            maxLen = remaining;
        }
        const char* start = reinterpret_cast<const char*>(data_ + offset_);
        size_t actualLen = 0;
        while (actualLen < maxLen && start[actualLen] != '\0') {
            ++actualLen;
        }
        str.assign(start, actualLen);
        offset_ += actualLen;
        if (actualLen < maxLen) {
            offset_ += 1;
        }
        return true;
    }

    const uint8_t* GetCurrentData() const
    {
        return data_ + offset_;
    }

    void Advance(size_t len)
    {
        offset_ += len;
    }

private:
    const uint8_t* data_ {nullptr};
    size_t size_ {0};
    size_t offset_ {0};
};
}
}

#endif
