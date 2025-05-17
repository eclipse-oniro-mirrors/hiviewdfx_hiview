/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HIVIEWFAULTLOGGER_IPC_INTERFACE_CODE_H
#define HIVIEWFAULTLOGGER_IPC_INTERFACE_CODE_H

/* SAID: 1202 */
namespace OHOS {
namespace HiviewDFX {
    enum class FaultLoggerServiceInterfaceCode {
        ADD_FAULTLOG = 0,
        QUERY_SELF_FAULTLOG,
        ENABLE_GWP_ASAN_GRAYSALE,
        DISABLE_GWP_ASAN_GRAYSALE,
        GET_GWP_ASAN_GRAYSALE,
        DESTROY,
    };

    enum class FaultLogQueryResultInterfaceCode {
        HASNEXT = 0,
        GETNEXT,
    };
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIVIEWFAULTLOGGER_IPC_INTERFACE_CODE_H