/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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

#include "rs_monitor_adapter.h"

#include "avcodec_video_monitor.h"

namespace OHOS {
namespace HiviewDFX {

RsMonitorAdapter::RsMonitorAdapter()
{
}

RsMonitorAdapter::~RsMonitorAdapter()
{
}

RsMonitorAdapter& RsMonitorAdapter::GetInstance()
{
    static RsMonitorAdapter instance;
    return instance;
}

void RsMonitorAdapter::VideoStart(const std::vector<uint64_t>& uniqueIdList,
    const std::vector<std::string>& surfaceNameList, const uint32_t fps, const uint64_t reportTime)
{
    AvcodecVideoMonitor::GetInstance().AvcodecVideoStart(uniqueIdList, surfaceNameList, fps, reportTime);
}

void RsMonitorAdapter::VideoStop(const std::vector<uint64_t>& uniqueIdList,
    const std::vector<std::string>& surfaceNameList, const uint32_t fps)
{
    AvcodecVideoMonitor::GetInstance().AvcodecVideoStop(uniqueIdList, surfaceNameList, fps);
}

void RsMonitorAdapter::VideoExpectionStop(const uint64_t uniqueId)
{
    AvcodecVideoMonitor::GetInstance().AvcodecVideoExpectionStop(uniqueId);
}

void RsMonitorAdapter::VideoCollectFinish()
{
    AvcodecVideoMonitor::GetInstance().AvcodecVideoCollectFinish();
}

void RsMonitorAdapter::VideoCollect(const uint64_t uniqueId, const uint32_t sequence)
{
    AvcodecVideoMonitor::GetInstance().AvcodecVideoCollect(uniqueId, sequence);
}

bool RsMonitorAdapter::VideoGet(uint64_t uniqueId)
{
    return AvcodecVideoMonitor::GetInstance().AvcodecVideoGet(uniqueId);
}

bool RsMonitorAdapter::VideoGetRecent()
{
    return AvcodecVideoMonitor::GetInstance().AvcodecVideoGetRecent();
}
}
}