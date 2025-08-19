/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#ifndef OHOS_HIVIEWDFX_XPERF_SERVICE_INTERFACES_H
#define OHOS_HIVIEWDFX_XPERF_SERVICE_INTERFACES_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace HiviewDFX {

class __attribute__((visibility("default"))) XperfServiceInterfaces {
public:
    static XperfServiceInterfaces &GetInstance();
    XperfServiceInterfaces(const XperfServiceInterfaces &) = delete;
    void operator=(const XperfServiceInterfaces &) = delete;

    void ReportSurfaceInfo(int32_t pid, std::string bundleName, int64_t uniqueId, std::string surfaceName);

private:
    XperfServiceInterfaces();
    ~XperfServiceInterfaces() noexcept;
};

}
}
#endif
