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

#include "dfx_bundle_util.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <file_util.h>
#include <regex>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time_util.h>
#include <unistd.h>

#include "bundle_mgr_client.h"
#include "climits"
#include "event_publish.h"
#include "hilog/log.h"
#include "hisysevent.h"
#include "securec.h"
#include "string_ex.h"
#include "parameters.h"
#include "parameter_ex.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D11

#undef LOG_TAG
#define LOG_TAG "DfxBundleUtil"

namespace OHOS {
namespace HiviewDFX {
using namespace OHOS::AppExecFwk;
bool IsNameValid(const std::string& name, const std::string& sep, bool canEmpty)
{
    std::vector<std::string> nameVec;
    SplitStr(name, sep, nameVec, canEmpty, false);
    std::regex re("^[a-zA-Z0-9][a-zA-Z0-9_]{0,127}$");
    for (auto const& splitName : nameVec) {
        if (!std::regex_match(splitName, re)) {
            HILOG_INFO(LOG_CORE, "Invalid splitName:%{public}s", splitName.c_str());
            return false;
        }
    }
    return true;
}

bool IsModuleNameValid(const std::string& name)
{
    if (name.empty() || name.size() > MAX_NAME_LENGTH) {
        HILOG_INFO(LOG_CORE, "invalid log name.");
        return false;
    }

    if (name.find("/") != std::string::npos || name.find(".") == std::string::npos) {
        std::string path = name.substr(1); // may skip first .
        path.erase(path.find_last_not_of(" \n\r\t") + 1);
        HILOG_INFO(LOG_CORE, "module name:%{public}s", name.c_str());
        return IsNameValid(path, "/", false);
    }

    return IsNameValid(name, ".", true);
}

std::string GetApplicationNameById(int32_t uid)
{
    std::string bundleName;
    AppExecFwk::BundleMgrClient client;
    if (client.GetNameForUid(uid, bundleName) != ERR_OK) {
        HILOG_WARN(LOG_CORE, "Failed to query bundleName from bms, uid:%{public}d.", uid);
    } else {
        HILOG_INFO(LOG_CORE, "bundleName of uid:%{public}d is %{public}s", uid, bundleName.c_str());
    }
    return bundleName;
}

bool GetDfxBundleInfo(const std::string& bundleName, DfxBundleInfo& bundleInfo)
{
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT,
                              info, Constants::ALL_USERID)) {
        HILOG_WARN(LOG_CORE, "Failed to query BundleInfo from bms, bundle:%{public}s.", bundleName.c_str());
        return false;
    } else {
        HILOG_INFO(LOG_CORE, "The version of %{public}s is %{public}s", bundleName.c_str(),
            info.versionName.c_str());
    }
    bundleInfo.isPreInstalled = info.isPreInstallApp;
    bundleInfo.versionName = info.versionName;
    bundleInfo.versionCode = info.versionCode;
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
