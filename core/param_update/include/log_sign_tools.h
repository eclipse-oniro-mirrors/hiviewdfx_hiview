/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef LOG_SIGN_TOOLS_H
#define LOG_SIGN_TOOLS_H

#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <string>
#include <sstream>
#include <unistd.h>

#include "param_const_common.h"

namespace OHOS {
namespace HiviewDFX {
class LogSignTools {
public:
    static bool VerifyFileSign(const std::string &pubKeyPath, const std::string &signPath,
        const std::string &digestPath);
    static std::string CalcFileSha256Digest(const std::string &fpath);

private:
    static void CalcBase64(uint8_t *input, uint32_t inputLen, std::string &encodedStr);
    static bool VerifyRsa(RSA *rsa, const std::string &digest, const std::string &sign);
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // LOG_SIGN_TOOLS_H
