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

#include "trace_quota_config.h"

#include "cjson_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("TraceQuotaConfig");
const std::string TRACE_QUOTA_CONFIG_PATH = "/system/etc/hiview/trace_quota_config.json";
}

int64_t TraceQuotaConfig::GetTraceQuotaByCaller(const std::string& caller)
{
    auto root = CJsonUtil::ParseJsonRoot(TRACE_QUOTA_CONFIG_PATH);
    if (root == nullptr) {
        HIVIEW_LOGW("failed to parse config");
        return -1;
    }
    int64_t traceQuota = CJsonUtil::GetInt64MemberValue(root, caller, -1);
    if (traceQuota < 0) {
        HIVIEW_LOGW("failed to get quota for caller=%{public}s", caller.c_str());
    }
    cJSON_Delete(root);
    return traceQuota;
}
} // HiViewDFX
} // OHOS
