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
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/statvfs.h>
#include <parameters.h>
#include <parameter.h>
#include <securec.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <charconv>
#include <sys/xattr.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <cerrno>
#include <thread>
#include <chrono>
#include <csignal>
#include <ability_mgr_helper.h>
#include "dfr_util.h"
#include "file_util.h"
#include "fault_common.h"
#include "hiview_logger.h"
#include "bundle_mgr_client.h"
#include "dirent.h"
#include "suspend_manager_client.h"
#include "ffrt.h"
#include "native_leak_detector.h"
#include "hitrace_meter.h"
#include "os_account_manager.h"
#include "trace_collector.h"
#include "perf_collector.h"
#include "app_mgr_client.h"
#include "cJSON.h"
#include "sys_event.h"
#include "hiview_global.h"
#include "string_ex.h"
#include "application_context.h"

using namespace OHOS::AppExecFwk;
using namespace std;
using namespace OHOS::HiviewDFX::UCollectUtil;

namespace {
constexpr size_t PIPE_READ_BUF_SIZE = 4096;
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FaultLeakDetectorUtil");

namespace {
constexpr const char *const APP_TELEMETRY_XATTR_NAME = "user.resource_observer";
constexpr const char *const RUNNING_ID_XATTR_NAME = "user.runningId";
constexpr const char *const RSRC_OBSV_TRSHD_XATTR_PSS = "user.resource_observer_threshold.pss";
constexpr const char *const RSRC_OBSV_TRSHD_XATTR_GPU = "user.resource_observer_threshold.gpu";
constexpr const char *const RSRC_OBSV_TRSHD_XATTR_FD = "user.resource_observer_threshold.fd";
constexpr const char *const RSRC_OBSV_TRSHD_XATTR_RAT = "user.resource_observer_threshold.rss_ark_ts";
constexpr const char *const RSRC_OBSV_TRSHD_XATTR_RNH = "user.resource_observer_threshold.rss_native_heap";
constexpr const char *const RSRC_OBSV_TRSHD_XATTR_KH = "user.resource_observer_threshold.kmp_heap";
constexpr const char *const RSRC_OBSV_TRSHD_XATTR_ION = "user.resource_observer_threshold.ion";
constexpr int WIN_ID_LENGTH = 4;
constexpr int PID_INDEX = 2;
constexpr int WIN_ID_INDEX = 3;
constexpr int MAX_GET_SMAPS_NUM = 5;
// values must be 2: runningId,atLeakType
constexpr size_t XATTR_VALUE_COUNT = 2;
}

string DfrUtil::GetSubtypeByPpid(pid_t ppid)
{
    if (ppid == 0 || ppid == 1) {
        return NATIVE_DEFAULT;
    }
    return APP_DEFAULT;
}

string DfrUtil::GetExtensionSubtype(AppExecFwk::ExtensionAbilityType type)
{
    switch (type) {
        case AppExecFwk::ExtensionAbilityType::ASSET_ACCELERATION:
            return ASSET_ACCELERATION_TYPE;
        case AppExecFwk::ExtensionAbilityType::SERVICE:
            return SERVICE_TYPE;
        default:
            return EXTENSION_DEFAULT;
    }
}

string DfrUtil::GetSubtype(pid_t pid, pid_t ppid)
{
#ifdef DUBAI_CONFIG_FEATURE_RES_QUOTA
    AAFwk::ExtensionRunningInfo info;
    bool ret = XPowerManager::AbilityMgrHelper::Instance().GetPidInfo(pid, info);
    if (ret) {
        return GetExtensionSubtype(info.type);
    }
#endif
    return GetSubtypeByPpid(ppid);
}

string DfrUtil::SendLeakCmdInfo(pid_t pid, uint32_t cmdType, uint32_t infoType,
    uint32_t dfxLimit, uint32_t appKillHardThreshold)
{
    auto fd = open(BBOX_PATH, O_RDONLY);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", BBOX_PATH, strerror(errno));
        return "";
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    uint64_t stackSize = sizeof(CmdInfo) + MEMCHECK_CMDINFO_MAXSIZE;
    auto cmdInfo = static_cast<CmdInfo *>(calloc(1, stackSize));
    if (cmdInfo == nullptr) {
        HIVIEW_LOGE("FAILED TO ALLOC MEMORY!");
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return "";
    }
    auto cleanup = [cmdInfo, fd]() {
        if (cmdInfo) {
            free(cmdInfo);
        }
        if (fd >= 0) {
            fdsan_close_with_tag(fd, FDSAN_TAG);
        }
    };
    cmdInfo->appKillHardThreshold = appKillHardThreshold;
    cmdInfo->magic = MEMCHECK_MAGIC;
    cmdInfo->pid = static_cast<uint32_t>(pid);
    cmdInfo->size = MEMCHECK_CMDINFO_MAXSIZE;
    cmdInfo->cmdType = cmdType;
    cmdInfo->infoType = infoType;
    cmdInfo->dfxLimit = dfxLimit;
    cmdInfo->timestamp = static_cast<uint64_t>(time(nullptr));
    HIVIEW_LOGI("cmd TYPE: pid=%{public}u, cmdType=%{public}u, infoType=%{public}u, time=%{public}" PRIu64,
                cmdInfo->pid, cmdInfo->cmdType, cmdInfo->infoType, cmdInfo->timestamp);
    int32_t ret = ioctl(fd, LOGGER_MEMCHECK_CMD_INFO, cmdInfo);
    if (ret != 0) {
        HIVIEW_LOGE("IOCTL READ CMD INFO FAILED, ret: %{public}d, err: %{public}s", ret, strerror(errno));
        cleanup();
        return "";
    }
    string data = cmdInfo->data;
    cleanup();
    return data;
}

void DfrUtil::DumpSingleDetailFromIoctl(pid_t pid, uint32_t cmdType, uint32_t infoType, ofstream &os)
{
    string data = SendLeakCmdInfo(pid, cmdType, infoType);
    os << data << endl;
    return;
}

string DfrUtil::JoinPidsStr(const vector<int32_t> &pids)
{
    std::ostringstream oss;
    for (size_t i = 0; i < pids.size(); ++i) {
        if (i != 0) {
            oss << ",";
        }
        oss << pids[i];
    }
    return oss.str();
}

void DfrUtil::NotifyWarningToApp(int32_t pid, AppExecFwk::LeakType leakType, unsigned long leakSize,
                                 const AppExecFwk::LeakDetailInfo &leakDetailInfo)
{
    auto appMgrClient = std::make_unique<AppMgrClient>();
    auto connectResult = appMgrClient->ConnectAppMgrService();
    if (connectResult != AppMgrResultCode::RESULT_OK) {
        HIVIEW_LOGE("ConnectAppMgrService Failed, result: %{public}d", connectResult);
        return;
    }

    OHOS::AppExecFwk::AppFaultDataBySA faultData;
    faultData.faultType = FaultDataType::RESOURCE_CONTROL;
    faultData.errorObject.name = "RESOURCE_LEAK_NOTIFY";
    faultData.errorObject.stack = "";
    faultData.pid = pid;
    faultData.notifyApp = true;
    faultData.leakObject.leakType = leakType;
    faultData.leakObject.leakSize = leakSize;
    faultData.leakObject.detailInfo = leakDetailInfo;

    int32_t resultCode = appMgrClient->NotifyAppFaultBySA(faultData);
    if (resultCode != AppMgrResultCode::RESULT_OK) {
        HIVIEW_LOGE("NotifyAppFaultBySA failed");
    }
}

string DfrUtil::GetAppRunningUniqueIdByPid(pid_t pid)
{
    string appRunningUniqueId = "";
    int32_t queryResult =
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->GetAppRunningUniqueIdByPid(pid, appRunningUniqueId);
    // when queryResult != 0, process already died
    string queryInfo = queryResult == 0 ? "success" : "error";
    HIVIEW_LOGI("Pid: %{public}d, get appRunningId %{public}s, value: %{public}s", pid, queryInfo.c_str(),
                appRunningUniqueId.c_str());
    return appRunningUniqueId;
}

void DfrUtil::DumpRssKillerSmaps(pid_t pid, string &smapsInfo)
{
    auto fd = open(BBOX_PATH, O_RDWR);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", BBOX_PATH, strerror(errno));
        return;
    }

    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    uint64_t stackSize = sizeof(CmdInfo) + MEMCHECK_CMDINFO_MAXSIZE;
    auto cmdInfo = static_cast<CmdInfo *>(calloc(1, stackSize));
    if (cmdInfo == nullptr) {
        HIVIEW_LOGE("FAILED TO ALLOC MEMORY!");
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return;
    }
    cmdInfo->magic = MEMCHECK_MAGIC;
    cmdInfo->pid = static_cast<uint32_t>(pid);
    cmdInfo->size = MEMCHECK_CMDINFO_MAXSIZE;
    cmdInfo->cmdType = CMD_GET_MEM_DETAIL_INFO;
    cmdInfo->infoType = RSS_KILL_REPORT_DETAIL;
    cmdInfo->timestamp = static_cast<uint64_t>(time(nullptr));

