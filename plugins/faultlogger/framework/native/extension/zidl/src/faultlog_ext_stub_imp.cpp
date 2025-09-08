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

#include "faultlog_ext_stub_imp.h"
#include "hiview_logger.h"

namespace OHOS {
namespace FaultLogExt {
DEFINE_LOG_TAG("HiView-FaultLogExt");

ErrCode FaultLogExtStubImp::OnFaultReportReady()
{
    HIVIEW_LOGI("begin OnFaultReportReady.");
    auto extension = extension_.lock();
    if (extension != nullptr) {
        extension->OnFaultReportReady();
        HIVIEW_LOGI("end OnFaultReportReady successfully.");
    }
    return ERR_OK;
}
} // namespace FaultLogExt
} // namespace OHOS