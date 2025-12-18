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

#ifndef BASE_EVENT_RAW_ENCODE_INCLUDE_ENCODED_PARAM_H
#define BASE_EVENT_RAW_ENCODE_INCLUDE_ENCODED_PARAM_H

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "base/raw_data_base_def.h"
#include "base/value_param.h"
#include "encoded/raw_data_encoder.h"
#include "base/raw_data.h"

namespace OHOS {
namespace HiviewDFX {
namespace EventRaw {
class EncodedParam : public ValueParam {
public:
    EncodedParam(const std::string& key);
    virtual ~EncodedParam();

public:
    virtual std::string& GetKey();
    virtual RawData& GetRawData();
    virtual bool Encode();

public:
    virtual bool AsUint64(uint64_t& dest) override;
    virtual bool AsInt64(int64_t& dest) override;
    virtual bool AsDouble(double& dest) override;
    virtual bool AsString(std::string& dest) override;
    virtual bool AsUint64Vec(std::vector<uint64_t>& dest) override;
    virtual bool AsInt64Vec(std::vector<int64_t>& dest) override;
    virtual bool AsDoubleVec(std::vector<double>& dest) override;
    virtual bool AsStringVec(std::vector<std::string>& dest) override;

protected:
    virtual bool EncodeKey();
    virtual bool EncodeValueType() = 0;
    virtual bool EncodeValue() = 0;

protected:
    std::string key_;
    RawData rawData_;
    bool hasEncoded_ = false;
};

template<typename T,
    std::enable_if_t<std::is_same_v<std::decay_t<T>, uint8_t> || std::is_same_v<std::decay_t<T>, uint16_t> ||
    std::is_same_v<std::decay_t<T>, uint32_t> || std::is_same_v<std::decay_t<T>, uint64_t>>* = nullptr>
class UnsignedVarintEncodedParam : public EncodedParam {
public:
    UnsignedVarintEncodedParam(const std::string& key, T val): EncodedParam(key)
    {
        val_ = val;
    }

    virtual bool EncodeValueType() override
    {
        return RawDataEncoder::ValueTypeEncoded(rawData_, false, ValueType::UINT64, 0);
    }

    virtual bool EncodeValue() override
    {
        return RawDataEncoder::UnsignedVarintEncoded(rawData_, EncodeType::VARINT, val_);
    }
    
    virtual bool AsString(std::string& ret) override
    {
        ret = std::to_string(val_);
        return true;
    }

    virtual bool AsUint64(uint64_t& dest) override
    {
        dest = static_cast<uint64_t>(val_);
        return true;
    }

    virtual DataCodedType GetDataCodedType() override
    {
        return DataCodedType::UNSIGNED_VARINT;
    }

private:
    T val_;
};

template<typename T,
    std::enable_if_t<std::is_same_v<std::decay_t<T>, uint8_t> || std::is_same_v<std::decay_t<T>, uint16_t> ||
    std::is_same_v<std::decay_t<T>, uint32_t> || std::is_same_v<std::decay_t<T>, uint64_t>>* = nullptr>
class UnsignedVarintEncodedArrayParam : public EncodedParam {
public:
    UnsignedVarintEncodedArrayParam(const std::string& key, const std::vector<T>& vals): EncodedParam(key)
    {
        unsigned int index = 0;
        for (auto item = vals.begin(); item != vals.end(); item++) {
            index++;
            if (index > MAX_ARRAY_SIZE) {
                break;
            }
            vals_.emplace_back(*item);
        }
    }

    virtual bool EncodeValueType() override
    {
        return RawDataEncoder::ValueTypeEncoded(rawData_, true, ValueType::UINT64, 0);
    }

    virtual bool EncodeValue() override
    {
        bool ret = RawDataEncoder::UnsignedVarintEncoded(rawData_, EncodeType::LENGTH_DELIMITED, vals_.size());
        for (auto item : vals_) {
            ret = ret && RawDataEncoder::UnsignedVarintEncoded(rawData_, EncodeType::VARINT, item);
        }
        return ret;
    }

    virtual bool AsUint64Vec(std::vector<uint64_t>& dest) override
    {
        for (auto item : vals_) {
            dest.emplace_back(static_cast<uint64_t>(item));
        }
        return true;
    }