    int getNum = 0;
    while (cmdInfo->copyErr > 0 || cmdInfo->offset == 0) {
        int ret = ioctl(fd, LOGGER_MEMCHECK_CMD_INFO, cmdInfo);
        if (ret != 0 || cmdInfo->copyErr <= 0) {
            HIVIEW_LOGW("ioctl failed or smaps dump finished ret: %{public}d, copyErr: %{public}d", ret,
                        cmdInfo->copyErr);
            break;
        }
        cmdInfo->offset += static_cast<uint64_t>(cmdInfo->copyErr);
        string data(cmdInfo->data, static_cast<size_t>(cmdInfo->copyErr));
        smapsInfo += data;
        getNum++;
        if (getNum > MAX_GET_SMAPS_NUM) {
            HIVIEW_LOGW("Get smpas counts over limit, count:%{public}d", getNum);
            break;
        }
    }
    free(cmdInfo);
    fdsan_close_with_tag(fd, FDSAN_TAG);
    // free rss smaps in kernel
    FreeRssKillerSmaps(pid);
    // write into file
}

void DfrUtil::FreeRssKillerSmaps(pid_t pid)
{
    SendLeakCmdInfo(pid, CMD_GET_MEM_DETAIL_INFO, RSS_KILL_REPORT_DETAIL_FREE);
}

