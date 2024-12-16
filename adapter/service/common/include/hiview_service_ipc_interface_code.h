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

#ifndef OHOS_HIVIEWDFX_HIVIEW_SERVICE_IPC_INTERFACE_CODE_H
#define OHOS_HIVIEWDFX_HIVIEW_SERVICE_IPC_INTERFACE_CODE_H

/* SAID: 1201 */
namespace OHOS {
namespace HiviewDFX {
enum class HiviewServiceInterfaceCode {
    HIVIEW_SERVICE_ID_LIST = 1001,
    HIVIEW_SERVICE_ID_COPY = 1002,
    HIVIEW_SERVICE_ID_MOVE = 1003,
    HIVIEW_SERVICE_ID_REMOVE = 1004,
    HIVIEW_SERVICE_ID_OPEN_SNAPSHOT_TRACE = 1005,
    HIVIEW_SERVICE_ID_DUMP_SNAPSHOT_TRACE = 1006,
    HIVIEW_SERVICE_ID_OPEN_RECORDING_TRACE = 1007,
    HIVIEW_SERVICE_ID_RECORDING_TRACE_ON = 1008,
    HIVIEW_SERVICE_ID_RECORDING_TRACE_OFF = 1009,
    HIVIEW_SERVICE_ID_CLOSE_TRACE = 1010,
    HIVIEW_SERVICE_ID_RECOVER_TRACE = 1011,
    HIVIEW_SERVICE_ID_GET_SYSTEM_CPU_USAGE = 1012,
    HIVIEW_SERVICE_ID_GET_APP_TRACE = 1013,
    HIVIEW_SERVICE_ID_SET_APPRESOURCE_LIMIT = 1014,
    HIVIEW_SERVICE_ID_GET_GRAPHIC_USAGE = 1015,
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // OHOS_HIVIEWDFX_HIVIEW_SERVICE_IPC_INTERFACE_CODE_H
