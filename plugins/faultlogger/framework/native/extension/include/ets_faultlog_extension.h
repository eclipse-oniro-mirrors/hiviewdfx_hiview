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

#ifndef ETS_FAULTLOG_EXTENSION_H
#define ETS_FAULTLOG_EXTENSION_H

#include "faultlog_extension.h"
#include "ets_runtime.h"
#include "ets_native_reference.h"
#include "runtime.h"

namespace OHOS {
namespace FaultLogExt {
class EtsFaultLogExtension : public FaultLogExtension {
public:
    explicit EtsFaultLogExtension(AbilityRuntime::Runtime& runtime)
        : etsRuntime_(static_cast<AbilityRuntime::ETSRuntime&>(runtime)) {}

    /**
     * @brief Create EtsFaultLogExtension.
     *
     * @param runtime The runtime.
     * @return The JsFaultLogExtension instance.
     */
    static EtsFaultLogExtension* Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime);

    /**
     * @brief The Init.
     *
     * @param record The record info.
     * @param application The application info.
     * @param handler The handler.
     * @param token The remmote object token.
     */
    void Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord> &record,
              const std::shared_ptr<AppExecFwk::OHOSApplication> &application,
              std::shared_ptr<AppExecFwk::AbilityHandler> &handler,
              const sptr<IRemoteObject> &token) override;

    /**
     * @brief The OnConnect callback.
     *
     * @param want The want.
     * @return Remote object.
     */
    sptr<IRemoteObject> OnConnect(const AAFwk::Want &want) override;

    /**
     * @brief The OnDisconnect callback.
     *
     * @param want The want.
     */
    void OnDisconnect(const AAFwk::Want &want) override;

    /**
     * @brief The OnFaultReportReady callback.
     */
    void OnFaultReportReady() override;

private:
    bool CallObjectMethod(const char* name, const char* signature, ...);
    void BindContext(ani_env* env, std::shared_ptr<AAFwk::Want> want);

    AbilityRuntime::ETSRuntime &etsRuntime_;
    std::unique_ptr<AppExecFwk::ETSNativeReference> etsObj_;
    std::shared_ptr<AppExecFwk::ETSNativeReference> shellContextRef_;
};
} // namespace FaultLogExt
} // namespace OHOS

#endif // ETS_FAULTLOG_EXTENSION_H