uint64_t DfrUtil::GetDegital(const string &data, const string &startStr, const string &endStr)
{
    uint64_t virtualSize = 0;
    auto indexStart = data.find(startStr);
    if (indexStart == string::npos) {
        return 0;
    }
    auto indexEnd = data.find(endStr, indexStart);
    if (indexEnd == string::npos) {
        indexEnd = data.size();
    }
    indexStart += startStr.size();
    if (indexEnd <= indexStart) {
        return 0;
    }

    string virtualSizeStr = data.substr(indexStart, indexEnd - indexStart);
    if (!DfrUtil::GetInstance().ConvertToULL(virtualSizeStr, virtualSize)) {
        return 0;
    }
    return virtualSize;
}

uint64_t DfrUtil::GetDirSize(const string &dirPath)
{
    return FileUtil::GetFolderSize(dirPath);
}

bool DfrUtil::GetFilesInDir(const string &dirPath, vector<string> &fileList)
{
    FileUtil::GetDirFiles(dirPath, fileList);
    return fileList.size() == 0 ? false : true;
}

std::string DfrUtil::GetXattr(const std::string &path, const std::string &name)
{
    ssize_t attrSize = getxattr(path.c_str(), name.c_str(), nullptr, 0);
    if (attrSize <= 0) {
        return "";
    }

    std::vector<char> buffer(attrSize);
    ssize_t ret = getxattr(path.c_str(), name.c_str(), buffer.data(), attrSize);
    if (ret <= 0) {
        HIVIEW_LOGE("failed getxattr, ret: %{public}zd path: %{public}s, name: %{public}s", ret, path.c_str(),
                    name.c_str());
        return "";
    }
    std::string value(buffer.data(), ret);

    return value;
}

bool DfrUtil::ReadResourceObserver(const std::string &xattrPath, std::string &runningId, uint64_t &atLeakType)
{
    string value = GetXattr(xattrPath, APP_TELEMETRY_XATTR_NAME);
    if (value.empty()) {
        return false;
    }

    // 2: values must be 2, like: runnindId,atLeakType
    vector<string> values = Split(value, ',');
    if (values.size() != XATTR_VALUE_COUNT) {
        HIVIEW_LOGE("invalid value.size: %{public}zu, must be 2", value.size());
        return false;
    }

    const std::string typeStr = values[1];  // 1: index for atLeakType
    long long type = 0;
    if (!SafeStoll(typeStr, type)) {
        HIVIEW_LOGE("failed SafeStoll, typeStr: %{public}s", typeStr.c_str());
        return false;
    }
    atLeakType = static_cast<uint64_t>(type);
    runningId = values[0];  // 0: index for runnindId
    return true;
}

void DfrUtil::GetAppTelemetryThresholdAndType(const string &bundleName, uint64_t &type, int *threshold)
{
    int thresholdTmp = 0;
    if (ReadAppTelemetryThreshold(bundleName, AppExecFwk::ATLT_RSS_NATIVE_HEAP) != INT_MAX) {
        thresholdTmp = ReadAppTelemetryThreshold(bundleName, AppExecFwk::ATLT_RSS_NATIVE_HEAP);
        type = AppExecFwk::ATLT_RSS_NATIVE_HEAP;
    } else if (ReadAppTelemetryThreshold(bundleName, AppExecFwk::ATLT_RSS_ARK_TS) != INT_MAX) {
        thresholdTmp = ReadAppTelemetryThreshold(bundleName, AppExecFwk::ATLT_RSS_ARK_TS);
        type = AppExecFwk::ATLT_RSS_ARK_TS;
    } else if (ReadAppTelemetryThreshold(bundleName, AppExecFwk::ATLT_RSS_KMP_DUMP) != INT_MAX) {
        thresholdTmp = ReadAppTelemetryThreshold(bundleName, AppExecFwk::ATLT_RSS_KMP_DUMP);
        type = AppExecFwk::ATLT_RSS_KMP_DUMP;
    }
    HIVIEW_LOGD("app telemetry bundleName: %{public}s, threshold: %{public}d, type: %{public}lu", bundleName.c_str(),
                thresholdTmp, type);
    if (threshold != nullptr) {
        *threshold = thresholdTmp;
    }
}

