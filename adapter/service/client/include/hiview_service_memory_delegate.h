/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_HIVIEWDFX_ADAPTER_SERVICE_IDL_INCLUDE_MEMORY_DELEGATE_H
#define OHOS_HIVIEWDFX_ADAPTER_SERVICE_IDL_INCLUDE_MEMORY_DELEGATE_H

#include "client/memory_collector_client.h"
#include "collect_result.h"
#include "request_ui_tree_callback_stub.h"

namespace OHOS {
namespace HiviewDFX {
class UiTreeCallbackStub : public RequestUiTreeCallbackStub {
public:
    UiTreeCallbackStub(std::shared_ptr<UCollectClient::RequestUiTreeCallback> callback) : callback_(callback) {}
    ErrCode OnUiTreeResponse(uint32_t retCode) override
    {
        if (callback_ != nullptr) {
            callback_->OnUiTreeResponse(retCode);
        }
        return 0;
    }

private:
    std::shared_ptr<UCollectClient::RequestUiTreeCallback> callback_;
};

class HiViewServiceMemoryDelegate {
public:
    static CollectResult<int32_t> SetAppResourceLimit(UCollectClient::MemoryCaller& memoryCaller);
    static CollectResult<UCollectClient::GraphicUsage> GetGraphicUsage();
    static CollectResult<int32_t> SetSplitMemoryValue(std::vector<UCollectClient::MemoryCaller>& memList);
    static CollectResult<int32_t> IsolateSubProcess(const std::string& packageName,
        int32_t mainProcPid, int32_t subProcPid);
    static CollectResult<int32_t> RequestUiTree(int32_t pid,
        std::shared_ptr<UCollectClient::RequestUiTreeCallback> callback);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_ADAPTER_SERVICE_IDL_INCLUDE_MEMORY_DELEGATE_H
