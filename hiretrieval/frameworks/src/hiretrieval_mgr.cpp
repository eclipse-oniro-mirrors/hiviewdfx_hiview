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

#include "hiretrieval_mgr.h"

#include <charconv>
#include <cinttypes>
#include <unordered_map>

#include "application_context.h"
#include "hilog/log.h"
#include "hiretrieval_base_util.h"
#include "preferences_helper.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D10

#undef LOG_TAG
#define LOG_TAG "HIRETRIEVAL_MGR"

namespace OHOS::HiviewDFX {
namespace {
constexpr long long DEFAULT_PARTICIPATION_TS = 0;
constexpr size_t CONFIG_PARAM_MAX_LEN = 128;
constexpr char PARTICIPATED_STATUS[] = "participated";
constexpr char QUIT_STATUS[] = "quit";

inline std::string SubOverLimitStr(const std::string& str)
{
    if (str.length() > CONFIG_PARAM_MAX_LEN) {
        return str.substr(0, CONFIG_PARAM_MAX_LEN);
    }
    return str;
}

void UpdateParticipationInfo(std::unordered_map<std::string, std::string>& infos,
    const std::string& preferenceFile)
{
    int32_t err = 0;
    auto pref = NativePreferences::PreferencesHelper::GetPreferences(preferenceFile, err);
    if (pref == nullptr) {
        HILOG_ERROR(LOG_CORE, "failed to get preference");
        return;
    }
    for (const auto& info : infos) {
        if (pref->PutString(info.first, info.second) != 0) {
            HILOG_ERROR(LOG_CORE, "failed to put value into preference file");
            return;
        }
    }
    if (pref->FlushSync() != 0) {
        HILOG_ERROR(LOG_CORE, "failed to flush preference file");
        return;
    }
    NativePreferences::PreferencesHelper::RemovePreferencesFromCache(preferenceFile);
}

std::string ReadParticipationInfo(const std::string& key, const std::string& preferenceFile)
{
    int32_t err = 0;
    auto pref = NativePreferences::PreferencesHelper::GetPreferences(preferenceFile, err);
    if (pref == nullptr) {
        HILOG_ERROR(LOG_CORE, "failed to get preference");
        return "";
    }
    std::string val = pref->GetString(key, "");
    NativePreferences::PreferencesHelper::RemovePreferencesFromCache(preferenceFile);
    return val;
}

inline long long GetCurrentTs()
{
    auto now = std::chrono::system_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return millisecs.count();
}

inline bool IsSameConfig(HiRetrievalMgr::Config& src, HiRetrievalMgr::Config& dest)
{
    return src.userType == dest.userType && src.deviceType == dest.deviceType &&
        src.deviceModel == dest.deviceModel;
}
}

HiRetrievalMgr& HiRetrievalMgr::GetInstance()
{
    static HiRetrievalMgr instance;
    return instance;
}

HiRetrievalMgr::HiRetrievalMgr()
{
    dllLoader_ = std::make_unique<HiRetrievalDynamicLoader>();
}

int32_t HiRetrievalMgr::Init()
{
    std::lock_guard<std::mutex> lock(mgrMutex_);
    if (isInit_) {
        HILOG_INFO(LOG_CORE, "hiretrieval module has been initialized");
        return HiRetrieval::NativeErrorCode::SUCC;
    }
    auto ret = dllLoader_->Init();
    if (ret == HiRetrieval::NativeErrorCode::SUCC ||
        ret == HiRetrieval::NativeErrorCode::DLL_FAILED) {
        isInit_ = true;
        return ret;
    }
    HILOG_ERROR(LOG_CORE, "failed to init, ret is %{public}d", ret);
    return ret;
}

int32_t HiRetrievalMgr::Participate(HiRetrievalMgr::Config& cfg)
{
    std::lock_guard<std::mutex> lock(mgrMutex_);
    if (!isInit_) {
        HILOG_ERROR(LOG_CORE, "module not init");
        return HiRetrieval::NativeErrorCode::NOT_INIT;
    }
    if (cfg.deviceType.empty()) {
        cfg.deviceType = HiRetrievalBaseUtil::GetDefaultDeviceType();
    }
    if (cfg.deviceModel.empty()) {
        cfg.deviceModel = HiRetrievalBaseUtil::GetDefaultDeviceModel();
    }
    if (HiRetrievalMgr::Config curCfg = GetCurrentConfig();
        IsParticipant() && IsSameConfig(curCfg, cfg)) {
        HILOG_INFO(LOG_CORE, "participation with same cfg isn't permitted");
        return HiRetrieval::NativeErrorCode::SUCC;
    }
    PersistConfig(cfg);
    auto ret = dllLoader_->Participate();
    if (ret == HiRetrieval::NativeErrorCode::SUCC ||
        ret == HiRetrieval::NativeErrorCode::DLL_FAILED) {
        return ret;
    }
    HILOG_ERROR(LOG_CORE, "failed to participate, ret is %{public}d", ret);
    return ret;
}

int32_t HiRetrievalMgr::Quit()
{
    std::lock_guard<std::mutex> lock(mgrMutex_);
    if (!isInit_) {
        HILOG_ERROR(LOG_CORE, "module not init");
        return HiRetrieval::NativeErrorCode::NOT_INIT;
    }
    if (!IsParticipant()) {
        HILOG_INFO(LOG_CORE, "no need to quit without participation");
        isInit_ = false;
        return HiRetrieval::NativeErrorCode::SUCC;
    }
    auto ret =  dllLoader_->Quit();
    if (ret == HiRetrieval::NativeErrorCode::SUCC ||
        ret == HiRetrieval::NativeErrorCode::DLL_FAILED) {
        ClearConfigCache();
        isInit_ = false;
        return ret;
    }
    HILOG_ERROR(LOG_CORE, "failed to quit, ret is %{public}d", ret);
    return ret;
}

bool HiRetrievalMgr::IsParticipant()
{
    auto status = ReadParticipationInfo(HiRetrieval::CommonDef::PARTICIPATION_STATUS_KEY, GetPreferenceFile());
    return status == PARTICIPATED_STATUS;
}

long long HiRetrievalMgr::GetLastParticipationTs()
{
    std::string ts = ReadParticipationInfo(HiRetrieval::CommonDef::PARTICIPATION_TS_KEY, GetPreferenceFile());
    long long ret = DEFAULT_PARTICIPATION_TS;
    auto parseRet = std::from_chars(ts.c_str(), ts.c_str() + ts.size(), ret);
    return (parseRet.ec != std::errc()) ? DEFAULT_PARTICIPATION_TS : ret;
}

int32_t HiRetrievalMgr::Run()
{
    std::lock_guard<std::mutex> lock(mgrMutex_);
    if (!isInit_) {
        HILOG_ERROR(LOG_CORE, "module not init");
        return HiRetrieval::NativeErrorCode::NOT_INIT;
    }
    auto ret = dllLoader_->Run();
    if (ret == HiRetrieval::NativeErrorCode::SUCC ||
        ret == HiRetrieval::NativeErrorCode::DLL_FAILED) {
        return ret;
    }
    HILOG_ERROR(LOG_CORE, "failed to run, ret is %{public}d", ret);
    return ret;
}

HiRetrievalMgr::Config HiRetrievalMgr::GetCurrentConfig()
{
    Config cfg;
    ReadPersistedConfig(cfg);
    return cfg;
}

void HiRetrievalMgr::SetWorkDir(const std::string& dir)
{
    workDir_ = dir;
}

std::string HiRetrievalMgr::GetPreferenceFile()
{
    if (!workDir_.empty()) {
        return workDir_ + "/hiretrieval_cfg.xml";
    }
    auto ctx = OHOS::AbilityRuntime::Context::GetApplicationContext();
    if (ctx == nullptr) {
        HILOG_ERROR(LOG_CORE, "context is invalid");
        return "";
    }
    std::string preferenceDir = ctx->GetPreferencesDir();
    if (preferenceDir.empty()) {
        HILOG_ERROR(LOG_CORE, "preference dir is empty");
        return "";
    }
    return preferenceDir + "/hiretrieval_cfg.xml";
}

void HiRetrievalMgr::PersistConfig(const HiRetrievalMgr::Config& cfg)
{
    std::unordered_map<std::string, std::string> infos;
    infos.emplace(HiRetrieval::CommonDef::USER_TYPE_ATTR_NAME, SubOverLimitStr(cfg.userType));
    infos.emplace(HiRetrieval::CommonDef::DEVICE_TYPE_ATTR_NAME, SubOverLimitStr(cfg.deviceType));
    infos.emplace(HiRetrieval::CommonDef::DEVICE_MODEL_ATTR_NAME, SubOverLimitStr(cfg.deviceModel));
    infos.emplace(HiRetrieval::CommonDef::PARTICIPATION_TS_KEY, std::to_string(GetCurrentTs()));
    infos.emplace(HiRetrieval::CommonDef::PARTICIPATION_STATUS_KEY, PARTICIPATED_STATUS);
    UpdateParticipationInfo(infos, GetPreferenceFile());
}

void HiRetrievalMgr::ClearConfigCache()
{
    std::unordered_map<std::string, std::string> infos;
    infos.emplace(HiRetrieval::CommonDef::USER_TYPE_ATTR_NAME, "");
    infos.emplace(HiRetrieval::CommonDef::DEVICE_TYPE_ATTR_NAME, "");
    infos.emplace(HiRetrieval::CommonDef::DEVICE_MODEL_ATTR_NAME, "");
    infos.emplace(HiRetrieval::CommonDef::PARTICIPATION_STATUS_KEY, QUIT_STATUS);
    UpdateParticipationInfo(infos, GetPreferenceFile());
}

void HiRetrievalMgr::ReadPersistedConfig(HiRetrievalMgr::Config& cfg)
{
    cfg.userType = ReadParticipationInfo(HiRetrieval::CommonDef::USER_TYPE_ATTR_NAME, GetPreferenceFile());
    cfg.deviceType = ReadParticipationInfo(HiRetrieval::CommonDef::DEVICE_TYPE_ATTR_NAME, GetPreferenceFile());
    cfg.deviceModel = ReadParticipationInfo(HiRetrieval::CommonDef::DEVICE_MODEL_ATTR_NAME, GetPreferenceFile());
}
} // namespace OHOS::HiviewDFX