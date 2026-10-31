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
 * WITHfout WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "notify_leak_compress_manager.h"
#include "notify_leak_compress_common.h"
#include "dfr_util.h"
#include <iostream>
#include <fstream>
#include <type_traits>
#include <deque>
#include <string>
#include <sys/types.h>
#include <parameters.h>

namespace {
constexpr size_t MAX_COMPREESS_CNT_PER_WEEK = 2;
constexpr int DEFAUT_ROM_RSV_SIZE = 30720; // 30G
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "NotifyLeakCompressManager");

using namespace std;

NotifyLeakCompressManager::NotifyLeakCompressManager()
{
    InitFile();
}

NotifyLeakCompressManager::~NotifyLeakCompressManager()
{
}

void NotifyLeakCompressManager::InitFile()
{
    bool isFirstBoot = !DfrUtil::GetInstance().FileExist(COMPRESS_HISTORY_PATH);
    if (isFirstBoot) {
        HIVIEW_LOGI("is First Boot");
        WriteDefaultContend(0, DEFAULT_BUNDLENAME);
    }
    chmod(COMPRESS_HISTORY_PATH, S_IRUSR | S_IWUSR | S_IRGRP);
}

bool NotifyLeakCompressManager::CouldCompress()
{
    if (!DfrUtil::GetInstance().IsBetaOrDevelopMode()) {
        HIVIEW_LOGD("not beta version, not compress.");
        return false;
    }
    if (NotifyLeakCompressCommon::GetInstance().GetScreenOnStatus()) {
        HIVIEW_LOGD("screen on, not compress.");
        return false;
    }
    if (!NotifyLeakCompressCommon::GetInstance().GetPowerConnectStatus()) {
        HIVIEW_LOGD("power not connected, not compress.");
        return false;
    }
    if (DfrUtil::GetInstance().GetFreeRomMb() <= DEFAUT_ROM_RSV_SIZE) {
        HIVIEW_LOGD("not hava enough rom for compress!");
        return false;
    }
    HIVIEW_LOGI("screen off, power connected, have enough rom!");
    return true;
}

bool NotifyLeakCompressManager::CommonCompress(uint64_t faultType, const string &bundleName)
{
    if (DfrUtil::GetInstance().GetFreeRomMb() <= DEFAUT_ROM_RSV_SIZE) {
        HIVIEW_LOGW("not hava enough rom for compress!");
        return false;
    }
    bool ret = false;
    switch (faultType) {
        case NATIVE_LEAK_TYPE:
            break;
        case JS_OOM_TYPE:
            ret = HaveEnoughQuota(bundleName);
            break;
        default:
            HIVIEW_LOGE("unknown type: %{public}lu", faultType);
            break;
    }
    HIVIEW_LOGI("ret: %{public}d, faultType: %{public}" PRIu64 ", bundleName: %{public}s",
        ret, faultType, bundleName.c_str());
    return ret;
}

bool NotifyLeakCompressManager::HaveEnoughQuota(const string &bundleName)
{
    time_t now = time(nullptr);
    if (now == static_cast<time_t>(-1)) {
        HIVIEW_LOGE("time() failed");
        return false;
    }
    auto compressHistory = ReadCompressHistory();
    if (compressHistory.empty()) {
        return false;
    }
    bool bundleFound = false;
    for (const auto &record : compressHistory) {
        if (record.second == bundleName) {
            if (!DfrUtil::GetInstance().LongerThanAWeek(now, record.first)) {
                HIVIEW_LOGW("bundleName %{public}s has already compressd in a week", bundleName.c_str());
                return false;
            }
            bundleFound = true;
            break;
        }
    }
    if (!bundleFound) {
        time_t earliestTime = compressHistory.front().first;
        if (!DfrUtil::GetInstance().LongerThanAWeek(now, earliestTime)) {
            HIVIEW_LOGW("the max cnt has been used, but time not over a week");
            return false;
        }
    }
    int usedQuota = 0;
    for (const auto &record : compressHistory) {
        if (!DfrUtil::GetInstance().LongerThanAWeek(now, record.first)) {
            usedQuota++;
        }
    }
    int remainingQuota = static_cast<int>(MAX_COMPREESS_CNT_PER_WEEK) - usedQuota;
    int queueSize = NotifyLeakCompressCommon::GetInstance().GetCommonCompressQueueSize();
    if (remainingQuota <= 0 || queueSize >= remainingQuota) {
        HIVIEW_LOGI("common compress quota exceeded, remaining: %{public}d, queue: %{public}d, bundleName: %{public}s",
            remainingQuota, queueSize, bundleName.c_str());
        return false;
    }
    return true;
}

deque<pair<time_t, string>> NotifyLeakCompressManager::ReadCompressHistory()
{
    compressHistroyMtx_.lock();
    deque<pair<time_t, string>> history;
    ifstream fin(COMPRESS_HISTORY_PATH);
    if (!fin.is_open()) {
        HIVIEW_LOGE("compressHistory file open faied");
        compressHistroyMtx_.unlock();
        return history;
    }
    string line;
    while (getline(fin, line)) {
        stringstream ss(line);
        time_t timestamp;
        string bundleName;
        char delimiter;
        if (!(ss >> timestamp >> delimiter >> bundleName) || delimiter != ',') {
            HIVIEW_LOGE("Failed to decode value");
            compressHistroyMtx_.unlock();
            return history;
        }
        history.emplace_back(timestamp, bundleName);
    }
    fin.close();
    compressHistroyMtx_.unlock();
    return history;
}

void NotifyLeakCompressManager::WriteDefaultContend(time_t defaultTime, const string &bundleName)
{
    deque<pair<time_t, string>> history;
    for (size_t i = 0; i < MAX_COMPREESS_CNT_PER_WEEK; ++i) {
        history.emplace_back(defaultTime, bundleName);
    }
    OverWriteData(history);
}

void NotifyLeakCompressManager::OverWriteData(const deque<pair<time_t, string>> &history)
{
    compressHistroyMtx_.lock();
    ofstream fout(COMPRESS_HISTORY_PATH, ios::trunc);
    if (!fout.is_open()) {
        HIVIEW_LOGE("compressHistory file open faied");
        compressHistroyMtx_.unlock();
        return;
    }
    for (const auto &record : history) {
        fout << record.first << "," << record.second << "\n";
    }
    fout.close();
    compressHistroyMtx_.unlock();
}

int NotifyLeakCompressManager::GetCommonCompressRemainingQuota(const string &bundleName)
{
    time_t now = time(nullptr);
    if (now == static_cast<time_t>(-1)) {
        HIVIEW_LOGE("time() failed");
        return 0;
    }
    auto compressHistory = ReadCompressHistory();
    if (compressHistory.empty()) {
        return 0;
    }
    for (const auto &record : compressHistory) {
        if (record.second == bundleName) {
            if (DfrUtil::GetInstance().LongerThanAWeek(now, record.first)) {
                break;
            }
            return 0;
        }
    }
    int usedQuota = 0;
    for (const auto &record : compressHistory) {
        if (!DfrUtil::GetInstance().LongerThanAWeek(now, record.first)) {
            usedQuota++;
        }
    }
    return MAX_COMPREESS_CNT_PER_WEEK - usedQuota;
}

void NotifyLeakCompressManager::UpdateCompressHistory(const string &bundleName)
{
    time_t now = time(nullptr);
    auto compressHistory = ReadCompressHistory();
    while (compressHistory.size() >= MAX_COMPREESS_CNT_PER_WEEK) {
        compressHistory.pop_front();
    }
    compressHistory.push_back({now, bundleName});
    OverWriteData(compressHistory);
}
}
}
