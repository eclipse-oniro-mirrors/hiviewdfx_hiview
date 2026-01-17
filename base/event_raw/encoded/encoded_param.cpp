/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "encoded/encoded_param.h"

#include "encoded/raw_data_encoder.h"
#include "hiview_logger.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventRaw {
DEFINE_LOG_TAG("HiView-EncodedParam");
template class FloatingNumberEncodedParam<double>;
template class SignedVarintEncodedParam<int64_t>;
template class UnsignedVarintEncodedParam<uint64_t>;
EncodedParam::EncodedParam(const std::string& key)
{
    key_ = key;
}

EncodedParam::~EncodedParam()
{
}

std::string& EncodedParam::GetKey()
{
    return key_;
}

RawData& EncodedParam::GetRawData()
{
    if (!hasEncoded_) {
        hasEncoded_ = Encode();
    }
    return rawData_;
}

bool EncodedParam::AsUint64(uint64_t& dest)
{
    return false;
}

bool EncodedParam::AsInt64(int64_t& dest)
{
    return false;
}

bool EncodedParam::AsDouble(double& dest)
{
    return false;
}

bool EncodedParam::AsString(std::string& dest)
{
    return false;
}

bool EncodedParam::AsUint64Vec(std::vector<uint64_t>& dest)
{
    return false;
}

bool EncodedParam::AsInt64Vec(std::vector<int64_t>& dest)
{
    return false;
}

bool EncodedParam::AsDoubleVec(std::vector<double>& dest)
{
    return false;
}

bool EncodedParam::AsStringVec(std::vector<std::string>& dest)
{
    return false;
}

bool EncodedParam::Encode()
{
    hasEncoded_ = EncodeKey() && EncodeValueType() && EncodeValue();
    return hasEncoded_;
}

bool EncodedParam::EncodeKey()
{
    if (!RawDataEncoder::StringValueEncoded(rawData_, key_)) {
        HIVIEW_LOGE("The key of customized value encoded failded.");
        return false;
    }
    return true;
}
} // namespace EventRaw
} // namespace HiviewDFX
} // namespace OHOS
