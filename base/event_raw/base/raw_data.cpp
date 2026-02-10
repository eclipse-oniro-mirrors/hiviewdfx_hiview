/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "base/raw_data.h"

#include <new>

#include "hiview_logger.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventRaw {
DEFINE_LOG_TAG("HiView-RawData");
namespace {
constexpr size_t EXPAND_BUF_SIZE = 128;
}

RawData::RawData()
{
    data_ = new(std::nothrow) uint8_t[EXPAND_BUF_SIZE];
    if (data_ == nullptr) {
        return;
    }
    capacity_ = EXPAND_BUF_SIZE;
    len_ = 0;
}

RawData::RawData(size_t dataLen)
{
    data_ = new(std::nothrow) uint8_t[dataLen];
    if (data_ == nullptr) {
        return;
    }
    capacity_ = dataLen;
    len_ = 0;
}

RawData::RawData(uint8_t* data, size_t dataLen)
{
    if (data == nullptr || dataLen == 0) {
        data_ = new(std::nothrow) uint8_t[EXPAND_BUF_SIZE];
        if (data_ == nullptr) {
            return;
        }
        capacity_ = EXPAND_BUF_SIZE;
        len_ = 0;
        return;
    }
    data_ = new(std::nothrow) uint8_t[dataLen];
    if (data_ == nullptr) {
        return;
    }
    auto ret = memcpy_s(data_, dataLen, data, dataLen);
    if (ret != EOK) {
        HIVIEW_LOGE("Failed to copy RawData in constructor, ret is %{public}d.", ret);
        return;
    }
    capacity_ = dataLen;
    len_ = dataLen;
}

RawData::RawData(const RawData& data)
{
    auto dataLen = data.GetDataLength();
    if (dataLen == 0) {
        return;
    }
    data_ = new(std::nothrow) uint8_t[dataLen];
    if (data_ == nullptr) {
        return;
    }
    auto ret = memcpy_s(data_, dataLen, data.GetData(), dataLen);
    if (ret != EOK) {
        HIVIEW_LOGE("Failed to copy RawData in constructor, ret is %{public}d.", ret);
        return;
    }
    capacity_ = dataLen;
    len_ = dataLen;
}

RawData& RawData::operator=(const RawData& data)
{
    if (this == &data) {
        return *this;
    }
    auto dataLen = data.GetDataLength();
    if (dataLen == 0) {
        return *this;
    }
    uint8_t* tmpData = new(std::nothrow) uint8_t[dataLen];
    if (tmpData == nullptr) {
        return *this;
    }
    auto ret = memcpy_s(tmpData, dataLen, data.GetData(), dataLen);
    if (ret != EOK) {
        HIVIEW_LOGE("Failed to copy RawData in constructor, ret is %{public}d.", ret);
        delete[] tmpData;
        return *this;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_ != nullptr) {
        delete[] data_;
    }
    data_ = tmpData;
    capacity_ = dataLen;
    len_ = dataLen;
    return *this;
}

RawData::~RawData()
{
    if (data_ != nullptr) {
        delete[] data_;
        data_ = nullptr;
    }
}

void RawData::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (data_ != nullptr) {
        delete[] data_;
        data_ = nullptr;
    }
    data_ = new(std::nothrow) uint8_t[EXPAND_BUF_SIZE];
    if (data_ == nullptr) {
        return;
    }
    capacity_ = EXPAND_BUF_SIZE;
    len_ = 0;
}

bool RawData::Append(uint8_t* data, size_t len)
{
    if (len == 0) {
        return true;
    }
    return Update(data, len, len_);
}

bool RawData::IsEmpty()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return len_ == 0 || data_ == nullptr;
}

bool RawData::Update(uint8_t* data, size_t len, size_t pos)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (data == nullptr || pos > len_) {
        HIVIEW_LOGE("Invalid memory operation: length is %{public}zu, pos is %{public}zu, "
            "len_ is %{public}zu", len, pos, len_);
        return false;
    }
    if (data_ == nullptr) {
        HIVIEW_LOGE("Try to update an invalid raw data");
        return false;
    }
    auto ret = EOK;
    if ((pos + len) > capacity_) {
        size_t expandedSize = (len > EXPAND_BUF_SIZE) ? len : EXPAND_BUF_SIZE;
        size_t expandedCapacity = capacity_ + expandedSize;
        uint8_t* resizedData = new(std::nothrow) uint8_t[expandedCapacity];
        if (resizedData == nullptr) {
            return false;
        }
        ret = memcpy_s(resizedData, expandedCapacity, data_, len_);
        if (ret != EOK) {
            HIVIEW_LOGE("Failed to expand capacity of raw data, ret is %{public}d.", ret);
            delete[] resizedData;
            return false;
        }
        capacity_ += expandedSize;
        delete[] data_;
        data_ = resizedData;
    }
    // append new data
    ret = memcpy_s(data_ + pos, capacity_ - pos, data, len);
    if (ret != EOK) {
        HIVIEW_LOGE("Failed to append new data, ret is %{public}d.", ret);
        return false;
    }
    if ((pos + len) > len_) {
        len_ = pos + len;
    }
    return true;
}

uint8_t* RawData::GetData() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return data_;
}

size_t RawData::GetDataLength() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return len_;
}
} // namespace EventRaw
} // namespace HiviewDFX
} // namespace OHOS