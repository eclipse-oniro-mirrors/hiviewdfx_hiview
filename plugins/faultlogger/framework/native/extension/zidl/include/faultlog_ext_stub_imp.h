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

#ifndef OHOS_ZIDL_FAULTLOG_EXT_STUB_IMP_H
#define OHOS_ZIDL_FAULTLOG_EXT_STUB_IMP_H

#include "js_faultlog_extension.h"
#include "fault_log_ext_stub.h"

namespace OHOS {
namespace FaultLogExt {
class FaultLogExtStubImp : public FaultLogExtStub {
public:
    explicit FaultLogExtStubImp(const std::shared_ptr<JsFaultLogExtension>& extension)
        : extension_(extension) {}

    ~FaultLogExtStubImp() override {}

    /**
     * @brief The OnFaultReportReady callback.
     */
    ErrCode OnFaultReportReady() override;
private:
    std::weak_ptr<JsFaultLogExtension> extension_;
};
} // namespace FaultLogExt
} // namespace OHOS
#endif // OHOS_ZIDL_FAULTLOG_EXT_STUB_IMP_H

