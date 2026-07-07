/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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
#include "app_usage_event.h"

#include "hisysevent_util.h"
#include "hiview_logger.h"
#include "usage_event_common.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("AppUsageEvent");
using namespace AppUsageEventSpace;

AppUsageEvent::AppUsageEvent(const std::string &name, HiSysEvent::EventType type)
    : LoggerEvent(name, type)
{
    this->paramMap_ = {
        {std::string(KEY_OF_PACKAGE), std::string(DEFAULT_STRING)},
        {std::string(KEY_OF_VERSION), std::string(DEFAULT_STRING)},
        {std::string(KEY_OF_USAGE), DEFAULT_UINT64},
        {std::string(KEY_OF_DATE), std::string(DEFAULT_STRING)},
        {std::string(KEY_OF_START_NUM), DEFAULT_UINT32}
    };
}

void AppUsageEvent::Report()
{
    ReportDFX();
    ReportDFXUE();
}

void AppUsageEvent::ReportDFX()
{
    ReportEvent(false);
}

void AppUsageEvent::ReportDFXUE()
{
    ReportEvent(true);
}

void AppUsageEvent::ReportEvent(bool isReportUeEvent)
{
    std::string packageName = this->paramMap_[KEY_OF_PACKAGE].GetString();
    std::string version = this->paramMap_[KEY_OF_VERSION].GetString();
    std::string date = this->paramMap_[KEY_OF_DATE].GetString();
    HiSysEventParam params[] = {
        BUILD_PARAM(KEY_OF_PACKAGE_LITERAL, HISYSEVENT_STRING, s, PARAM_STR(packageName)),
        BUILD_PARAM(KEY_OF_VERSION_LITERAL, HISYSEVENT_STRING, s, PARAM_STR(version)),
        BUILD_PARAM(KEY_OF_USAGE_LITERAL, HISYSEVENT_UINT64, ui64, this->paramMap_[KEY_OF_USAGE].GetUint64()),
        BUILD_PARAM(KEY_OF_DATE_LITERAL, HISYSEVENT_STRING, s, PARAM_STR(date)),
        BUILD_PARAM(KEY_OF_START_NUM_LITERAL, HISYSEVENT_UINT32, ui32, this->paramMap_[KEY_OF_START_NUM].GetUint32()),
    };
    int ret = 0;
    if (isReportUeEvent) {
        ret = OH_HiSysEvent_Write(DomainSpace::HIVIEWDFX_UE_DOMAIN, this->eventName_.c_str(),
            TranslateEventType(this->eventType_), params, sizeof(params) / sizeof(HiSysEventParam));
    } else {
        ret = OH_HiSysEvent_Write(HiSysEvent::Domain::HIVIEWDFX, this->eventName_.c_str(),
            TranslateEventType(this->eventType_), params, sizeof(params) / sizeof(HiSysEventParam));
    }
    if (ret != 0) {
        HIVIEW_LOGW("failed to report app usage event, ret=%{public}d", ret);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