std::string DfrUtil::GetXattrNameByATLT(AppExecFwk::AppTelemetryLeakType atlt)
{
    switch (atlt) {
        case ATLT_PSS:
            return RSRC_OBSV_TRSHD_XATTR_PSS;
        case ATLT_DMA:
            return RSRC_OBSV_TRSHD_XATTR_ION;
        case ATLT_GPU:
            return RSRC_OBSV_TRSHD_XATTR_GPU;
        case ATLT_FD:
            return RSRC_OBSV_TRSHD_XATTR_FD;
        case ATLT_RSS_ARK_TS:
            return RSRC_OBSV_TRSHD_XATTR_RAT;
        case ATLT_RSS_NATIVE_HEAP:
            return RSRC_OBSV_TRSHD_XATTR_RNH;
        case ATLT_RSS_KMP_DUMP:
            return RSRC_OBSV_TRSHD_XATTR_KH;
    }
    return "unknown";
}

// INT_MAX for no register
int DfrUtil::ReadAppTelemetryThreshold(const std::string &bundleName, AppExecFwk::AppTelemetryLeakType atlt)
{
    const std::string xattrPath = GetBundleQuotaPath(bundleName);
    std::string runningIdWhenSetting;
    uint64_t registedAtlt = 0;
    if (!ReadResourceObserver(xattrPath, runningIdWhenSetting, registedAtlt)) {
        return INT_MAX;
    }
    std::string curRunnindId = GetXattr(xattrPath, RUNNING_ID_XATTR_NAME);
    if (curRunnindId.empty()) {
        HIVIEW_LOGE("failed to get curRunnindId, bundleName: %{public}s", bundleName.c_str());
        return INT_MAX;
    }
    if (runningIdWhenSetting != curRunnindId) {
        HIVIEW_LOGE("runningIdWhenSetting != curRunnindId, bundleName: %{public}s", bundleName.c_str());
        return INT_MAX;
    }
    if ((registedAtlt & atlt) == 0) {
        HIVIEW_LOGE("not register current type, registedAtlt: %{public}" PRIu64 ", atlt: %{public}d", registedAtlt,
                    atlt);
        return INT_MAX;
    }
    if (atlt == AppExecFwk::ATLT_GPU || atlt == AppExecFwk::ATLT_FD || atlt == AppExecFwk::ATLT_DMA) {
        HIVIEW_LOGI("atlt: %{public}u no need to check threshold", atlt);
        return 0;
    }
    std::string xattrName = GetXattrNameByATLT(atlt);
    std::string thresholdStr = GetXattr(xattrPath, xattrName);
    int threshold = 0;
    if (!SafeStoi(thresholdStr, threshold)) {
        HIVIEW_LOGE("failed to SafeStoi, thresholdStr: %{public}s", thresholdStr.c_str());
        return INT_MAX;
    }
    if (threshold > INT_MAX / KB_PER_MB) {
        HIVIEW_LOGE("threshold too large, overflow risk: %{public}d", threshold);
        return INT_MAX;
    }
    threshold = threshold * KB_PER_MB;
    HIVIEW_LOGI("bundleName: %{public}s, registedAtlt: %{public}" PRIu64
                ", notifyAtlt: %{public}d, threshold: %{public}d K",
                bundleName.c_str(), registedAtlt, atlt, threshold);
    return threshold;
}

std::string DfrUtil::SerializeHookSizeToJson(const GpuHookSize &hookSizeMap)
{
    cJSON *root = cJSON_CreateObject();
    for (const auto &[type, hookSize] : hookSizeMap) {
        cJSON *typeObj = cJSON_CreateObject();
        cJSON *firstArr = cJSON_CreateArray();
        cJSON_AddItemToArray(firstArr, cJSON_CreateNumber(hookSize.first.first));
        cJSON_AddItemToArray(firstArr, cJSON_CreateNumber(hookSize.first.second));
        cJSON_AddItemToObject(typeObj, "first", firstArr);

        cJSON *secondArr = cJSON_CreateArray();
        cJSON_AddItemToArray(secondArr, cJSON_CreateNumber(hookSize.second.first));
        cJSON_AddItemToArray(secondArr, cJSON_CreateNumber(hookSize.second.second));
        cJSON_AddItemToObject(typeObj, "second", secondArr);

        cJSON_AddItemToObject(root, type.c_str(), typeObj);
    }
    char *jsonStr = cJSON_PrintUnformatted(root);
    if (jsonStr == nullptr) {
        cJSON_Delete(root);
        return "";
    }
    std::string result(jsonStr);
    free(jsonStr);
    cJSON_Delete(root);
    return result;
}

