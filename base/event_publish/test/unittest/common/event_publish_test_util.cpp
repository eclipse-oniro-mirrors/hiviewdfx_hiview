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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "event_publish_test_util.h"

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "hiview_logger.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("HiView-EventPublishUtil");
const int BUF_LEN = 128;
const int MD5_SUM_LEN = 32;
}
void InstallTestHap(const std::string& hapName)
{
    std::string installHapCmd = "bm install -p " + hapName;
    FILE *fp = popen(installHapCmd.c_str(), "r");
    if (fp == nullptr) {
        HIVIEW_LOGE("popen excute install test hap failed.");
    }
    pclose(fp);
    sleep(2); // 2 : sleep 2s
}

void UninstallTestHap(const std::string& hapName)
{
    std::string uninstallHapCmd = "bm uninstall -n " + hapName;
    FILE *fp = popen(uninstallHapCmd.c_str(), "r");
    if (fp == nullptr) {
        HIVIEW_LOGE("popen excute uninstall test hap failed.");
    }
    pclose(fp);
}

int32_t LaunchTestHap(const std::string& abilityName, const std::string& bundleName)
{
    std::string launchHapCmd = "/system/bin/aa start -a " + abilityName + " -b " + bundleName;
    FILE *fp = popen(launchHapCmd.c_str(), "r");
    if (fp == nullptr) {
        HIVIEW_LOGE("popen excute launch test hap failed.");
    }
    pclose(fp);
    sleep(2); // 2 : sleep 2s
    return GetPidByBundleName(bundleName);
}

void StopTestHap(const std::string& hapName)
{
    std::string stopHapCmd = "/system/bin/aa force-stop " + hapName;
    FILE *fp = popen(stopHapCmd.c_str(), "r");
    if (fp == nullptr) {
        HIVIEW_LOGE("popen excute stop test hap failed.");
    }
    pclose(fp);
}

int32_t GetPidByBundleName(const std::string& bundleName)
{
    std::string getPidCmd = "pidof " + bundleName;
    int32_t pid = -1;
    FILE *fp = popen(getPidCmd.c_str(), "r");
    if (fp == nullptr) {
        HIVIEW_LOGE("popen excute get process pid failed.");
        return pid;
    }
    std::string pidStr;
    char bufferInfo[BUF_LEN] = { '\0' };
    while (fgets(bufferInfo, sizeof(bufferInfo), fp) != nullptr) {
        pidStr += bufferInfo;
    }
    pclose(fp);
    HIVIEW_LOGI("the pid of process %{public}s is %{public}s", bundleName.c_str(), pidStr.c_str());
    std::stringstream pidStream(pidStr);
    pidStream >> pid;
    return pid;
}

int32_t GetUidByPid(const int32_t pid)
{
    std::string pidStatusPath = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream statusFile(pidStatusPath);
    int32_t uid = -1;
    if (!statusFile.is_open()) {
        HIVIEW_LOGE("open pid status failed.");
        return uid;
    }
    std::string uidFlag = "Uid:";
    std::string infoLine;
    while (std::getline(statusFile, infoLine)) {
        if (infoLine.compare(0, uidFlag.size(), uidFlag) == 0) {
            std::istringstream infoStream(infoLine);
            if (std::getline(infoStream, infoLine, ':') && std::getline(infoStream, infoLine)) {
                std::stringstream(infoLine) >> uid;
                HIVIEW_LOGI("uid of pid %{public}" PRId32 " is %{public}" PRId32 ".", pid, uid);
                break;
            }
        }
    }
    statusFile.close();
    return uid;
}

bool FileExists(const std::string& filePath)
{
    struct stat buffer;
    return stat(filePath.c_str(), &buffer) == 0;
}

bool LoadlastLineFromFile(const std::string& filePath, std::string& lastLine)
{
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::string line;
        int index = 0;
        while (std::getline(file, line)) {
            if (line.size() != 0) {
                lastLine = std::to_string(index) + line;
                index++;
            }
        }
        file.close();
        return true;
    }
    return false;
}

std::string GetFileMd5Sum(const std::string& filePath, int delayTime)
{
    sleep(delayTime);
    std::string cmdRes = "";
    if (!FileExists(filePath)) {
        HIVIEW_LOGE("the file is not Exists");
        return cmdRes;
    }
    std::string getMd5Cmd = "md5sum " + filePath;
    FILE *fp = popen(getMd5Cmd.c_str(), "r");
    if (fp == nullptr) {
        HIVIEW_LOGE("popen excute get md5 failed.");
        return cmdRes;
    }
    char bufferInfo[MD5_SUM_LEN + 1] = { '\0' };
    if (fgets(bufferInfo, MD5_SUM_LEN + 1, fp)) {
        cmdRes = bufferInfo;
    }
    pclose(fp);
    return cmdRes;
}
} // namespace HiviewDFX
} // namespace OHOS
