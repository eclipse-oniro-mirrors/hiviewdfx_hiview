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

#ifndef DFR_LEAK_UNFREEZER_H
#define DFR_LEAK_UNFREEZER_H

#include <string>

namespace OHOS {
namespace HiviewDFX {

// Unfreezes (thaws) an application through the suspend manager. The real
// implementation (SuspendManagerClient::ThawOneApplication, which depends on
// the vendored efficiency_manager part) is exported by the closed-source
// libleakdetectors_ext.z.so and resolved through dlopen/dlsym, so this
// open-source library keeps no build-time dependency on efficiency_manager.
// When the .so or the symbol is absent (e.g. pure OHOS products), the call
// degrades to a no-op.
int32_t UnfreezeLeakProcessByExt(int32_t uid, const std::string &bundleName, const std::string &reason);

} // namespace HiviewDFX
} // namespace OHOS
#endif // DFR_LEAK_UNFREEZER_H