void DfrUtil::CollectAppTelemetry(int32_t pid, AppExecFwk::AppTelemetryLeakType atlt, const GpuHookSize &hookSize)
{
    auto appMgrClient = std::make_unique<AppMgrClient>();
    auto connectResult = appMgrClient->ConnectAppMgrService();
    if (connectResult != AppMgrResultCode::RESULT_OK) {
        HIVIEW_LOGE("ConnectAppMgrService Failed, result: %{public}d", connectResult);
        return;
    }

    OHOS::AppExecFwk::AppFaultDataBySA faultData;
    faultData.faultType = FaultDataType::APP_TELEMETRY;
    faultData.pid = pid;
    faultData.notifyApp = true;
    faultData.atLeakType = atlt;
    faultData.gpuHookSize = SerializeHookSizeToJson(hookSize);
    HIVIEW_LOGD("faultData.gpuHookSize content: %{public}s", faultData.gpuHookSize.c_str());
    HIVIEW_LOGI("ready NotifyAppFaultBySA, pid: %{public}d, atlt: %{public}d, gpuHookSize length: %{public}zu",
                pid, atlt, faultData.gpuHookSize.length());
    int32_t resultCode = appMgrClient->NotifyAppFaultBySA(faultData);
    if (resultCode != AppMgrResultCode::RESULT_OK) {
        HIVIEW_LOGE("NotifyAppFaultBySA failed");
    }
}

bool DfrUtil::ForkProcessToDumpSmaps(pid_t pid, CommonDumpType dumpType, const string &detectorTypeName)
{
    int writeFd = 0;
    int ret;
    string realPath = "/data/log/reliability/resource_leak/memory_leak/telemetryTemp";

    uint32_t fileAction = O_CREAT | O_RDWR | O_APPEND;
    writeFd = open(realPath.c_str(), fileAction, DEFAULT_LOG_FILE_MODE);
    if (writeFd < 0) {
        HIVIEW_LOGE("%{public}s failed to open %{public}s, err: %{public}s", detectorTypeName.c_str(), realPath.c_str(),
                    strerror(errno));
        return false;
    }

    if (!(writeFd > 0)) {
        return false;
    }
    fdsan_exchange_owner_tag(writeFd, 0, FDSAN_TAG);
    ret = dup2(writeFd, STDIN_FILENO) + dup2(writeFd, STDOUT_FILENO) + dup2(writeFd, STDERR_FILENO);
    fdsan_close_with_tag(writeFd, FDSAN_TAG);
    if (ret < 0) {
        HIVIEW_LOGE("%{public}s dup2 writeFd fail, err: %{public}s", detectorTypeName.c_str(), strerror(errno));
        return false;
    }
    return ForkProcessAndDump(pid, dumpType, detectorTypeName);
}

bool DfrUtil::ForkProcessAndDump(pid_t pid, CommonDumpType dumpType, const std::string &detectorTypeName)
{
    pid_t childPid = fork();
    if (childPid < 0) {
        HIVIEW_LOGE("%{public}s failed to fork process, err: %{public}s", detectorTypeName.c_str(), strerror(errno));
        return false;
    }
    if (childPid == 0) {
        int ret = ExecuteChildProcess(pid, dumpType);
        if (ret < 0) {
            HIVIEW_LOGE("%{public}s execl return %{public}d, type is %{public}s, err: %{public}s",
                        detectorTypeName.c_str(), ret, CommonDumpTypeStr.at(dumpType).c_str(), strerror(errno));
        }
        _exit(ret);
    } else {
        if (waitpid(childPid, nullptr, 0) != childPid) {
            HIVIEW_LOGE("%{public}s waitpid fail, pid: %{public}d, err: %{public}s", detectorTypeName.c_str(), childPid,
                        strerror(errno));
            return false;
        }
        HIVIEW_LOGI("%{public}s waitpid %{public}d success", detectorTypeName.c_str(), childPid);
    }
    return true;
}

