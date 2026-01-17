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

#ifndef PANIC_REPORT_RECOVERY_H_
#define PANIC_REPORT_RECOVERY_H_

#include <filesystem>
#include <memory>
#include <string>

#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
namespace PanicReport {

struct BboxSaveLogFlags {
    std::string happenTime;
    std::string factoryRecoveryTime;
    std::string softwareVersion;
    bool isPanicUploaded = true;
    bool isStartUpShort = false;
};

/**
 * load bboxSaveLogFlags from file.
 *
 * @return bboxSaveLogFlags
 */
BboxSaveLogFlags LoadBboxSaveFlagFromFile();

/**
 * save bboxSaveLogFlags to file.
 *
 * @param bboxSaveLogFlags bboxSaveLogFlags
 * @return whether save completed.
 */
bool SaveBboxLogFlagsToFile(const BboxSaveLogFlags& bboxSaveLogFlags);

/**
 * clearAllFiles in the path given.
 *
 * @param dirPath the path
 * @return whether clear completed.
 */
bool ClearFilesInDir(const std::filesystem::path& dirPath);

/**
 * Initialize the configuration file.
 *
 * @return whether init completed.
 */
bool InitPanicConfigFile();

/**
 * Initialize the panic report.
 *
 * @return whether init completed.
 */
bool InitPanicReport(bool& isLastStartUpShort);

/**
 * The function to determine whether the system is crashed within 10 minutes after boot completed.
 *
 * @return whether the system is crashed within 10 minutes after boot completed.
 */
bool IsLastShortStartUp(bool isLastStartUpShort);

/**
 * The function to determine whether the system boot completed.
 *
 * @return whether whether the system starts normally.
 */
bool IsBootCompleted();

/**
 * The function to determine whether the system is crashed within 10 minutes after boot completed.
 *
 * @return whether the system is crashed within 10 minutes after boot completed.
 */
bool IsLastShortStartUp();

/**
 * The function to determine whether the sysEvent is recovery panic.
 *
 * @return whether the event is recovery panic event.
 */
bool IsRecoveryPanicEvent(const std::shared_ptr<SysEvent>& sysEvent);

/**
 * The function to get current software version.
 *
 * @return currentVersion.
 */
std::string GetCurrentVersion();

/**
 * The function to get the last recovery time.
 *
 * @return the time of last recovery.
 */
std::string GetLastRecoveryTime();

/**
 * The function to get the absolute file path by the timeStr given.
 *
 * @param timeStr timeStr.
 * @return the absolute path of bakeFile.
 */
std::string GetBackupFilePath(const std::string& timeStr);

/**
 * The function to get a value from the content given by param name.
 *
 * @param content  content.
 * @param param the param.
 * @return the value.
 */
std::string GetParamValueFromString(const std::string& content, const std::string& param);

/**
 * The function to get a value from the file given by param name.
 *
 * @param filePath the path of the file given.
 * @param param the param.
 * @return the value.
 */
std::string GetParamValueFromFile(const std::string& filePath, const std::string& param);

/**
 * The function to compress log files.
 *
 * @param srcPath filePath.
 * @param timeStr time in string format.
 */
void CompressAndCopyLogFiles(const std::string& srcPath, const std::string& timeStr);

/**
 * The function to determine whether the sysEvent is recovery panic.
 *
 * @return whether the event is recovery panic event.
 */
bool IsRecoveryPanicEvent(const std::shared_ptr<SysEvent>& sysEvent);

/**
 * Try to report recovery panic event.
 *
 * @return whether the recovery panic event was reported
 */
bool TryToReportRecoveryPanicEvent();

/**
 * Confirm the result of recovery report.
 */
void ConfirmReportResult();
}
}
}

#endif // PANIC_REPORT_RECOVERY_H_
