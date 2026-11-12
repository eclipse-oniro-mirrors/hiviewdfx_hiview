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

#include "thread_leak_config.h"

#include "fault_common.h"
#include "hiview_logger.h"
#include "dfr_util.h"
#include "leak_json_parser.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "ThreadLeakConfig");

using namespace std;

ThreadLeakConfig::ThreadLeakConfig() {}

ThreadLeakConfig::~ThreadLeakConfig() {}

void ThreadLeakConfig::UpdateValue(const cJSON *node)
{
    if (!DfrUtil::GetInstance().IsThreadLeakEnable()) {
        HIVIEW_LOGE("param has setted threadleak detecting disable.");
        return;
    }
    if (node == nullptr) {
        HIVIEW_LOGE("ThreadLeak node is null");
        return;
    }
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "enable", enable_);
    parser.OverWriteIfExist(node, "kill_enable", killEnable_);
    parser.OverWriteIfExist(node, "upload_interval", uploadInterval_);
    parser.OverWriteIfExist(node, "hungtask_check_thread", hungtaskThreadCheck_);
    parser.OverWriteIfExist(node, "profiler_time", profilerTime_);
    parser.OverWriteIfExist(node, "profiler_asyncStack_enable", asyncStackEnable_);
    UpdateThreshold(parser.GetSubNode("threshold", node));
    HIVIEW_LOGI("enable_: %{public}d, killEnable_:%{public}d, threshold_: %{public}d, smallThreshold_: %{public}d, "
        "uploadInterval_: %{public}d, profilerTime_: %{public}d, asyncStackEnable_: %{public}d", enable_, killEnable_,
        threshold_, smallThreshold_, uploadInterval_, profilerTime_, asyncStackEnable_);
}

void ThreadLeakConfig::UpdateThreshold(const cJSON *node)
{
    if (node == nullptr) {
        HIVIEW_LOGE("threshold node is null");
        return;
    }
    LeakJsonParser parser;
    parser.OverWriteIfExist(node, "default", threshold_);
    parser.OverWriteIfExist(node, "32bit", smallThreshold_);
}

void ThreadLeakConfig::Print()
{
    HIVIEW_LOGI("enable_: %{public}d, killEnable_:%{public}d, threshold_: %{public}d, smallThreshold_: %{public}d, "
        "uploadInterval_: %{public}d, profilerTime_: %{public}d, asyncStackEnable_: %{public}d", enable_, killEnable_,
        threshold_, smallThreshold_, uploadInterval_, profilerTime_, asyncStackEnable_);
}
} // namespace HiviewDFX
} // namespace OHOS