int DfrUtil::ExecuteChildProcess(pid_t pid, CommonDumpType dumpType)
{
    int ret = 0;
    if (dumpType & SMAPS_INFO) {
        ret = execl("/system/bin/hidumper", "hidumper", "--mem-smaps", to_string(pid).c_str(), nullptr);
        return ret;
    }
    if ((dumpType & RENDER_SERVICE_MEM) && DfrUtil::GetInstance().ShouldDumpRs()) {
        ret = execl("/system/bin/hidumper", "hidumper", "-s", "10", "-a", "dumpMem", nullptr);
        return ret;
    }
    if (dumpType & AVC_MEM) {
        ret = execl("/system/bin/hidumper", "hidumper", "-s", "AVCodecService", nullptr);
        return ret;
    }
    if (dumpType & MEDIA_MEM) {
        ret = execl("/system/bin/hidumper", "hidumper", "-s", "3002", "-a", "player", nullptr);
        return ret;
    }
    HIVIEW_LOGW("Attention! pid %{public}d want to dump invalid type: %{public}d", pid, dumpType);
    return -1;
}

bool DfrUtil::WaitChildWithTimeout(pid_t childPid, int &status, const char *caller)
{
    bool finished = false;
    const int timeSliceUs = 100000;
    const int totalTimeoutUs = 5000000;
    for (int elapsedUs = 0; elapsedUs < totalTimeoutUs; elapsedUs += timeSliceUs) {
        pid_t wPid = waitpid(childPid, &status, WNOHANG);
        if (wPid == childPid) {
            HIVIEW_LOGI("%{public}s child exit=%{public}d", caller, WEXITSTATUS(status));
            finished = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(timeSliceUs));
    }
    if (!finished) {
        kill(childPid, SIGTERM);
        HIVIEW_LOGW("%{public}s child timeout, kill childPid=%{public}d", caller, childPid);
    }
    return finished;
}

int DfrUtil::GetWinIdByPid(pid_t pid)
{
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        HIVIEW_LOGE("pipe failed, errno=%{public}d", errno);
        return -1;
    }

    fdsan_exchange_owner_tag(pipefd[0], 0, FDSAN_TAG);
    fdsan_exchange_owner_tag(pipefd[1], 0, FDSAN_TAG);
    pid_t childPid = fork();
    if (childPid < 0) {
        HIVIEW_LOGE("fork failed, errno=%{public}d", errno);
        fdsan_close_with_tag(pipefd[0], FDSAN_TAG);
        fdsan_close_with_tag(pipefd[1], FDSAN_TAG);
        return -1;
    }

    if (childPid == 0) {
        close(STDIN_FILENO);
        fdsan_close_with_tag(pipefd[0], FDSAN_TAG);
        // Close all inherited file descriptors beyond stderr (stdin/stdout/stderr)
        // to prevent fd leaks from the parent process
        for (int i = STDERR_FILENO + 1; i < _SC_OPEN_MAX; i++) {
            close(i);
        }
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
            _exit(1);
        }
        fdsan_close_with_tag(pipefd[1], FDSAN_TAG);
        execl("/system/bin/hidumper", "hidumper", "-s", "WindowManagerService", "-a", "-a", nullptr);
        _exit(1);
    }

    fdsan_close_with_tag(pipefd[1], FDSAN_TAG);

    string output;
    char buffer[PIPE_READ_BUF_SIZE] = {0};
    ssize_t n;
    while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, n);
    }
    fdsan_close_with_tag(pipefd[0], FDSAN_TAG);

    int status;
    WaitChildWithTimeout(childPid, status, "GetWinIdByPid");

    return ParseWinIdFromOutput(output, pid);
}

int DfrUtil::ParseWinIdFromOutput(const string &output, pid_t pid)
{
    int32_t minWinId = INT32_MAX;
    bool startFlag = false;
    istringstream stream(output);
    string line;
    while (getline(stream, line)) {
        if (line.empty()) {
            continue;
        }
        if (line.find("WindowName") != string::npos) {
            startFlag = true;
            continue;
        }
        if (!startFlag) {
            continue;
        }
        std::stringstream lineSs(line);
        std::string field;
        std::vector<std::string> fields;
        while (lineSs >> field) {
            fields.push_back(field);
        }
        int winPid;
        int winId;
        if (fields.size() >= WIN_ID_LENGTH && SafeStoi(fields[PID_INDEX], winPid) &&
            SafeStoi(fields[WIN_ID_INDEX], winId)) {
            if (winPid == pid && winId >= 0) {
                minWinId = std::min(minWinId, winId);
            }
        }
        size_t endFlag = line.find("Focus window: ");
        if (endFlag != string::npos) {
            break;
        }
    }
    return (minWinId == INT32_MAX) ? -1 : minWinId;
}

