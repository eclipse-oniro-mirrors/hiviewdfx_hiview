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
#include "fold_app_usage_event.h"

#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
using namespace FoldAppUsageEventSpace;

FoldAppUsageEvent::FoldAppUsageEvent(const std::string &name, HiSysEvent::EventType type)
    : LoggerEvent(name, type)
{
    this->paramMap_ = {
        {KEY_OF_PACKAGE, DEFAULT_STRING}, {KEY_OF_VERSION, DEFAULT_STRING},
        {KEY_OF_FOLD_VER_USAGE, DEFAULT_UINT32}, {KEY_OF_FOLD_HOR_USAGE, DEFAULT_UINT32},
        {KEY_OF_EXPD_VER_USAGE, DEFAULT_UINT32}, {KEY_OF_EXPD_HOR_USAGE, DEFAULT_UINT32},
        {KEY_OF_USAGE, DEFAULT_UINT32}, {KEY_OF_DATE, DEFAULT_STRING},
        {KEY_OF_START_NUM, DEFAULT_UINT32}
    };
}

void FoldAppUsageEvent::Report()
{
    HiSysEventWrite(HiSysEvent::Domain::HIVIEWDFX, this->eventName_, this->eventType_,
        KEY_OF_PACKAGE, this->paramMap_[KEY_OF_PACKAGE].GetString(),
        KEY_OF_VERSION, this->paramMap_[KEY_OF_VERSION].GetString(),
        KEY_OF_FOLD_VER_USAGE, this->paramMap_[KEY_OF_FOLD_VER_USAGE].GetUint32(),
        KEY_OF_FOLD_HOR_USAGE, this->paramMap_[KEY_OF_FOLD_HOR_USAGE].GetUint32(),
        KEY_OF_EXPD_VER_USAGE, this->paramMap_[KEY_OF_EXPD_VER_USAGE].GetUint32(),
        KEY_OF_EXPD_HOR_USAGE, this->paramMap_[KEY_OF_EXPD_HOR_USAGE].GetUint32(),
        KEY_OF_USAGE, this->paramMap_[KEY_OF_USAGE].GetUint32(),
        KEY_OF_DATE, this->paramMap_[KEY_OF_DATE].GetString(),
        KEY_OF_START_NUM, this->paramMap_[KEY_OF_START_NUM].GetUint32());
}
} // namespace HiviewDFX
} // namespace OHOS
