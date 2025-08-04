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

#include "log_sign_tools.h"

#include "calc_fingerprint.h"
#include "file_util.h"
#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("Hiview-ParamUpdate");

namespace {
    constexpr int32_t BASE64_ENCODE_LEN_OF_EACH_GROUP_DATA = 4;
    constexpr int32_t BASE64_ENCODE_PACKET_LEN = 3;
    constexpr int BUFFER_SIZE = 4096;
}

bool LogSignTools::VerifyFileSign(const std::string &pubKeyPath, const std::string &signPath,
    const std::string &digestPath)
{
    if (!FileUtil::FileExists(pubKeyPath)) {
        HIVIEW_LOGE("pubKey file not exist");
        return false;
    }

    if (!FileUtil::FileExists(signPath)) {
        HIVIEW_LOGE("sign file not exist");
        return false;
    }

    if (!FileUtil::FileExists(digestPath)) {
        HIVIEW_LOGE("digest file not exist");
        return false;
    }

    std::string signStr;
    FileUtil::LoadStringFromFile(signPath, signStr);
    std::string digestStr;
    FileUtil::LoadStringFromFile(digestPath, digestStr);
    RSA *pubKey = RSA_new();
    bool verify = false;
    if (!(pubKey == nullptr || signStr.empty() || digestStr.empty())) {
        BIO *bio = BIO_new_file(pubKeyPath.c_str(), "r");
        if (PEM_read_bio_RSA_PUBKEY(bio, &pubKey, nullptr, nullptr) == nullptr) {
            HIVIEW_LOGE("get pubKey is failed.");
            BIO_free(bio);
            return false;
        }
        verify = VerifyRsa(pubKey, digestStr, signStr);
        BIO_free(bio);
    } else {
        HIVIEW_LOGE("pubKey or signStr or digestStr is error.");
    }
    RSA_free(pubKey);
    return verify;
}

int LogSignTools::CalcFileSha(const std::string& path, unsigned char *hash, size_t outLen)
{
    if (path.empty() || hash == nullptr || !FileUtil::IsLegalPath(path)) {
        HIVIEW_LOGE("file is invalid.");
        return EINVAL;
    }

    if (outLen < SHA256_DIGEST_LENGTH) {
        HIVIEW_LOGE("hash buf len error.");
        return ENOMEM;
    }

    FILE *fp = nullptr;
    fp = fopen(path.c_str(), "rb");
    if (fp == nullptr) {
        HIVIEW_LOGE("open file failed.");
        return errno; // if file not exist, errno will be ENOENT
    }

    size_t readSize;
    char fbuffer[BUFFER_SIZE] = {0};
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    while ((readSize = fread(fbuffer, 1, sizeof(fbuffer), fp))) {
        SHA256_Update(&ctx, (unsigned char *)fbuffer, readSize);
    }
    if (fclose(fp)) {
        HIVIEW_LOGE("fclose is failed");
    }
    fp = nullptr;
    SHA256_Final(hash, &ctx);
    return 0;
}

std::string LogSignTools::CalcFileSha256Digest(const std::string &fpath)
{
    unsigned char res[SHA256_DIGEST_LENGTH] = {0};
    if (CalcFileSha(fpath, res, SHA256_DIGEST_LENGTH) != 0) {
        HIVIEW_LOGE("CalcFileSha failed");
        return "";
    }
    std::string dist;
    CalcBase64(res, SHA256_DIGEST_LENGTH, dist);
    return dist;
}

void LogSignTools::CalcBase64(uint8_t *input, uint32_t inputLen, std::string &encodedStr)
{
    size_t base64Len = static_cast<size_t>(ceil(static_cast<long double>(inputLen) / BASE64_ENCODE_PACKET_LEN) *
    BASE64_ENCODE_LEN_OF_EACH_GROUP_DATA + 1);
    std::unique_ptr<unsigned char[]> base64Str = std::make_unique<unsigned char[]>(base64Len);
    size_t outLen = static_cast<size_t>(EVP_EncodeBlock(reinterpret_cast<uint8_t *>(base64Str.get()),
        input, inputLen));
    encodedStr = std::string(reinterpret_cast<char*>(base64Str.get()), outLen);
}

bool LogSignTools::VerifyRsa(RSA *pubKey, const std::string &digest, const std::string &sign)
{
    EVP_PKEY *evpKey = nullptr;
    EVP_MD_CTX *ctx = nullptr;
    evpKey = EVP_PKEY_new();
    if (evpKey == nullptr) {
        HIVIEW_LOGE("evpKey is nullptr");
        return false;
    }

    if (EVP_PKEY_set1_RSA(evpKey, pubKey) != 1) {
        HIVIEW_LOGE("set RSA failed.");
        EVP_PKEY_free(evpKey);
        return false;
    }

    ctx = EVP_MD_CTX_new();
    EVP_MD_CTX_init(ctx);
    if (ctx == nullptr) {
        HIVIEW_LOGE("ctx is nullptr.");
        EVP_PKEY_free(evpKey);
        return false;
    }

    if (EVP_VerifyInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        HIVIEW_LOGE("VerifyInit failed.");
        EVP_PKEY_free(evpKey);
        EVP_MD_CTX_free(ctx);
        return false;
    }

    if (EVP_VerifyUpdate(ctx, digest.c_str(), digest.size()) != 1) {
        HIVIEW_LOGE("VerifyUpdate failed.");
        EVP_PKEY_free(evpKey);
        EVP_MD_CTX_free(ctx);
        return false;
    }

    if (EVP_VerifyFinal(ctx, (unsigned char *)sign.c_str(), sign.size(), evpKey) != 1) {
        HIVIEW_LOGE("VerifyFinal failed.");
        EVP_PKEY_free(evpKey);
        EVP_MD_CTX_free(ctx);
        return false;
    }
    EVP_PKEY_free(evpKey);
    EVP_MD_CTX_free(ctx);
    return true;
}
}
}