bool DfrUtil::DumpUiTree(int fd, int &winId)
{
    pid_t childPid = fork();
    if (childPid < 0) {
        HIVIEW_LOGE("DumpUiTree fork failed, errno=%{public}d", errno);
        return false;
    }
    if (childPid == 0) {
        close(STDIN_FILENO);
        for (int i = STDERR_FILENO + 1; i < _SC_OPEN_MAX; i++) {
            if (i != fd) {
                close(i);
            }
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            _exit(1);
        }
        string winArg = to_string(winId);
        string argStr = "-w " + winArg + " -default";
        execl("/system/bin/hidumper", "hidumper", "-s", "WindowManagerService",
              "-a", argStr.c_str(), nullptr);
        _exit(1);
    }
    int status;
    if (!WaitChildWithTimeout(childPid, status, "DumpUiTree")) {
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool DfrUtil::DumpRawheapInfo(OHOS::AppExecFwk::JsHeapDumpInfo &info)
{
    OHOS::sptr<OHOS::ISystemAbilityManager> systemAbilityManager =
        OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        HIVIEW_LOGE("DumpRawheapInfo: GetSystemAbilityManager is nullptr.");
        return false;
    }
    OHOS::sptr<OHOS::AppExecFwk::IAppMgr> appManager =
        OHOS::iface_cast<OHOS::AppExecFwk::IAppMgr>(systemAbilityManager->GetSystemAbility(OHOS::APP_MGR_SERVICE_ID));
    if (appManager == nullptr) {
        HIVIEW_LOGE("DumpJsHeapMemory: Get the appManager is nullptr.");
        return false;
    }
    int ret = appManager->DumpJsHeapMemory(info);
    if (ret != 0) {
        HIVIEW_LOGE("DumpJsHeapMemory return failed, ret is:%{public}d", ret);
        return false;
    }
    return true;
}

std::string DfrUtil::GetLeakTypeName(uint64_t leakType, uint64_t subLeakType)
{
    auto it = LeakTypeStr.find(leakType);
    if (it == LeakTypeStr.end()) {
        return "";
    }
    std::string result = it->second;

    if ((leakType == RSS_LEAK_TYPE || leakType == EXT_PSS_LEAK_TYPE) && subLeakType != 0) {
        auto subIt = SubLeakTypeStr.find(static_cast<int>(subLeakType));
        if (subIt != SubLeakTypeStr.end()) {
            result += "-" + subIt->second;
        }
    }

    return result;
}

// for dump arkweb-v8 & jsvm heap
bool DfrUtil::DumpHeapMemory(OHOS::AppExecFwk::MemDumpInfo &info)
{
    bool isDumping = isDumping_.load();
    if (isDumping) {
        // limit concurrency to 1
        HIVIEW_LOGI("DumpHeapMemory is dumping");
        return false;
    }
    OHOS::sptr<OHOS::ISystemAbilityManager> systemAbilityManager =
        OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        HIVIEW_LOGE("DumpHeapMemory: Get the systemAbilityManager is nullptr.");
        return false;
    }
    OHOS::sptr<OHOS::AppExecFwk::IAppMgr> appManager =
        OHOS::iface_cast<OHOS::AppExecFwk::IAppMgr>(systemAbilityManager->GetSystemAbility(OHOS::APP_MGR_SERVICE_ID));
    if (appManager == nullptr) {
        HIVIEW_LOGE("DumpHeapMemory: Get the appManager is nullptr.");
        return false;
    }
    isDumping_.store(true);
    int ret = appManager->DumpMem(info, nullptr);
    if (ret != ERR_OK) {
        HIVIEW_LOGE("DumpHeapMemory return failed, ret is:%{public}d", ret);
        isDumping_.store(false);
        return false;
    }
    isDumping_.store(false);
    return true;
}

// for arkweb-v8 & jsvm leak test temporarily
bool DfrUtil::SkipForegroundDetectlimit()
{
    bool isSkip = false;
    string value = system::GetParameter(NATIVE_LEAK_DETECT_PROPERTY, "");
    if (value == "enable" || value == "true") {
        isSkip = true;
    }
    return isSkip;
}

void DfrUtil::DumpAppPageInfo(pid_t pid, const std::string &processName, std::ofstream &fout) const
{
    if (!DfrUtil::GetInstance().IsApp(pid)) {
        HIVIEW_LOGI("pid=%{public}d, processName=%{public}s, is not APP", pid, processName.c_str());
        return;
    }
    std::string eventName = "PROCESS_PAGE_INFO";
    SysEventCreator sysEventCreator("PERFORMANCE", eventName, SysEventCreator::FAULT);
    sysEventCreator.SetKeyValue("PID", pid);
    sysEventCreator.SetKeyValue("PROCESS_NAME", processName);

    auto sysEvent = std::make_shared<SysEvent>(eventName, nullptr, sysEventCreator);
    std::shared_ptr<Event> event = std::dynamic_pointer_cast<Event>(sysEvent);

    auto &hiviewInstance = HiviewGlobal::GetInstance();
    if (!hiviewInstance) {
        HIVIEW_LOGE("pid=%{public}d, processName=%{public}s, HiviewGlobal instance is null", pid, processName.c_str());
        return;
    }

    if (!hiviewInstance->PostSyncEventToTarget("Faultlogger", event)) {
        HIVIEW_LOGE("pid=%{public}d, processName=%{public}s, PostSyncEventToTarget ERROR", pid, processName.c_str());
        return;
    }

    string pageInfo = sysEvent->GetEventValue("PAGE_INFO");
    pageInfo = ReplaceStr(pageInfo, "\\n", "\n");
    HIVIEW_LOGI("pid=%{public}d, processName=%{public}s, pageInfo=%{public}s",
        pid, processName.c_str(), pageInfo.c_str());

    fout << "*****************************" << endl;
    fout << "APP_PAGE_HISTORY_INFO" << endl;
    fout << pageInfo;
    fout << "************ endl ************" << endl;
}