    virtual DataCodedType GetDataCodedType() override
    {
        return DataCodedType::UNSIGNED_VARINT_ARRAY;
    }

private:
    std::vector<T> vals_;
};

template<typename T,
    std::enable_if_t<std::is_same_v<std::decay_t<T>, bool> || std::is_same_v<std::decay_t<T>, int8_t> ||
    std::is_same_v<std::decay_t<T>, int16_t> || std::is_same_v<std::decay_t<T>, int32_t> ||
    std::is_same_v<std::decay_t<T>, int64_t>>* = nullptr>
class SignedVarintEncodedParam : public EncodedParam {
public:
    SignedVarintEncodedParam(const std::string& key, T val): EncodedParam(key)
    {
        val_ = val;
    }

    virtual bool EncodeValueType() override
    {
        return RawDataEncoder::ValueTypeEncoded(rawData_, false, ValueType::INT64, 0);
    }

    virtual bool EncodeValue() override
    {
        return RawDataEncoder::SignedVarintEncoded(rawData_, EncodeType::VARINT, val_);
    }

    virtual bool AsString(std::string& ret) override
    {
        ret = std::to_string(val_);
        return true;
    }

    virtual bool AsInt64(int64_t& dest) override
    {
        dest = static_cast<int64_t>(val_);
        return true;
    }

    virtual DataCodedType GetDataCodedType() override
    {
        return DataCodedType::SIGNED_VARINT;
    }

private:
    T val_;
};

template<typename T,
    std::enable_if_t<std::is_same_v<std::decay_t<T>, bool> || std::is_same_v<std::decay_t<T>, int8_t> ||
    std::is_same_v<std::decay_t<T>, int16_t> || std::is_same_v<std::decay_t<T>, int32_t> ||
    std::is_same_v<std::decay_t<T>, int64_t>>* = nullptr>
class SignedVarintEncodedArrayParam : public EncodedParam {
public:
    SignedVarintEncodedArrayParam(const std::string& key, const std::vector<T>& vals): EncodedParam(key)
    {
        unsigned int index = 0;
        for (auto item = vals.begin(); item != vals.end(); item++) {
            index++;
            if (index > MAX_ARRAY_SIZE) {
                break;
            }
            vals_.emplace_back(*item);
        }
    }

    virtual bool EncodeValueType() override
    {
        return RawDataEncoder::ValueTypeEncoded(rawData_, true, ValueType::INT64, 0);
    }

    virtual bool EncodeValue() override
    {
        bool ret = RawDataEncoder::UnsignedVarintEncoded(rawData_, EncodeType::LENGTH_DELIMITED, vals_.size());
        for (auto item : vals_) {
            ret = ret && RawDataEncoder::SignedVarintEncoded(rawData_, EncodeType::VARINT, item);
        }
        return ret;
    }

    virtual bool AsInt64Vec(std::vector<int64_t>& dest) override
    {
        for (auto item : vals_) {
            dest.emplace_back(static_cast<int64_t>(item));
        }
        return true;
    }

    virtual DataCodedType GetDataCodedType() override
    {
        return DataCodedType::SIGNED_VARINT_ARRAY;
    }

private:
    std::vector<T> vals_;
};

template<typename T,
    std::enable_if_t<std::is_same_v<std::decay_t<T>, float> || std::is_same_v<std::decay_t<T>, double>>* = nullptr>
class FloatingNumberEncodedParam : public EncodedParam {
public:
    FloatingNumberEncodedParam(const std::string& key, T val): EncodedParam(key)
    {
        val_ = val;
    }

    virtual bool EncodeValueType() override
    {
        auto valueType = ValueType::UNKNOWN;
        if (std::is_same_v<std::decay_t<T>, float>) {
            valueType = ValueType::FLOAT;
        }
        if (std::is_same_v<std::decay_t<T>, double>) {
            valueType = ValueType::DOUBLE;
        }
        return RawDataEncoder::ValueTypeEncoded(rawData_, false, valueType, 0);
    }

    virtual bool EncodeValue() override
    {
        return RawDataEncoder::FloatingNumberEncoded(rawData_, val_);
    }

    virtual bool AsString(std::string& ret) override
    {
        ret = std::to_string(val_);
        return true;
    }

