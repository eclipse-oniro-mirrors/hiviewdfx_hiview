/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "panic_report_recovery.h"

#include <filesystem>
#include <regex>
#include <cstdlib>

#include "file_util.h"
#include "hiview_logger.h"
#include "hiview_zip_util.h"
#include "hisysevent.h"
#include "hisysevent_util.h"
#include "parameters.h"
#include "string_util.h"
#include "tbox.h"

namespace OHOS {
namespace HiviewDFX {
namespace PanicReport {
DEFINE_LOG_LABEL(0xD002D11, "PanicReport");

#ifdef UNITTEST
constexpr const char* BBOX_PARAM_PATH = "/data/test/bbox/bbox.save.log.flags";
constexpr const char* PANIC_LOG_PATH = "/data/test/bbox/panic_log/";
#else
constexpr const char* BBOX_PARAM_PATH = "/log/bbox/bbox.save.log.flags";
constexpr const char* PANIC_LOG_PATH = "/log/bbox/panic_log/";
#endif
constexpr const char* FACTORY_RECOVERY_TIME_PATH = "/log/bbox/factory.recovery.time";
constexpr const char* CMD_LINE = "/proc/cmdline";

constexpr const char* LAST_FASTBOOT_LOG = "/ap_log/last_fastboot_log";
constexpr const char* HM_KLOG = "/ap_log/hm_klog.txt";
constexpr const char* HM_SNAPSHOT = "/ap_log/hm_snapshot.txt";

constexpr const char* BOOTEVENT_BOOT_COMPLETED = "bootevent.boot.completed";
constexpr const char* LAST_BOOTUP_KEYPOINT = "last_bootup_keypoint";

constexpr const char* HAPPEN_TIME = "happentime";
constexpr const char* FACTORY_RECOVERY_TIME = "factoryRecoveryTime";
constexpr const char* IS_PANIC_UPLOADED = "isPanicUploaded";
constexpr const char* IS_STARTUP_SHORT = "isStartUpShort";
constexpr const char* SOFTWARE_VERSION = "softwareVersion";

constexpr const char* REGEX_FORMAT = R"(=((".*?")|(\S*)))";

constexpr int COMPRESSION_RATION = 9;
constexpr int ZIP_FILE_SIZE_LIMIT = 5 * 1024 * 1024; // 5MB

static bool g_isLastStartUpShort = false;

bool InitPanicConfigFile()
{
    if (!FileUtil::FileExists(BBOX_PARAM_PATH) && !FileUtil::CreateFile(BBOX_PARAM_PATH, FileUtil::FILE_PERM_640)) {
        return false;
    }
    BboxSaveLogFlags bboxSaveLogFlags = LoadBboxSaveFlagFromFile();
    g_isLastStartUpShort = bboxSaveLogFlags.isStartUpShort;
    bboxSaveLogFlags.isStartUpShort = true;
    return SaveBboxLogFlagsToFile(bboxSaveLogFlags);
}

BboxSaveLogFlags LoadBboxSaveFlagFromFile()
{
    std::string content;
    if (!FileUtil::LoadStringFromFile(BBOX_PARAM_PATH, content)) {
        HIVIEW_LOGE("Failed to load file: %{public}s.", BBOX_PARAM_PATH);
        return {};
    }
    return {
        .happenTime = GetParamValueFromString(content, HAPPEN_TIME),
        .factoryRecoveryTime = GetParamValueFromString(content, FACTORY_RECOVERY_TIME),
        .softwareVersion = GetParamValueFromString(content, SOFTWARE_VERSION),
        .isPanicUploaded = GetParamValueFromString(content, IS_PANIC_UPLOADED) != "false",
        .isStartUpShort = GetParamValueFromString(content, IS_STARTUP_SHORT) == "true"
    };
}

bool SaveBboxLogFlagsToFile(const BboxSaveLogFlags& bboxSaveLogFlags)
{
    std::stringstream ss;
    ss << HAPPEN_TIME << "=" << bboxSaveLogFlags.happenTime <<
        " " << FACTORY_RECOVERY_TIME << "=" << bboxSaveLogFlags.factoryRecoveryTime <<
        " " << SOFTWARE_VERSION << "=" << bboxSaveLogFlags.softwareVersion <<
        " " << IS_PANIC_UPLOADED << "=" << (bboxSaveLogFlags.isPanicUploaded ? "true" : "false") <<
        " " << IS_STARTUP_SHORT << "=" << (bboxSaveLogFlags.isStartUpShort ? "true" : "false");
    if (!FileUtil::SaveStringToFile(BBOX_PARAM_PATH, ss.str())) {
        HIVIEW_LOGE("failed to save the msg to file: %{public}s", BBOX_PARAM_PATH);
        return false;
    }
    return true;
}

bool InitPanicReport()
{
    auto fileSize = FileUtil::GetFolderSize(PANIC_LOG_PATH);
    if (fileSize > ZIP_FILE_SIZE_LIMIT) {
        HIVIEW_LOGW("zip file size: %{public}" PRIu64 " is over limit", fileSize);
        ClearFilesInDir(PANIC_LOG_PATH);
    }
    return InitPanicConfigFile() ;
}

bool ClearFilesInDir(const std::string &dirPath)
{
    namespace fs  = std::filesystem;
    if (!fs::exists(dirPath) || fs::is_regular_file(dirPath)) {
        HIVIEW_LOGE("The file %{public}s is not existed or is not an directory", dirPath.c_str());
        return false;
    }
    bool ret = true;
    std::error_code errorCode;
    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (fs::is_regular_file(entry.path())) {
            if (!fs::remove(entry.path(), errorCode)) {
                HIVIEW_LOGE("Failed to deleted %{public}s errorCode: %{public}d",
                            entry.path().c_str(), errorCode.value());
                ret = false;
            }
        } else if (fs::is_directory(entry.path())) {
            if (!ClearFilesInDir(entry.path()) || !fs::remove(entry.path(), errorCode)) {
                HIVIEW_LOGE("Failed to deleted %{public}s errorCode: %{public}d",
                            entry.path().c_str(), errorCode.value());
                ret = false;
            }
        }
    }
    return ret;
};

bool IsBootCompleted()
{
    return OHOS::system::GetParameter(BOOTEVENT_BOOT_COMPLETED, "false") == "true";
}

bool IsLastShortStartUp()
{
    if (g_isLastStartUpShort) {
        return true;
    }
    std::string lastBootUpKeyPoint = GetParamValueFromFile(CMD_LINE, LAST_BOOTUP_KEYPOINT);
    if (lastBootUpKeyPoint.empty()) {
        return false;
    }
    constexpr int normalNum = 250;
    return atoi(lastBootUpKeyPoint.c_str()) < normalNum;
}

bool IsRecoveryPanicEvent(const std::shared_ptr<SysEvent>& sysEvent)
{
    std::string logPath = sysEvent->GetEventValue("LOG_PATH");
    return logPath.find(PANIC_LOG_PATH) == 0;
}

std::string GetLastRecoveryTime()
{
    std::string factoryRecoveryTime;
    FileUtil::LoadStringFromFile(FACTORY_RECOVERY_TIME_PATH, factoryRecoveryTime);
    return std::string("\"") + factoryRecoveryTime + "\"";
}

std::string GetCurrentVersion()
{
    std::string currentVersion = OHOS::system::GetParameter("const.product.software.version", "unknown");
    return std::string("\"") + currentVersion + "\"";
}

std::string GetBackupFilePath(const std::string& timeStr)
{
    return std::string(PANIC_LOG_PATH) + "panic_log_" + timeStr + ".zip";
}

std::string GetParamValueFromString(const std::string& content, const std::string& param)
{
    std::smatch matchStr;
    if (!std::regex_search(content, matchStr, std::regex(param + REGEX_FORMAT))) {
        return "";
    }
    return matchStr[1].str();
}

std::string GetParamValueFromFile(const std::string& filePath, const std::string& param)
{
    std::string content;
    if (!FileUtil::LoadStringFromFile(filePath, content)) {
        HIVIEW_LOGE("Failed to load file: %{public}s.", filePath.c_str());
        return "";
    }
    return GetParamValueFromString(content, param);
}

void AddFileToZip(HiviewZipUnit& hiviewZipUnit, const std::string& path)
{
    if (hiviewZipUnit.AddFileInZip(path, ZipFileLevel::KEEP_NONE_PARENT_PATH) != 0) {
        HIVIEW_LOGW("Failed to add file: %{public}s to zip", path.c_str());
    }
}

void CompressAndCopyLogFiles(const std::string& srcPath, const std::string& timeStr)
{
    if (!FileUtil::FileExists(PANIC_LOG_PATH)) {
        HIVIEW_LOGE("The path of target file: %{public}s is not existed", PANIC_LOG_PATH);
        return;
    }
    ClearFilesInDir(PANIC_LOG_PATH);
    HiviewZipUnit hiviewZipUnit(GetBackupFilePath(timeStr));
    std::string lastFastBootLog = srcPath + LAST_FASTBOOT_LOG;
    AddFileToZip(hiviewZipUnit, lastFastBootLog);
    std::string hmKLog = srcPath + HM_KLOG;
    AddFileToZip(hiviewZipUnit, hmKLog);
    std::string hmSnapShot = srcPath + HM_SNAPSHOT;
    uint64_t sourceFileSize = FileUtil::GetFileSize(lastFastBootLog) + FileUtil::GetFileSize(hmKLog) +
        FileUtil::GetFileSize(hmSnapShot);
    if (sourceFileSize < COMPRESSION_RATION * ZIP_FILE_SIZE_LIMIT) {
        AddFileToZip(hiviewZipUnit, hmSnapShot);
    } else {
        HIVIEW_LOGW("sourceFileSize size: %{public}" PRIu64 " is over limit, dropping hmSnapShot.", sourceFileSize);
    }
    uint64_t zipFileSize = FileUtil::GetFolderSize(PANIC_LOG_PATH);
    if (zipFileSize > ZIP_FILE_SIZE_LIMIT) {
        HIVIEW_LOGW("zip file size: %{public}" PRIu64 " is over limit", zipFileSize);
        ClearFilesInDir(PANIC_LOG_PATH);
        return;
    }
    BboxSaveLogFlags bboxSaveLogFlags = LoadBboxSaveFlagFromFile();
    bboxSaveLogFlags.happenTime = timeStr;
    bboxSaveLogFlags.factoryRecoveryTime = GetLastRecoveryTime();
    bboxSaveLogFlags.isPanicUploaded = false;
    bboxSaveLogFlags.softwareVersion = GetCurrentVersion();
    SaveBboxLogFlagsToFile(bboxSaveLogFlags);
}

void ReportPanicEventAfterRecovery(const BboxSaveLogFlags& bboxSaveLogFlags)
{
    const std::string& timeStr = bboxSaveLogFlags.happenTime;
    int64_t happenTime = Tbox::GetHappenTime(StringUtil::GetRleftSubstr(timeStr, "-"),
                                             "(\\d{4})(\\d{2})(\\d{2})(\\d{2})(\\d{2})(\\d{2})");
    HiSysEventWrite(
        HisysEventUtil::KERNEL_VENDOR,
        "PANIC",
        HiSysEvent::EventType::FAULT,
        "MODULE", "AP",
        "REASON", "HM_PANIC:HM_PANIC_SYSMGR",
        "LOG_PATH", GetBackupFilePath(timeStr),
        "SUB_LOG_PATH", timeStr,
        "HAPPEN_TIME", happenTime,
        "FIRST_FRAME", "RECOVERY_PANIC",
        "LAST_FRAME", bboxSaveLogFlags.softwareVersion,
        "FINGERPRINT", Tbox::CalcFingerPrint(timeStr + bboxSaveLogFlags.softwareVersion, 0, FP_BUFFER)
    );
}

bool TryToReportRecoveryPanicEvent()
{
    BboxSaveLogFlags bboxSaveLogFlags = LoadBboxSaveFlagFromFile();
    bool ret = false;
    if (!bboxSaveLogFlags.isPanicUploaded && bboxSaveLogFlags.factoryRecoveryTime != GetLastRecoveryTime()) {
        if (FileUtil::FileExists(GetBackupFilePath(bboxSaveLogFlags.happenTime))) {
            ReportPanicEventAfterRecovery(bboxSaveLogFlags);
            ret = true;
        }
    }
    bboxSaveLogFlags.isStartUpShort = false;
    SaveBboxLogFlagsToFile(bboxSaveLogFlags);
    return ret;
}

void ConfirmReportResult()
{
    BboxSaveLogFlags bboxSaveLogFlags = LoadBboxSaveFlagFromFile();
    const std::string& timeStr = bboxSaveLogFlags.happenTime;
    if (HisysEventUtil::IsEventProcessed("PANIC", "LOG_PATH", GetBackupFilePath(timeStr))) {
        bboxSaveLogFlags.isPanicUploaded = true;
        SaveBboxLogFlagsToFile(bboxSaveLogFlags);
    }
}
}
}
}