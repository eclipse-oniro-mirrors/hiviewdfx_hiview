/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#ifndef HIVIEW_PLUGIN_EVENT_FIELD_VALIDATOR_H
#define HIVIEW_PLUGIN_EVENT_FIELD_VALIDATOR_H

#include <memory>
#include <string>

#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
class EventFieldValidator {
public:
    static bool IsSenderAllowed(const std::string& domain, const std::string& eventName, int32_t senderUid);
    static bool IsTrustedSource(const std::shared_ptr<SysEvent>& event);
    static bool ValidateEvent(const std::shared_ptr<SysEvent>& event);
    static bool IsAcceptedReadPath(const std::string& path);
    static bool ContainPathTraversal(const std::string& path);

private:
    static bool ValidateUntrustedEvent(const std::shared_ptr<SysEvent>& event);
    static bool CheckIdentityFields(const std::shared_ptr<SysEvent>& event);
    static bool CheckNumericFields(const std::shared_ptr<SysEvent>& event);
    static bool CheckNameFields(const std::shared_ptr<SysEvent>& event);
    static bool CheckPathFields(const std::shared_ptr<SysEvent>& event);
    static bool CheckContentFields(const std::shared_ptr<SysEvent>& event);
    static bool CheckStackAndBinderPaths(const std::shared_ptr<SysEvent>& event);
    // binder info from untrusted senders is never trusted: drop the externally
    // supplied BINDER_INFO/HICOLLIE_BINDER_INFO fields; peer binder context for
    // such events is collected by hiview itself (BinderCatcher reads
    // /proc/transaction_proc) instead of being taken from the event
    static bool StripUntrustedBinderInfo(const std::shared_ptr<SysEvent>& event);
    static bool IsDecimalValue(const std::string& value);
    static bool ToInt32Value(const std::string& value, int32_t& out);
    static bool IsSafeName(const std::string& value);
    static bool IsAcceptedPath(const std::string& path, int32_t senderUid);
    static bool IsOwnerOfPath(const std::string& path, int32_t senderUid);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEW_PLUGIN_EVENT_FIELD_VALIDATOR_H