    virtual bool AsDouble(double& ret) override
    {
        ret = static_cast<double>(val_);
        return true;
    }

    virtual DataCodedType GetDataCodedType() override
    {
        return DataCodedType::FLOATING;
    }

private:
    T val_;
};

template<typename T,
    std::enable_if_t<std::is_same_v<std::decay_t<T>, float> || std::is_same_v<std::decay_t<T>, double>>* = nullptr>
class FloatingNumberEncodedArrayParam : public EncodedParam {
public:
    FloatingNumberEncodedArrayParam(const std::string& key, const std::vector<T>& vals): EncodedParam(key)
    {
        unsigned int index = 0;
        for (auto item = vals.begin(); item != vals.end(); item++) {
            index++;
            if (index > MAX_ARRAY_SIZE) {
                break;
            }
            vals_.emplace_back(*item);
        }
    }

    virtual bool EncodeValueType() override
    {
        auto valueType = ValueType::UNKNOWN;
        if (std::is_same_v<std::decay_t<T>, float>) {
            valueType = ValueType::FLOAT;
        }
        if (std::is_same_v<std::decay_t<T>, double>) {
            valueType = ValueType::DOUBLE;
        }
        return RawDataEncoder::ValueTypeEncoded(rawData_, true, valueType, 0);
    }

    virtual bool EncodeValue() override
    {
        bool ret = RawDataEncoder::UnsignedVarintEncoded(rawData_, EncodeType::LENGTH_DELIMITED, vals_.size());
        for (auto item : vals_) {
            ret = ret && RawDataEncoder::FloatingNumberEncoded(rawData_, item);
        }
        return ret;
    }

    virtual bool AsDoubleVec(std::vector<double>& dest) override
    {
        for (auto item : vals_) {
            dest.emplace_back(static_cast<double>(item));
        }
        return true;
    }

    virtual DataCodedType GetDataCodedType() override
    {
        return DataCodedType::FLOATING_ARRAY;
    }

private:
    std::vector<T> vals_;
};

class StringEncodedParam : public EncodedParam {
public:
    StringEncodedParam(const std::string& key, const std::string& val): EncodedParam(key)
    {
        val_ = val;
    }

    virtual bool EncodeValueType() override
    {
        return RawDataEncoder::ValueTypeEncoded(rawData_, false, ValueType::STRING, 0);
    }

    virtual bool EncodeValue() override
    {
        return RawDataEncoder::StringValueEncoded(rawData_, val_);
    }

    bool AsString(std::string& ret) override
    {
        ret = val_;
        return true;
    }

    DataCodedType GetDataCodedType() override
    {
        return DataCodedType::DSTRING;
    }

private:
    std::string val_;
};

class StringEncodedArrayParam : public EncodedParam {
public:
    StringEncodedArrayParam(const std::string& key, const std::vector<std::string>& vals): EncodedParam(key)
    {
        unsigned int index = 0;
        for (auto item = vals.begin(); item != vals.end(); item++) {
            index++;
            if (index > MAX_ARRAY_SIZE) {
                break;
            }
            vals_.emplace_back(*item);
        }
    }

    virtual bool EncodeValueType() override
    {
        return RawDataEncoder::ValueTypeEncoded(rawData_, true, ValueType::STRING, 0);
    }

    virtual bool EncodeValue() override
    {
        bool ret = RawDataEncoder::UnsignedVarintEncoded(rawData_, EncodeType::LENGTH_DELIMITED, vals_.size());
        for (auto item : vals_) {
            ret = ret && RawDataEncoder::StringValueEncoded(rawData_, item);
        }
        return ret;
    }

    bool AsStringVec(std::vector<std::string>& dest) override
    {
        dest.assign(vals_.begin(), vals_.end());
        return true;
    }

    DataCodedType GetDataCodedType() override
    {
        return DataCodedType::DSTRING_ARRAY;
    }

private:
    std::vector<std::string> vals_;
};

extern template class FloatingNumberEncodedParam<double>;
extern template class SignedVarintEncodedParam<int64_t>;
extern template class UnsignedVarintEncodedParam<uint64_t>;
} // namespace EventRaw
} // namespace HiviewDFX
} // namespace OHOS

#endif // BASE_EVENT_RAW_ENCODE_INCLUDE_ENCODED_PARAM_H
