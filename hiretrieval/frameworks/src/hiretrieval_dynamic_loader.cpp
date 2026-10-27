/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "hiretrieval_dynamic_loader.h"

#include <cerrno>
#include <functional>
#include <string>

#include "hilog/log.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D10

#undef LOG_TAG
#define LOG_TAG "HIRETRIEVAL_DLL"

namespace OHOS::HiviewDFX {
namespace {
constexpr char DLL_MODEL_NAME[] = "libhiretrieval_ext.z.so";
constexpr size_t ERR_MSG_MAX_LEN = 256;

using ReturnIntEmptyParamFunc = int32_t(*)();

DllModule Load()
{
#if defined(_WIN32)
return LoadLibraryA(DLL_MODEL_NAME);
#else
return dlopen(DLL_MODEL_NAME, RTLD_GLOBAL);
#endif
}

void Unload(DllModule module)
{
#if defined(_WIN32)
    FreeLibrary(module);
#else
    dlclose(module);
#endif
}

void LogDllErrInfo(const std::string& formatMsg)
{
    const size_t buffSize { ERR_MSG_MAX_LEN };
    char errMsg[buffSize] { };
    strerror_r(errno, errMsg, buffSize);
    HILOG_ERROR(LOG_CORE, "%{public}s: errno=%{public}d, msg=%{public}s", formatMsg.c_str(), errno, errMsg);
}

int32_t CallDllFunc(void* module, const std::string& funcName)
{
    if (module == INVALID_DLL_MODULE) {
        LogDllErrInfo("failed to load hiretrieval_ext module:");
        return HiRetrieval::NativeErrorCode::DLL_FAILED;
    }
    DllModule loadFunc = dlsym(module, funcName.c_str());
    if (loadFunc == INVALID_DLL_MODULE) {
        LogDllErrInfo("failed to load func:");
        return HiRetrieval::NativeErrorCode::DLL_FAILED;
    }
    auto callFunc = reinterpret_cast<ReturnIntEmptyParamFunc>(loadFunc);
    return callFunc();
}
}

HiRetrievalDynamicLoader::HiRetrievalDynamicLoader()
{
    loadModule_ = Load();
}

HiRetrievalDynamicLoader::~HiRetrievalDynamicLoader()
{
    Unload(loadModule_);
    loadModule_ = INVALID_DLL_MODULE;
}

int32_t HiRetrievalDynamicLoader::Init()
{
    return CallDllFunc(loadModule_, "HiRetrieval_Init");
}

int32_t HiRetrievalDynamicLoader::Participate()
{
    return CallDllFunc(loadModule_, "HiRetrieval_Participate");
}

int32_t HiRetrievalDynamicLoader::Quit()
{
    return CallDllFunc(loadModule_, "HiRetrieval_Quit");
}

int32_t HiRetrievalDynamicLoader::Run()
{
    return CallDllFunc(loadModule_, "HiRetrieval_Run");
}
} // namespace OHOS::HiviewDFX