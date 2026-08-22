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

#include "decoded/decoded_param.h"

#include <cinttypes>

#include "decoded/raw_data_decoder.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventRaw {
std::string DecodedParam::GetKey()
{
    return key_;
}

size_t DecodedParam::GetPosition()
{
    return pos_;
}

bool DecodedParam::AsUint64(uint64_t& dest)
{
    return false;
}

bool DecodedParam::AsInt64(int64_t& dest)
{
    return false;
}

bool DecodedParam::AsDouble(double& dest)
{
    return false;
}

bool DecodedParam::AsString(std::string& dest)
{
    return false;
}

bool DecodedParam::AsUint64Vec(std::vector<uint64_t>& dest)
{
    return false;
}

bool DecodedParam::AsInt64Vec(std::vector<int64_t>& dest)
{
    return false;
}

bool DecodedParam::AsDoubleVec(std::vector<double>& dest)
{
    return false;
}

bool DecodedParam::AsStringVec(std::vector<std::string>& dest)
{
    return false;
}

bool UnsignedVarintDecodedParam::DecodeValue()
{
    return RawDataDecoder::UnsignedVarintDecoded(rawData_, maxLen_, pos_, val_);
}

DataCodedType UnsignedVarintDecodedParam::GetDataCodedType()
{
    return DataCodedType::UNSIGNED_VARINT;
}

bool UnsignedVarintDecodedParam::AsUint64(uint64_t& dest)
{
    dest = val_;
    return true;
}

bool UnsignedVarintDecodedArrayParam::DecodeValue()
{
    uint64_t size = 0;
    auto ret = RawDataDecoder::UnsignedVarintDecoded(rawData_, maxLen_, pos_, size);
    if (size > MAX_ARRAY_SIZE) {
        return false;
    }
    while (size > 0) {
        uint64_t val = 0;
        ret = ret && RawDataDecoder::UnsignedVarintDecoded(rawData_, maxLen_, pos_, val);
        if (!ret) {
            return ret;
        }
        vals_.emplace_back(val);
        size--;
    }
    return ret;
}

DataCodedType UnsignedVarintDecodedArrayParam::GetDataCodedType()
{
    return DataCodedType::UNSIGNED_VARINT_ARRAY;
}

bool UnsignedVarintDecodedArrayParam::AsUint64Vec(std::vector<uint64_t>& dest)
{
    dest.assign(vals_.begin(), vals_.end());
    return true;
}

bool SignedVarintDecodedParam::DecodeValue()
{
    return RawDataDecoder::SignedVarintDecoded(rawData_, maxLen_, pos_, val_);
}

DataCodedType SignedVarintDecodedParam::GetDataCodedType()
{
    return DataCodedType::SIGNED_VARINT;
}

bool SignedVarintDecodedParam::AsInt64(int64_t& dest)
{
    dest = val_;
    return true;
}

bool SignedVarintDecodedArrayParam::DecodeValue()
{
    uint64_t size = 0;
    auto ret = RawDataDecoder::UnsignedVarintDecoded(rawData_, maxLen_, pos_, size);
    if (size > MAX_ARRAY_SIZE) {
        return false;
    }
    while (size > 0) {
        int64_t val = 0;
        ret = ret && RawDataDecoder::SignedVarintDecoded(rawData_, maxLen_, pos_, val);
        if (!ret) {
            return ret;
        }
        vals_.emplace_back(val);
        size--;
    }
    return ret;
}

DataCodedType SignedVarintDecodedArrayParam::GetDataCodedType()
{
    return DataCodedType::SIGNED_VARINT_ARRAY;
}

bool SignedVarintDecodedArrayParam::AsInt64Vec(std::vector<int64_t>& dest)
{
    dest.assign(vals_.begin(), vals_.end());
    return true;
}

bool FloatingNumberDecodedParam::DecodeValue()
{
    return RawDataDecoder::FloatingNumberDecoded(rawData_, maxLen_, pos_, val_);
}

DataCodedType FloatingNumberDecodedParam::GetDataCodedType()
{
    return DataCodedType::FLOATING;
}

bool FloatingNumberDecodedParam::AsDouble(double& dest)
{
    dest = val_;
    return true;
}

bool FloatingNumberDecodedArrayParam::DecodeValue()
{
    uint64_t size = 0;
    auto ret = RawDataDecoder::UnsignedVarintDecoded(rawData_, maxLen_, pos_, size);
    if (size > MAX_ARRAY_SIZE) {
        return false;
    }
    while (size > 0) {
        double val = 0;
        ret = ret && RawDataDecoder::FloatingNumberDecoded(rawData_, maxLen_, pos_, val);
        if (!ret) {
            return ret;
        }
        vals_.emplace_back(val);
        size--;
    }
    return ret;
}

DataCodedType FloatingNumberDecodedArrayParam::GetDataCodedType()
{
    return DataCodedType::FLOATING_ARRAY;
}

bool FloatingNumberDecodedArrayParam::AsDoubleVec(std::vector<double>& dest)
{
    dest.assign(vals_.begin(), vals_.end());
    return true;
}

bool StringDecodedParam::DecodeValue()
{
    return RawDataDecoder::StringValueDecoded(rawData_, maxLen_, pos_, val_);
}

DataCodedType StringDecodedParam::GetDataCodedType()
{
    return DataCodedType::DSTRING;
}

bool StringDecodedParam::AsString(std::string& dest)
{
    dest = val_;
    return true;
}

bool StringDecodedArrayParam::DecodeValue()
{
    uint64_t size = 0;
    auto ret = RawDataDecoder::UnsignedVarintDecoded(rawData_, maxLen_, pos_, size);
    if (size > MAX_ARRAY_SIZE) {
        return false;
    }
    while (size > 0) {
        std::string val;
        ret = ret && RawDataDecoder::StringValueDecoded(rawData_, maxLen_, pos_, val);
        if (!ret) {
            return ret;
        }
        vals_.emplace_back(val);
        size--;
    }
    return ret;
}

DataCodedType StringDecodedArrayParam::GetDataCodedType()
{
    return DataCodedType::DSTRING_ARRAY;
}

bool StringDecodedArrayParam::AsStringVec(std::vector<std::string>& dest)
{
    dest.assign(vals_.begin(), vals_.end());
    return true;
}
} // namespace EventRaw
} // namespace HiviewDFX
} // namespace OHOS