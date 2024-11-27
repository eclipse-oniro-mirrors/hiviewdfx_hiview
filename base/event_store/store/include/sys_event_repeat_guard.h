/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef SYS_EVENT_REPEAT_GUARD_H
#define SYS_EVENT_REPEAT_GUARD_H

#include "sys_event.h"
#include "sys_event_doc_reader.h"

namespace OHOS {
namespace HiviewDFX {
template<class T>
struct EventRawDataInfo {
    EventRawDataInfo(uint8_t* rawData)
    {
        Parse(rawData);
    }

    void Parse(uint8_t* rawData)
    {
        /* rawData formate:
            dataSize|header|TraceInfo(when isTraceOpened == 1)
         */
        dataSize = *(reinterpret_cast<int32_t*>(rawData));
        dataPos = sizeof(int32_t);
        head = *(reinterpret_cast<T*>(rawData + dataPos));
        dataPos += sizeof(T);
        if (head.isTraceOpened == 1) {
            dataPos += sizeof(EventRaw::TraceInfo);
        }
        dataPos += sizeof(int32_t);
    };

    bool IsLogPacked()
    {
        constexpr uint8_t logLowBit = 5;
        constexpr uint8_t logNotPacked = 1;
        return (head.log >> logLowBit) != logNotPacked;
    }
    T head;
    int32_t dataPos = 0;
    int32_t dataSize = 0;
};

class SysEventRepeatGuard {
public:
    static void Check(SysEvent& event);
    static void RegisterListeningUeSwitch();
    static void UnregisterListeningUeSwitch();
private:
    static bool IsEventRepeat(SysEvent& event);
    static int64_t GetMinValidTime();
    static bool GetEventUniqueId(SysEvent& event, std::string& uniqueId);
};
} // HiviewDFX
} // OHOS

#endif // SYS_EVENT_REPEAT_GUARD_H