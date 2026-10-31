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
#include "fault_config_manager.h"

#include <fstream>
#include "fault_common.h"
#include "dfr_util.h"
#include "hiview_logger.h"
#include "config_policy_utils.h"
#include "waiting_kill_list.h"
#include "old_log_param_config.h"
#include "sysload_config_parser.h"
#include "fd_leak_config.h"
#include "native_leak_config.h"
#include "kernel_leak_config.h"
#include "thread_leak_config.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FaultConfigManager");

using namespace std;

namespace {
// /system/variant/phone/base/etc/reliability/leak_detector_config.json
constexpr const char *LEAK_DETECTOR_CONFIG_PATH = "etc/reliability/leak_detector_config.json";
constexpr const char *DEFAULT_QUOTA = "Quota_32G";
};

FaultConfigManager::FaultConfigManager() {}

FaultConfigManager::~FaultConfigManager() {}

void FaultConfigManager::ParseConfigByVersion(const cJSON *root, const string &version)
{
    LeakJsonParser parser;
    HIVIEW_LOGI("parsing version: %{public}s", version.c_str());
    auto versionConfig = parser.GetSubNode(version, root);
    if (versionConfig == nullptr) {
        HIVIEW_LOGW("config version: %{public}s is null", version.c_str());
        return;
    }
    FdLeakConfig::GetInstance().UpdateValue(parser.GetSubNode("FileDescriptorLeak", versionConfig));
    KernelLeakConfig::GetInstance().UpdatePolicy(parser.GetSubNode("KernelLeak", versionConfig));
    ThreadLeakConfig::GetInstance().UpdateValue(parser.GetSubNode("ThreadLeak", versionConfig));
    NativeLeakConfig::GetInstance().ParseNativeConfig(parser.GetSubNode("NativeLeak", versionConfig));
    WaitingKillList::GetInstance().DoParseConfig(parser.GetSubNode("WaitingKillList", versionConfig));
    SysloadConfigParser::GetInstance().DoParseSysload(parser.GetSubNode("SysLoad", versionConfig));
    OldLogParamConfig::GetInstance().ParseConfig(parser.GetSubNode("OldLogParamConfig", versionConfig));

    ParseQuota(versionConfig);
}

void FaultConfigManager::ParseConfigFile()
{
    char buf[MAX_PATH_LEN];
    char* path = GetOneCfgFile(LEAK_DETECTOR_CONFIG_PATH, buf, MAX_PATH_LEN);
    if (!(path != nullptr && *path != '\0')) {
        HIVIEW_LOGE("GetOneCfgFile leak_detector_config.json is NULL.\n");
        return;
    }

    LeakJsonParser parser;
    if (!(parser.InitJsonFile(path))) {
        HIVIEW_LOGE("failed to init json file, please check json file style.\n");
        return;
    }

    auto root = parser.GetRootNode();
    if (!(root != nullptr)) {
        HIVIEW_LOGE("root node is null.");
        return;
    }

    ParseConfigByVersion(root, "Commercial");
    if (DfrUtil::GetInstance().IsBetaOrDevelopMode()) {
        cout << "beta version" << endl;
        ParseConfigByVersion(root, "Beta");
    } else {
        cout << "commercial version" << endl;
    }
    FdLeakConfig::GetInstance().Print();
    KernelLeakConfig::GetInstance().Print();
    ThreadLeakConfig::GetInstance().Print();
}

void FaultConfigManager::ParseQuota(const cJSON *node)
{
    const cJSON *quotaNode = GetQuotaNode(node);
    if (quotaNode == nullptr) {
        HIVIEW_LOGW("quota node is null");
        return;
    }
    LeakJsonParser parser;
    KernelLeakConfig::GetInstance().UpdateQuota(parser.GetSubNode("kernel_type", quotaNode));
}

const cJSON *FaultConfigManager::GetQuotaNode(const cJSON *node)
{
    int ramSize = DfrUtil::GetInstance().GetRamSize();
    string targetNode = "Quota_" + to_string(ramSize) + "G";
    LeakJsonParser parser;
    if (!parser.IsKeyExist(targetNode, node)) {
        return parser.GetSubNode(DEFAULT_QUOTA, node);
    }
    return parser.GetSubNode(targetNode, node);
}
} // HiviewDFX END
} // OHOS END