std::string DfrUtil::GenerateMetaInfoJson(const MetaInfoParams &params)
{
    std::string productModel = GetProductModelStr();
    std::string sysVersion = GetSysVersionStr();
    cJSON *root = cJSON_CreateObject();
    if (root == nullptr) {
        HIVIEW_LOGE("Failed to create cJSON object");
        return "";
    }
    cJSON_AddStringToObject(root, "app_version_name", params.versionName.c_str());
    cJSON_AddStringToObject(root, "app_version_code", params.versionCode.c_str());
    std::string romVersion = productModel + " " + sysVersion;
    cJSON_AddStringToObject(root, "rom_version", romVersion.c_str());
    cJSON_AddStringToObject(root, "os_release_type", IsBetaOrDevelopMode() ? "Beta" : "Release");
    cJSON_AddStringToObject(root, "device_model", productModel.c_str());
    cJSON_AddStringToObject(root, "device_type", deviceType_.c_str());
    cJSON_AddStringToObject(root, "foreground", params.foreground.c_str());
    cJSON_AddStringToObject(root, "os_version", sysVersion.c_str());
    cJSON_AddStringToObject(root, "happen_time", params.timeStamp.c_str());
    cJSON_AddStringToObject(root, "module_name", params.processName.c_str());
    cJSON_AddStringToObject(root, "app_running_id", params.appRunningId.c_str());
    cJSON_AddStringToObject(root, "first_frame", params.firstFrame.c_str());
    cJSON_AddStringToObject(root, "second_frame", params.secondFrame.c_str());
    cJSON_AddStringToObject(root, "third_frame", params.thirdFrame.c_str());
    char* jsonStr = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (jsonStr == nullptr) {
        HIVIEW_LOGE("Failed to create JSON string");
        return "";
    }
    std::string result(jsonStr);
    free(jsonStr);
    return result;
}

bool DfrUtil::CreateMetaInfoFile(const MetaInfoParams &params)
{
    std::string metaFileName = params.eventName + "-metainfo-" + params.processName + "-" +
                              to_string(params.pid) + "-" + params.timeStamp + ".txt";
    std::string filePath = params.basePath + "/" + metaFileName;
    std::string realPath = RealPath(filePath);
    if (realPath.empty()) {
        HIVIEW_LOGE("Failed to get real path: %{public}s", filePath.c_str());
        return false;
    }
    TryToGetDeviceType();
    std::ofstream fout(realPath);
    if (!fout.is_open()) {
        HIVIEW_LOGE("Failed to create meta info file: %{public}s", realPath.c_str());
        return false;
    }
    std::string jsonStr = GenerateMetaInfoJson(params);
    if (jsonStr.empty()) {
        fout.close();
        HIVIEW_LOGE("Failed to generate meta info JSON");
        return false;
    }
    fout << jsonStr;
    fout.close();
    HIVIEW_LOGI("Create meta info file: %{public}s", filePath.c_str());
    return true;
}

std::string DfrUtil::GetProductModelStr()
{
    const char *productModel = GetProductModel();
    if (productModel == nullptr) {
        return "";
    }
    return std::string(productModel);
}

std::string DfrUtil::GetSysVersionStr()
{
    const char *displayVersion = GetDisplayVersion();
    if (displayVersion == nullptr) {
        return "";
    }
    std::string displayedVersionStr(displayVersion);
    auto pos = displayedVersionStr.find(' ');
    if (pos == displayedVersionStr.npos) {
        return displayedVersionStr;
    }
    return displayedVersionStr.substr(pos + 1);
}

int32_t DfrUtil::Int64ToInt32(int64_t value)
{
    if (value > INT32_MAX) {
        return INT32_MAX;
    }
    if (value < INT32_MIN) {
        return INT32_MIN;
    }
    return static_cast<int32_t>(value);
}
}  // namespace HiviewDFX
}  // namespace OHOS
