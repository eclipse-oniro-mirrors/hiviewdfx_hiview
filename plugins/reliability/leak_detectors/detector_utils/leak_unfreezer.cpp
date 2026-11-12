/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// Resolves the process unfreezer from libleakdetectors_ext.z.so at runtime.
// The .so is a closed-source vendor part that depends on the vendored
// efficiency_manager part (SuspendManagerClient), so it must not be linked
// directly by this open-source library. When the .so or the symbol is absent,
// unfreezing degrades to a no-op.

#include "leak_unfreezer.h"

#include <dlfcn.h>
#include <mutex>

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "LeakUnfreezer");

namespace {
constexpr const char* const LEAKDETECTORS_EXT_SO = "libleakdetectors_ext.z.so";
constexpr const char* const UNFREEZE_LEAK_PROCESS_SYMBOL = "UnfreezeLeakProcess";
constexpr int32_t UNFREEZE_SUCCESS = 0;
constexpr int32_t UNFREEZE_FAILED = -1;

using UnfreezeLeakProcessFunc = int32_t (*)(int32_t, const char *, const char *);

UnfreezeLeakProcessFunc LoadUnfreezer()
{
    static std::once_flag once;
    static UnfreezeLeakProcessFunc unfreezer = nullptr;
    std::call_once(once, []() {
        void *handle = dlopen(LEAKDETECTORS_EXT_SO, RTLD_LAZY);
        if (handle == nullptr) {
            HIVIEW_LOGI("dlopen %{public}s failed: %{public}s, unfreeze disabled",
                LEAKDETECTORS_EXT_SO, dlerror());
            return;
        }
        unfreezer = reinterpret_cast<UnfreezeLeakProcessFunc>(
            dlsym(handle, UNFREEZE_LEAK_PROCESS_SYMBOL));
        if (unfreezer == nullptr) {
            HIVIEW_LOGE("dlsym %{public}s failed: %{public}s", UNFREEZE_LEAK_PROCESS_SYMBOL, dlerror());
            dlclose(handle);
        }
    });
    return unfreezer;
}
} // namespace

int32_t UnfreezeLeakProcessByExt(int32_t uid, const std::string &bundleName, const std::string &reason)
{
    UnfreezeLeakProcessFunc unfreezer = LoadUnfreezer();
    if (unfreezer == nullptr) {
        return UNFREEZE_SUCCESS;
    }
    if (bundleName.empty() || reason.empty()) {
        HIVIEW_LOGW("invalid bundleName or reason, uid: %{public}d", uid);
        return UNFREEZE_FAILED;
    }
    return unfreezer(uid, bundleName.c_str(), reason.c_str());
}

} // namespace HiviewDFX
} // namespace OHOS
