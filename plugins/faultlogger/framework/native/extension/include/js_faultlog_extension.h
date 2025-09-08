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

#ifndef JS_FAULTLOG_EXTENSION_H
#define JS_FAULTLOG_EXTENSION_H

#include "faultlog_extension.h"
#include "js_runtime.h"

namespace OHOS {
namespace FaultLogExt {
class JsFaultLogExtension : public FaultLogExtension {
public:
    explicit JsFaultLogExtension(AbilityRuntime::JsRuntime &jsRuntime);
    ~JsFaultLogExtension() override;

    /**
     * @brief Create JsFaultLogExtension.
     *
     * @param runtime The runtime.
     * @return The JsFaultLogExtension instance.
     */
    static JsFaultLogExtension* Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime);

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
     * @brief The OnStart callback.
     *
     * @param want The want.
     */
    void OnStart(const AAFwk::Want &want) override;

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
     * @brief The OnStop callback.
     */
    void OnStop() override;

    /**
     * @brief The OnFaultReportReady callback.
     */
    void OnFaultReportReady() override;
private:
    void BindContext(napi_env env, napi_value obj);
    void GetSrcPath(std::string &srcPath);
    AbilityRuntime::JsRuntime &jsRuntime_;
    std::unique_ptr<NativeReference> jsObj_;
    std::shared_ptr<NativeReference> shellContextRef_ = nullptr;
};
}  // namespace FaultLogExt
}  // namespace OHOS
#endif  // JS_FAULTLOG_EXTENSION_H