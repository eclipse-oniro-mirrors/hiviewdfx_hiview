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

#ifndef HIVIEW_NAPI_ERR_CODE_H
#define HIVIEW_NAPI_ERR_CODE_H

namespace OHOS {
namespace HiviewDFX {
namespace HiviewNapiErrCode {
constexpr int32_t ERR_PERMISSION_CHECK = 201;
constexpr int32_t ERR_NON_SYS_APP_PERMISSION = 202;
constexpr int32_t ERR_PARAM_CHECK = 401;

constexpr int32_t ERR_DEFAULT = -1;
constexpr int32_t ERR_SOURCE_FILE_NOT_EXIST = 21300001;

// inner errcode
constexpr int32_t ERR_INNER_INVALID_LOGTYPE = 10001;
constexpr int32_t ERR_INNER_READ_ONLY = 10002;
} // namespace HiviewNapiErrCode

namespace TraceErrCode {
constexpr int32_t ERR_OK = 0;
constexpr int32_t ERR_PERMISSION_CHECK = 201;

constexpr int32_t ERR_GET_HIVIEW_SERVICE = -10;
constexpr int32_t ERR_READ_MSG_PARCEL = -11;
constexpr int32_t ERR_WRITE_MSG_PARCEL = -12;
constexpr int32_t ERR_WRITE_INTERFACE_TOKEN = -13;
constexpr int32_t ERR_SEND_REQUEST = -14;
constexpr int32_t ERR_INVALID_HANDLER = -15;
}

namespace UiTreeErrCode {
constexpr int32_t ERR_OK = 0;
constexpr int32_t ERR_RENDER_SERIVICE_CHECK = 201;
constexpr int32_t ERR_GET_HIVIEW_SERVICE = -10;
}
} // namespace HiviewDFX
} // namespace OHOS
#endif