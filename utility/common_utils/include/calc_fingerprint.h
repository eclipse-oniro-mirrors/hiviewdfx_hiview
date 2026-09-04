/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef CALC_FINGERPRINT_H
#define CALC_FINGERPRINT_H

#include <string>
#include <openssl/sha.h>

namespace OHOS {
namespace HiviewDFX {
class CalcFingerprint {
public:
    CalcFingerprint() {};
    ~CalcFingerprint() {};

public:
    /*
    * CalcBufferSha: calculate a buffer sha1 hash for given buffer
    *
    * This function read the buffer and calc the sha1 value ,
    * The caller can pass a char hash_str[41] to get the hash string
    * The return value: 0 means successful,others mean failed.
    */
    static int CalcBufferSha(const std::string& buffer, size_t bufSize, char *hash, size_t len);

    /*
    * CalcBufferSha: calculate a source sha1 hash for given source
    *
    * This function read the source and calc the sha1 value ,
    * The caller can pass a char hash[41] to get the hash string
    * The return value: 0 means successful,others mean failed.
    */
    static int CalcBufferSha(unsigned char* source, size_t sourceLen, char *hash, size_t hashLen);

private:
    static int ConvertToString(const unsigned char hash[SHA256_DIGEST_LENGTH], char *outstr, size_t len);
};
}
}
#endif
