/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "xperf_event_builder.h"
#include <cstdlib>
#include <cstring>
#include <securec.h>
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {

DEFINE_LOG_LABEL(0xD002D66, "Hiview-XPerformance");

XperfEventBuilder::XperfEventBuilder()
{
}

XperfEventBuilder::~XperfEventBuilder()
{
}

void XperfEventBuilder::SetParamName(HiSysEventParam& param, const char* name)
{
    int ret = strcpy_s(param.name, sizeof(param.name), name);
    if (ret != 0) {
        HIVIEW_LOGE("XperfEventBuilder::SetParamName error, err is %{public}d.", ret);
    }
}

XperfEventBuilder& XperfEventBuilder::EventName(const std::string& eventName)
{
    this->evtName = eventName;
    return *this;
}

XperfEventBuilder& XperfEventBuilder::EventType(const HiSysEventEventType& eventType)
{
    this->evtType = eventType;
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, bool value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_BOOL,
        .v = { .b = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, int8_t value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_INT8,
        .v = { .i8 = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, uint8_t value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_UINT8,
        .v = { .ui8 = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, int16_t value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_INT16,
        .v = { .i16 = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, uint16_t value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_UINT16,
        .v = { .ui16 = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, int32_t value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_INT32,
        .v = { .i32 = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, uint32_t value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_UINT32,
        .v = { .ui32 = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, int64_t value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_INT64,
        .v = { .i64 = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, uint64_t value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_UINT64,
        .v = { .ui64 = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, float value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_FLOAT,
        .v = { .f= value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, double value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_DOUBLE,
        .v = { .d = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, const std::vector<uint16_t>& value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_UINT16_ARRAY,
        .v = { .array = static_cast<void*>(const_cast<uint16_t*>(value.data())) },
        .arraySize = value.size(),
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, const std::string value)
{
    int len = static_cast<int>(value.length() + 1);
    char* cStr = new char[len];
    int ret = strcpy_s(cStr, len, value.c_str());
    if (ret != 0) {
        HIVIEW_LOGE("XperfEventBuilder::InsertParam string error, err is %{public}d.", ret);
    }
    return this->Param(name, cStr);
}

XperfEventBuilder& XperfEventBuilder::Param(const char* name, char* value)
{
    HiSysEventParam param = {
        .t = HISYSEVENT_STRING,
        .v = { .s = value },
        .arraySize = 0,
    };
    SetParamName(param, name);
    paramList.push_back(param);
    return *this;
}

OHOS::HiviewDFX::XperfEvent XperfEventBuilder::Build()
{
    if (!Check()) {
        OHOS::HiviewDFX::XperfEvent defaultEvent;
        return defaultEvent;
    }
    int size = static_cast<int>(paramList.size());
    OHOS::HiviewDFX::XperfEvent event(this->evtName, this->evtType, size);
    for (int i = 0; i < size; i++) {
        event.params[i] = paramList.at(i);
    }
    return event;
}

bool XperfEventBuilder::Check()
{
    if (evtName.empty()) {
        HIVIEW_LOGE("[XperfEventBuilder::Check] evtName is empty");
        Release();
        return false;
    }
    if (paramList.size() == 0) {
        HIVIEW_LOGE("[XperfEventBuilder::Check] paramList is empty");
        return false;
    }
    return true;
}

void XperfEventBuilder::Release()
{
    HIVIEW_LOGD("[XperfEventBuilder::Release]");
    for (auto it = paramList.begin(); it != paramList.end(); it++) {
        if (((*it).t == HISYSEVENT_STRING) && ((*it).v.s != nullptr)) {
            delete[] (*it).v.s;
            (*it).v.s = nullptr;
        }
    }
}

}
}