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

#ifndef FAULTLOG_EXTENSION_H
#define FAULTLOG_EXTENSION_H

#include "ability_local_record.h"
#include "ohos_application.h"
#include "ability_handler.h"
#include "runtime.h"
#include "extension_base.h"
#include "faultlog_extension_context.h"

namespace OHOS {
namespace FaultLogExt {
class FaultLogExtension : public AbilityRuntime::ExtensionBase<FaultLogExtensionContext> {
public:
    FaultLogExtension() = default;
    virtual ~FaultLogExtension() = default;

    /**
     * @brief Create and init context.
     *
     * @param record The record info.
     * @param application The application info.
     * @param handler The handler.
     * @param token The remote object token.
     * @return The context
     */
    std::shared_ptr<FaultLogExtensionContext> CreateAndInitContext(
        const std::shared_ptr<AppExecFwk::AbilityLocalRecord>& record,
        const std::shared_ptr<AppExecFwk::OHOSApplication>& application,
        std::shared_ptr<AppExecFwk::AbilityHandler>& handler,
        const sptr<IRemoteObject>& token) override;

    /**
     * @brief The Init.
     *
     * @param record The record.
     * @param application The application.
     * @param handler The handler.
     * @param token The remote object token.
     */
    void Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord>& record,
        const std::shared_ptr<AppExecFwk::OHOSApplication>& application,
        std::shared_ptr<AppExecFwk::AbilityHandler>& handler,
        const sptr<IRemoteObject>& token) override;

    /**
     * @brief Create FaultLogExtension.
     *
     * @param runtime The runtime.
     * @return The FaultLogExtension instance.
     */
    static FaultLogExtension* Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime);

    /**
     * @brief The OnFaultReportReady callback.
     */
    virtual void OnFaultReportReady();
};
}  // namespace FaultLogExt
}  // namespace OHOS
#endif  // FAULTLOG_EXTENSION_H
