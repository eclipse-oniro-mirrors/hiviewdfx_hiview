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
constexpr size_t UUID_LEN = 100;
}

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "FaultLeakDetectorUtil");

namespace {
constexpr int KB_PER_PAGE = 4;
constexpr int PID_KTHREADD = 2;
constexpr int HAS_LEN = 128;
constexpr int VALUE_MOD = 200000;
constexpr int KB_TO_GB_BITS = 20;
constexpr int MAP_ELEMT_OLD_TIME = 5;  // second
constexpr int DECIMAL_BASE = 10;
constexpr int LOWMEM_THRESHOLD = 800 * KB_PER_MB;        // 800 MB
constexpr int LOWMEM_THRESHOLD_WATCH = 200 * KB_PER_MB;  // 200 MB
constexpr uint32_t MAX_DUMP_TRACE_LIMIT = 15;
constexpr int PREF_COLLECT_TIME = 5;  // second
constexpr int GPU_MEMERY_ELEMENT_LENGTH = 5;
constexpr int GPU_MEMERY_PID_INDEX = 2;
constexpr int GPU_MEMERY_MEM_INDEX = 3;

// index to split /proc/pid/stat info begin ")" with  " "
enum StatIndex {
    MIN_INDEX = 0,
    PPID_INDEX,
    GRUOP_INDEX,
    RUN_TIME_INDEX = 11,  // the time of  process run from start.
    START_TIME_INDEX = 19,
    VSS_INDEX = 20,
    RSS_INDEX = 21,
    SWAPPSS_INDEX = 33,
    MAX_INDEX,
};

/*
 The abbreviation of the dump type function, used for config configuration.
 Note: When adding a new dump function, ensure that each abbreviation is unique and not repeated.
*/
std::map<std::string, CommonDumpType> COMMON_DUMP_FUNCTION_MAP{
    {"sm", SMAPS_INFO},
    {"rd", RENDER_SERVICE_MEM},
    {"am", ASHMEM_PROCESS_INFO},
    {"in", ION_PROCESS_INFO},
    {"gu", GPU_PROCESS_INFO},
    {"pr", PROFILER},
    {"nd", NMD},
    {"sh", SYSTEM_HEAP},
    {"inp", ION_NEW_PROC},
    {"si", SLAB_INFO},
    {"ci", CMA_INFO},
    {"vi", VMALLOC_INFO},
    {"sv", SYSVIEW},
    {"me", MEDIA_MEM},
    {"av", AVC_MEM},
};

}

DfrUtil::DfrUtil() : totalRamSize_(0)
{
    totalRamSize_ = GetTotalRamSize();
}

DfrUtil::~DfrUtil() {}

bool DfrUtil::IsMemLeakEnable()
{
    if (memLeakState_ == UNKNOWN) {
        string value = system::GetParameter(MEMORY_LEAK_ENABLE_PROPERTY, "");
        if (value == "disable" || value == "false") {
            memLeakState_ = STATE_FALSE;
        } else {
            memLeakState_ = STATE_TRUE;
        }
    }
    return memLeakState_ == STATE_TRUE;
}

bool DfrUtil::IsMemTestEnable()
{
    if (memTestState_ == UNKNOWN) {
        string value = system::GetParameter(MEMORY_LEAK_TEST_PROPERTY, "");
        if (value == "enable" || value == "true") {
            memTestState_ = STATE_TRUE;
        } else {
            memTestState_ = STATE_FALSE;
        }
    }
    return memTestState_ == STATE_TRUE;
}

bool DfrUtil::IsFdLeakEnable()
{
    if (fdLeakState_ == UNKNOWN) {
        string value = system::GetParameter(FD_LEAK_ENABLE_PROPERTY, "");
        if (value == "disable" || value == "false") {
            fdLeakState_ = STATE_FALSE;
        } else {
            fdLeakState_ = STATE_TRUE;
        }
    }
    return fdLeakState_ == STATE_TRUE;
}

bool DfrUtil::IsFdTestEnable()
{
    if (fdTestState_ == UNKNOWN) {
        string value = system::GetParameter(FD_LEAK_TEST_PROPERTY, "");
        if (value == "enable" || value == "true") {
            fdTestState_ = STATE_TRUE;
        } else {
            fdTestState_ = STATE_FALSE;
        }
    }
    return fdTestState_ == STATE_TRUE;
}

bool DfrUtil::IsThreadLeakEnable()
{
    if (threadLeakState_ == UNKNOWN) {
        string value = system::GetParameter(THREAD_LEAK_ENABLE_PROPERTY, "");
        if (value == "disable" || value == "false") {
            threadLeakState_ = STATE_FALSE;
        } else {
            threadLeakState_ = STATE_TRUE;
        }
    }
    return threadLeakState_ == STATE_TRUE;
}

bool DfrUtil::IsThreadTestEnable()
{
    if (threadTestState_ == UNKNOWN) {
        string value = system::GetParameter(THREAD_LEAK_TEST_PROPERTY, "");
        if (value == "enable" || value == "true") {
            threadTestState_ = STATE_TRUE;
        } else {
            threadTestState_ = STATE_FALSE;
        }
    }
    return threadTestState_ == STATE_TRUE;
}

bool DfrUtil::IsHilogBufferSizeModify()
{
    string propertyValue = system::GetParameter(HILOG_BUFFER_SIZE_PROPERTY, "");
    if (propertyValue.empty()) {
        return false;
    }
    HIVIEW_LOGI("Attention: hilog is modify buffer size, ignore it");
    return true;
}

bool DfrUtil::GetIsHmKernel()
{
    if (isHmKernel_ == UNKNOWN) {
        string meminfoPath = "/proc/memview";
        auto fd = open(meminfoPath.c_str(), O_RDONLY);
        if (fd < 0) {
            HIVIEW_LOGE("open /proc/memview failed, may be linux kernel, err: %{public}s", strerror(errno));
            isHmKernel_ = STATE_FALSE;
        } else {
            fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
            isHmKernel_ = STATE_TRUE;
            fdsan_close_with_tag(fd, FDSAN_TAG);
        }
    }
    return isHmKernel_ == STATE_TRUE;
}

int32_t DfrUtil::GetFocusMode()
{
    auto focusMode = system::GetIntParameter(PARAM_PHONE_FOCUS_MODE, 0);
    return focusMode;
}

vector<string> DfrUtil::Split(const string &str, char delim)
{
    istringstream ss(str);
    vector<string> elems;
    for (string item; getline(ss, item, delim);) {
        if (item.empty()) {
            continue;
        }
        elems.push_back(item);
    }
    return elems;
}

CommonDumpPathList DfrUtil::ParseDumpFunction(string &dumpFunctions, string defaultPath)
{
    CommonDumpPathList pathMap;
    // split dumpFunctions
    vector<string> dumpFunctionList = Split(dumpFunctions, ',');
    for (auto dumpTypeStr : dumpFunctionList) {
        // store path
        CommonDumpType dumpType = COMMON_DUMP_FUNCTION_MAP[dumpTypeStr];
        pathMap.push_back(make_pair(dumpType, defaultPath));
        HIVIEW_LOGD("%{public}s path is %{public}s", CommonDumpTypeStr.at(dumpType).c_str(), defaultPath.c_str());
    }
    return pathMap;
}

string DfrUtil::GetProcPath(string meminfoName, string procName)
{
    if (GetIsHmKernel() && meminfoName.find("LDK") != meminfoName.npos) {
        return "/proc/devhost/root/" + procName;
    }
    return "/proc/" + procName;
}

bool DfrUtil::SafeStoi(const string &str, int &value)
{
    long long tmp;
    if (!SafeStoll(str, tmp)) {
        HIVIEW_LOGE("failed to Stoll. str: %{public}s", str.c_str());
        return false;
    }

    if (tmp > INT_MAX || tmp < INT_MIN) {
        HIVIEW_LOGE("out of range. tmp: %{public}lld, str: %{public}s", tmp, str.c_str());
        return false;
    }

    value = static_cast<int>(tmp);
    HIVIEW_LOGD("success, str: %{public}s, value: %{public}d", str.c_str(), value);
    return true;
}

bool DfrUtil::SafeStoll(const string &str, long long &value)
{
    value = 0;
    size_t start = 0;
    bool isNegative = false;

    if (str.empty()) {
        HIVIEW_LOGE("str is empty.");
        return false;
    }

    if (str[0] == '-') {
        isNegative = true;
        start = 1;
    } else if (str[0] == '+') {
        start = 1;
    }

    for (size_t i = start; i < str.size(); ++i) {
        if (!isdigit(str[i])) {
            HIVIEW_LOGE("digit check failed. str: %{public}s, index: %{public}zu", str.c_str(), i);
            return false;
        }
        if (value > (LLONG_MAX - (str[i] - '0')) / DECIMAL_BASE) {
            HIVIEW_LOGE("out of range, str: %{public}s", str.c_str());
            return false;
        }
        value = value * DECIMAL_BASE + (str[i] - '0');
    }

    if (isNegative) {
        value = -value;
    }

    HIVIEW_LOGD("success, str: %{public}s, result: %{public}lld", str.c_str(), value);
    return true;
}

bool DfrUtil::IsFileEmpty(const string &path)
{
    string realPath = RealPath(path);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath path: %{public}s", path.c_str());
        return false;
    }
    ifstream fin(realPath);
    if (!fin.is_open()) {
        HIVIEW_LOGE("%{public}s file is empty", realPath.c_str());
        return true;
    }
    if (fin.peek() == ifstream::traits_type::eof()) {
        HIVIEW_LOGE("%{public}s file is empty", realPath.c_str());
        return true;
    }
    return false;
}

bool DfrUtil::RemoveFile(const string &path)
{
    string realPath = RealPath(path);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath path: %{public}s", path.c_str());
        return false;
    }
    if (remove(path.c_str()) != 0) {
        HIVIEW_LOGE("failed to remove file: %{public}s, error: %{public}s", path.c_str(), strerror(errno));
        return false;
    }
    return true;
}

bool DfrUtil::CopyFile(const string &path, const string &newPath)
{
    string realPath = RealPath(path);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath path: %{public}s", path.c_str());
        return false;
    }
    string realNewPath = RealPath(newPath);
    if (realNewPath.empty()) {
        HIVIEW_LOGE("failed realpath path: %{public}s", newPath.c_str());
        return false;
    }
    ifstream in(realPath);
    if (!in.is_open()) {
        HIVIEW_LOGE("failed to open:%{public}s, error: %{public}s", realPath.c_str(), strerror(errno));
        return false;
    }
    ofstream out(realNewPath);
    if (!out.is_open()) {
        HIVIEW_LOGE("failed to open:%{public}s, error: %{public}s", realNewPath.c_str(), strerror(errno));
        in.close();
        return false;
    }
    out << in.rdbuf();
    in.close();
    out.close();
    return true;
}

bool DfrUtil::CopyFileNoReal(const string &path, const string &newPath)
{
    ifstream in(path);
    if (!in.is_open()) {
        HIVIEW_LOGE("failed to open:%{public}s, error: %{public}s", path.c_str(), strerror(errno));
        return false;
    }
    ofstream out(newPath);
    if (!out.is_open()) {
        HIVIEW_LOGE("failed to open:%{public}s, error: %{public}s", newPath.c_str(), strerror(errno));
        in.close();
        return false;
    }
    out << in.rdbuf();
    in.close();
    out.close();
    return true;
}

bool DfrUtil::RenameFile(const string &path, const string &newPath)
{
    if (rename(path.c_str(), newPath.c_str())) {
        HIVIEW_LOGE("failed to move:%{public}s to %{public}s, err: %{public}s", path.c_str(), newPath.c_str(),
                    strerror(errno));
        return false;
    }
    HIVIEW_LOGE("succeed to move:%{public}s to %{public}s, err: %{public}s", path.c_str(), newPath.c_str(),
                strerror(errno));
    return true;
}

int32_t DfrUtil::GetAppUserId(int32_t pid)
{
    return GetProcessUid(pid) / VALUE_MOD;
}

bool DfrUtil::CreateDir(const string &path, mode_t mode)
{
    if (IsDirectory(path)) {
        // ensure we have correct perms
        chmod(path.c_str(), mode);
        HIVIEW_LOGI("directory already exist: %{public}s", path.c_str());
        return true;
    }
    if (mkdir(path.c_str(), mode) != 0) {
        HIVIEW_LOGE("failed to create dir: %{public}s", path.c_str());
        return false;
    }
    return true;
}

bool DfrUtil::FileExist(const string &path)
{
    return (access(path.c_str(), F_OK) == 0);
}

// the parameter of path should be full.
bool DfrUtil::IsDirectory(const string &path)
{
    struct stat statBuffer;
    if (stat(path.c_str(), &statBuffer) == 0 && S_ISDIR(statBuffer.st_mode)) {
        return true;
    }
    return false;
}

vector<string> DfrUtil::GetSubNodes(const string &path, bool digit)
{
    vector<string> subNodes;
    auto dir = opendir(path.c_str());
    if (dir == nullptr) {
        return subNodes;
    }
    for (struct dirent *ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
        string childNode = ent->d_name;
        if (childNode == "." || childNode == "..") {
            continue;
        }
        if (digit && !isdigit(childNode[0])) {
            continue;
        }
        subNodes.push_back(childNode);
    }
    closedir(dir);
    return subNodes;
}

vector<string> DfrUtil::GetSubFile(const string &path, bool digit)
{
    vector<string> subFiles;
    auto dir = opendir(path.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGE("failed to open dir: %{public}s, err: %{public}s", path.c_str(), strerror(errno));
        return subFiles;
    }
    for (struct dirent *ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
        string childNode = ent->d_name;
        if (childNode == "." || childNode == "..") {
            continue;
        }
        if (digit && !isdigit(childNode[0])) {
            continue;
        }
        if (IsDirectory(path + "/" + childNode)) {
            continue;  // skip directory
        }
        subFiles.push_back(childNode);
    }
    closedir(dir);
    return subFiles;
}

vector<string> DfrUtil::GetSubDir(const string &path, bool digit)
{
    vector<string> subDirs;
    auto dir = opendir(path.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGE("failed to open dir: %{public}s, err: %{public}s", path.c_str(), strerror(errno));
        return subDirs;
    }
    for (struct dirent *ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
        string childNode = ent->d_name;
        if (childNode == "." || childNode == "..") {
            continue;
        }
        if (digit && !isdigit(childNode[0])) {
            continue;
        }
        if (!IsDirectory(path + "/" + childNode)) {
            continue;  // skip directory
        }
        subDirs.push_back(childNode);
    }
    closedir(dir);
    return subDirs;
}

string DfrUtil::ReadFileByChar(const string &path)
{
    ifstream fin(path);
    if (!fin.is_open()) {
        return "";
    }
    string content;
    char c;
    while (!fin.eof()) {
        fin >> noskipws >> c;
        if (c == '\0' || c == '\n') {
            break;
        }
        content += c;
    }
    fin.close();
    return content;
}

string DfrUtil::GetProcessName(pid_t pid)
{
    string path = "/proc/" + to_string(pid) + "/cmdline";
    string name = ReadFileByChar(path);
    // cmdline is empty ? use comm instead
    if (name.length() < 2) {  // we consider namelen < 2 as empty.
        path = "/proc/" + to_string(pid) + "/comm";
        name = ReadFileByChar(path);
    }
    auto pos = name.find_last_not_of(" \n\r\t");
    if (pos != string::npos) {
        name.erase(pos + 1);
    }
    HIVIEW_LOGD("name: %{public}s.", name.c_str());
    pos = name.find_last_of("/");
    if (pos != string::npos) {
        name = name.substr(pos + 1);
    }
    return name;
}

int DfrUtil::GetProcessAdj(pid_t pid)
{
    int processAdj = INT_MAX;
    string path = "/proc/" + to_string(pid) + "/oom_score_adj";
    std::ifstream file(path);
    if (!file.is_open()) {
        HIVIEW_LOGE("can't open file : %{public}s", path.c_str());
        return processAdj;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::string data = buffer.str();
    data.erase(std::remove_if(data.begin(), data.end(), ::isspace), data.end());
    if (!SafeStoi(data, processAdj)) {
        HIVIEW_LOGE("failed to stoi, str: %{public}s", data.c_str());
    }
    return processAdj;
}

vector<pid_t> DfrUtil::GetAllPids()
{
    string path = "/proc";
    vector<string> allPids = GetSubDir(path, true);
    vector<pid_t> pids;
    for (const auto &pid : allPids) {
        if (!isdigit(pid[0])) {
            continue;
        }
        int pidInt;
        if (!ConvertToInt(pid, pidInt)) {
            continue;
        }
        pids.push_back(pidInt);
    }
    return pids;
}

time_t DfrUtil::StrToTimeStamp(const string &tmStr)
{
    string stTime = tmStr;
    struct tm tmFormat;
    strptime(stTime.c_str(), "%Y/%m/%d %H:%M:%S", &tmFormat);
    tmFormat.tm_isdst = -1;
    time_t timeStamp = mktime(&tmFormat);
    return timeStamp;
}

time_t DfrUtil::StrToTimeStampT(const string &tmStr)
{
    string stTime = tmStr;
    struct tm tmFormat;
    strptime(stTime.c_str(), "%Y%m%d%H%M%S", &tmFormat);
    tmFormat.tm_isdst = -1;
    time_t timeStamp = mktime(&tmFormat);
    return timeStamp;
}

string DfrUtil::TimeStampToStr(const time_t timeStamp)
{
    tm tm;
    const int timeLength = 64;
    char stampStr[timeLength] = {0};

    if (localtime_r(&timeStamp, &tm) == nullptr || strftime(stampStr, timeLength, "%Y/%m/%d %H:%M:%S", &tm) == 0) {
        HIVIEW_LOGE("failed to get real time");
        return "error time format!";
    }
    return string(stampStr);
}

string DfrUtil::TimeStampToSimplelyStr(const time_t timeStamp)
{
    tm tm;
    const int timeLength = 64;
    char stampStr[timeLength] = {0};

    if (localtime_r(&timeStamp, &tm) == nullptr || strftime(stampStr, timeLength, "%Y%m%d%H%M%S", &tm) == 0) {
        HIVIEW_LOGE("failed to get real time");
        return "error time format!";
    }
    return string(stampStr);
}

string DfrUtil::GetRealTime()
{
    time_t now = time(nullptr);
    return TimeStampToStr(now);
}

string DfrUtil::GetRealTimeStampStr()
{
    time_t now = time(nullptr);
    return TimeStampToSimplelyStr(now);
}

vector<string> DfrUtil::GetStatInfo(const string &path)
{
    string statInfo = ReadFileByChar(path);
    // process_name was included in pair (), find ")" as start for skip speical in process_name.
    size_t pos = statInfo.find(')');
    if (pos != string::npos) {
        statInfo = statInfo.substr(++pos);
    }
    return Split(statInfo, ' ');
}

time_t DfrUtil::GetProcessStartTime(int pid)
{
    string path = "/proc/" + to_string(pid) + "/stat";
    vector<string> statInfo = GetStatInfo(path);
    if (statInfo.size() < MAX_INDEX) {
        return -1;
    }
    string startTime = statInfo[START_TIME_INDEX];
    long long value;
    if (!SafeStoll(startTime, value)) {
        HIVIEW_LOGE("failed to get startTime, startTime: %{public}s, pid: %{public}d", startTime.c_str(), pid);
        return -1;
    }
    return static_cast<time_t>(value);
}

int32_t DfrUtil::GetSwapPssByPid(pid_t pid)
{
    string path = "/proc/" + to_string(pid) + "/stat";
    vector<string> statInfo = GetStatInfo(path);
    if (statInfo.size() < MAX_INDEX) {
        return -1;
    }
    string swapPss = statInfo[SWAPPSS_INDEX];
    if (!isdigit(swapPss[0])) {
        HIVIEW_LOGE("failed to get process swapPss, reason: not digital, pid: %d", pid);
        return -1;
    }
    int swapPssInt;
    if (!ConvertToInt(swapPss, swapPssInt)) {
        return -1;
    }
    return swapPssInt * KB_PER_PAGE;
}

int32_t DfrUtil::GetSwapRssByPid(pid_t pid)
{
    string path = "/proc/" + to_string(pid) + "/status";
    ifstream fin;
    fin.open(path.c_str());
    if (!fin.is_open()) {
        HIVIEW_LOGE("failed to open: %{public}s, err: %{public}s", path.c_str(), strerror(errno));
        return -1;
    }
    string line;
    while (getline(fin, line)) {
        if (line.find("VmSwap:") == line.npos) {
            continue;
        }
        size_t pos = line.find("\t");
        if (pos == line.npos) {
            fin.close();
            return -1;
        }
        line = line.substr(pos + 1);
        // VmSwap value format: "  <value> kB", strip " kB" suffix and leading spaces
        size_t kBPos = line.find(" kB");
        if (kBPos == line.npos) {
            fin.close();
            return -1;
        }
        line.erase(kBPos);
        line.erase(0, line.find_first_not_of(" "));
        int value;
        if (!ConvertToInt(line, value)) {
            fin.close();
            return -1;
        }
        fin.close();
        return value;
    }
    fin.close();
    return -1;
}

uint64_t DfrUtil::GetProcessVss(int pid)
{
    string path = "/proc/" + to_string(pid) + "/stat";
    vector<string> statInfo = GetStatInfo(path);
    if (statInfo.size() < MAX_INDEX) {
        return 0;
    }
    string vss = statInfo[VSS_INDEX];
    if (!isdigit(vss[0])) {
        HIVIEW_LOGE("failed to get process vss, reason: not digital, pid: %d", pid);
        return 0;
    }
    uint64_t value;
    if (!ConvertToULL(vss, value)) {
        HIVIEW_LOGE("failed to get process vss, reason: not digital, pid: %d", pid);
        return 0;
    }
    return value / BYTE_PER_KB;
}

uint64_t DfrUtil::GetProcessRss(int pid)
{
    string traceStr(__func__);
    traceStr.append(":").append(to_string(pid));
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    string path = "/proc/" + to_string(pid) + "/rss";
    string rssString = ReadFileByChar(path);
    string rss = "";
    for (auto item : rssString) {
        if (isdigit(item)) {
            rss += item;
        }
    }
    if (rss.empty()) {
        HIVIEW_LOGD("failed to get process rss, reason: not digital, pid: %{public}d", pid);
        return 0;
    }
    uint64_t rssInt;
    if (!ConvertToULL(rss, rssInt)) {
        HIVIEW_LOGD("failed to get process rss, reason: not digital, pid: %{public}d", pid);
        return 0;
    }
    return rssInt;
}

int32_t DfrUtil::GetRssThreshold(pid_t pid)
{
    string path = "/proc/" + to_string(pid) + "/rss_threshold";
    string thresholdStr = ReadFileByChar(path);
    if (thresholdStr.empty()) {
        HIVIEW_LOGD("failed to get process rss_threshold, pid: %{public}d", pid);
        return -1;
    }
    string digits;
    for (char c : thresholdStr) {
        if (isdigit(c)) {
            digits.push_back(c);
        }
    }
    if (digits.empty()) {
        HIVIEW_LOGE("failed to get process rss_threshold, reason: no digits, pid: %{public}d", pid);
        return -1;
    }
    int32_t thresholdInt;
    if (!ConvertToInt(digits, thresholdInt)) {
        HIVIEW_LOGE("failed to convert rss_threshold, pid: %{public}d", pid);
        return -1;
    }
    return thresholdInt;
}

int DfrUtil::GetParentPid(int pid)
{
    string path = "/proc/" + to_string(pid) + "/stat";
    vector<string> statInfo = GetStatInfo(path);
    if (statInfo.size() < MAX_INDEX) {
        return 0;
    }
    string ppidStr = statInfo[PPID_INDEX];
    if (!isdigit(ppidStr[0])) {
        HIVIEW_LOGE("failed to get process parent pid, reason: not digital, pid: %d", pid);
        return 0;
    }
    int ppid;
    if (!SafeStoi(ppidStr, ppid)) {
        HIVIEW_LOGE("failed to stoi, str: %{public}s", ppidStr.c_str());
        return -1;
    }
    return ppid;
}

bool DfrUtil::IsPathSafe(const string &path)
{
    if (path.empty() || path.find('/') != string::npos ||
        path.find("..") != string::npos) {
        HIVIEW_LOGE("path is invalid: %{public}s", path.c_str());
        return false;
    }
    for (size_t i = 0; i < path.size(); i++) {
        unsigned char c = static_cast<unsigned char>(path[i]);
        if (c < ASCII_PRINTABLE_MIN || c > ASCII_PRINTABLE_MAX) {
            HIVIEW_LOGE("path contains non-printable character: %{public}s", path.c_str());
            return false;
        }
    }
    return true;
}

time_t DfrUtil::GetThreadStartTime(int pid, int tid)
{
    string taskPath = "/proc/" + to_string(pid) + "/task/" + to_string(tid) + "/stat";
    vector<string> statInfo = GetStatInfo(taskPath);
    if (statInfo.size() < MAX_INDEX) {
        return -1;
    }
    string startTime = statInfo[START_TIME_INDEX];
    long long value;
    if (!SafeStoll(startTime, value)) {
        HIVIEW_LOGE("failed to get startTime, startTime: %{public}s, pid: %{public}d", startTime.c_str(), pid);
        return -1;
    }
    return static_cast<time_t>(value);
}

// consider the parent of process is zygote as 32 bit process for Java app.
bool DfrUtil::Is32BitProcess(int32_t pid)
{
    int ppid = GetParentPid(pid);
    return GetProcessName(ppid) == "zygote";
}

// consider the processce ppid == 2 as kernel thread.
bool DfrUtil::IsKernelProcess(int pid)
{
    int ppid = GetParentPid(pid);
    return ppid == PID_KTHREADD;
}

int DfrUtil::GetThreadNums(int pid)
{
    string taskPath = "/proc/" + to_string(pid) + "/status";
    ifstream fin;
    fin.open(taskPath.c_str());
    if (!fin.is_open()) {
        HIVIEW_LOGE("failed to open: %{public}s, err: %{public}s", taskPath.c_str(), strerror(errno));
        return 0;
    }
    string line;
    while (getline(fin, line)) {
        if (line.find("Threads:") != line.npos) {
            size_t pos = line.find("\t");
            line = line.substr(pos + 1);
            int value;
            if (!ConvertToInt(line, value)) {
                fin.close();
                return 0;
            }
            fin.close();
            return value;
        }
    }
    fin.close();
    return 0;
}

time_t DfrUtil::GetRunningMonotonicTime()
{
    timespec now;
    now.tv_sec = 0;
    now.tv_nsec = 0;
    int32_t ret = clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    if (ret != 0) {
        HIVIEW_LOGE("FATILED TO GET MONOTONIC TIME");
    }
    return now.tv_sec;
}

void DfrUtil::RemoveAllFilesInDir(const string &path)
{
    vector<string> files = GetSubFile(path);
    for (const auto &each : files) {
        string filePath = path + "/" + each;
        RemoveFile(filePath);
    }
}

pid_t DfrUtil::GetProcessUid(pid_t pid)
{
    pid_t uid = -1;
    struct stat st;
    string path = "/proc/" + to_string(pid) + "/attr";
    if (stat(path.c_str(), &st) == -1) {
        HIVIEW_LOGE("stat %{public}s error: %{public}s", path.c_str(), strerror(errno));
        return uid;
    }
    uid = static_cast<pid_t>(st.st_uid);
    return uid;
}

string DfrUtil::GetApplicationNameByUid(pid_t uid)
{
    string bundleName;
    AppExecFwk::BundleMgrClient client;
    int ret = client.GetNameForUid(uid, bundleName);
    if (ret != ERR_OK) {
        HIVIEW_LOGD("Failed to query bundleName from bms, uid:%{public}d, ret: %{public}d.", uid, ret);
        return "";
    }
    HIVIEW_LOGD("bundleName of uid:%{public}d is %{public}s", uid, bundleName.c_str());
    return bundleName;
}

string DfrUtil::GetApplicationVersionByPid(pid_t pid)
{
    pid_t uid = GetProcessUid(pid);
    if (uid < MIN_APP_USERID) {
        return "";
    }
    return GetApplicationVersionByUid(uid);
}

string DfrUtil::GetApplicationVersionByUid(pid_t uid)
{
    const string bundleName = GetApplicationNameByUid(uid);
    if (bundleName.empty()) {
        return "";
    }
    AppExecFwk::BundleInfo info;
    AppExecFwk::BundleMgrClient client;
    if (!client.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, info,
                              AppExecFwk::Constants::ALL_USERID)) {
        HIVIEW_LOGE("Failed to query BundleInfo from bms, uid:%{public}d.", uid);
        return "";
    }
    HIVIEW_LOGI("The version of %{public}s is %{public}s", bundleName.c_str(), info.versionName.c_str());
    return info.versionName;
}

bool DfrUtil::IsBetaVersion()
{
    string userType = system::GetParameter(KEY_HIVIEW_USER_TYPE, "");
    return userType == "beta";
}

bool DfrUtil::IsDevelopMode()
{
    string modeType = system::GetParameter(KEY_HIVIEW_DEVELOP_MODE, "");
    return modeType == "enable";
}

bool DfrUtil::IsBetaOrDevelopMode()
{
    if (developOrBetaModeState_ == UNKNOWN) {
        if (IsBetaVersion() || IsDevelopMode()) {
            developOrBetaModeState_ = STATE_TRUE;
        } else {
            developOrBetaModeState_ = STATE_FALSE;
        }
    }

    return developOrBetaModeState_ == STATE_TRUE;
}

int64_t DfrUtil::GetSizeInProcNode(string nodeName, string typeName)
{
    string buffer = GetCacheProcInfo(nodeName);
    if (buffer.empty()) {
        HIVIEW_LOGE("failed to open: %{public}s", nodeName.c_str());
        return -1;
    }
    istringstream stream(buffer);
    string line;
    while (getline(stream, line)) {
        if (line.find(typeName) != line.npos) {
            auto begin = line.find_first_of(DIGIT);
            auto end = line.find_last_of(DIGIT);
            if (begin == string::npos || end == string::npos || (end - begin + 1) <= 0) {
                HIVIEW_LOGD("failed to get %{public}s mem in %{public}s", typeName.c_str(), nodeName.c_str());
                break;
            }
            line = line.substr(begin, end - begin + 1);
            uint64_t value = static_cast<uint64_t>(INT_MAX);
            if (!ConvertToULL(line, value)) {
                break;
            }
            return value;
        }
    }
    return -1;
}

bool DfrUtil::CallSmapsRollup(pid_t pid, int &pss, int &swappss, int &rss)
{
    string traceStr(__func__);
    traceStr.append(":").append(to_string(pid));
    HitraceScoped hitrace(HITRACE_TAG_APP, traceStr);
    string path = "/proc/" + to_string(pid) + "/smaps_rollup";
    ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string key;
        int value;
        string unit;
        if (iss >> key >> value >> unit) {
            if (key == "Pss:") {
                pss = value;
            } else if (key == "SwapPss:") {
                swappss = value;
            } else if (key == "Rss:") {
                rss = value;
            }
        }
    }

    file.close();
    return true;
}

int DfrUtil::GetRamSize()
{
    return totalRamSize_;
}

int DfrUtil::GetTotalRamSize()
{
    int memTotal = static_cast<int>(GetSizeInProcNode(MEM_INFO_PATH, "MemTotal"));
    if (memTotal < 0 || memTotal >= INT_MAX) {
        return 0;
    }
    return memTotal / KB_PER_MB / MB_PER_GB + 1;
}

int DfrUtil::ConvertToString(const unsigned char hash[LEAK_SHA256_LENGTH], char *outstr, size_t len)
{
    char *outHash = outstr;

    if (hash == nullptr || outHash == nullptr) {
        return EINVAL;
    }
    constexpr int charsEachHex = 2;
    if (len < (LEAK_SHA256_LENGTH * charsEachHex + 1)) {  // 1: add '\0'
        return ENOMEM;
    }
    for (uint32_t i = 0; i < LEAK_SHA256_LENGTH; i++) {
        int err = snprintf_s(outHash, charsEachHex + 1, charsEachHex, "%02x", hash[i]);
        if (err < 0) {
            return err;
        }
        outHash += charsEachHex;
    }
    *outHash = '\0';
    return 0;
}

int DfrUtil::CalcBufferSha(const string &buffer, size_t bufSize, char *hash, size_t len)
{
    if (buffer.empty() || hash == nullptr) {
        return EINVAL;
    }
    unsigned char value[LEAK_SHA256_LENGTH] = {0};
    SHA256(reinterpret_cast<const unsigned char *>(buffer.c_str()), bufSize, value);
    return ConvertToString(value, hash, len);
}

string DfrUtil::CalcFingerPrint(const string &val, size_t mask)
{
    char hash[HAS_LEN] = {'0'};

    int err = CalcBufferSha(val, val.size(), hash, sizeof(hash));
    if (err != 0) {
        // if file not exist, API will return ENOENT
        HIVIEW_LOGE("ProcessedEvent: calc fingerprint failed, err is %{public}d.", err);
        return "";
    }
    return string(hash);
}

void DfrUtil::GetStatm(int32_t pid, uint64_t &vss, uint64_t &rss)
{
    string path = "/proc/" + to_string(pid) + "/statm";
    const string &realPath = RealPath(path);
    if (realPath.empty()) {
        HIVIEW_LOGE("failed realpath path: %{public}s", path.c_str());
        return;
    }
    ifstream statmStream(realPath);
    if (!statmStream) {
        HIVIEW_LOGE("Fail to open %{public}s", realPath.c_str());
        return;
    }
    string statmLine;
    getline(statmStream, statmLine);
    statmStream.close();
    if (statmLine.empty()) {
        HIVIEW_LOGE("pid: %{public}d maybe dead", pid);
        return;
    }
    list<string> numStrArr = GetDightStrArr(statmLine);
    auto it = numStrArr.begin();
    unsigned long long multiples = 4;
    if (it == numStrArr.end()) {
        HIVIEW_LOGE("pid: %{public}d numStrArr.size is too short", pid);
        return;
    }
    uint64_t value = 0;
    if (!ConvertToULL(*it, value)) {
        HIVIEW_LOGE("failed to get vss, it = %{public}s, pid = %{public}d.", it->c_str(), pid);
        return;
    }
    vss = multiples * value;
    it++;
    if (it == numStrArr.end()) {
        HIVIEW_LOGE("pid: %{public}d numStrArr.size is too short", pid);
        return;
    }
    if (!ConvertToULL(*it, value)) {
        HIVIEW_LOGE("failed to get rss, it = %{public}s, pid = %{public}d.", it->c_str(), pid);
        return;
    }
    rss = multiples * value;
    HIVIEW_LOGI("GET rss=%{public}lu, vss=%{public}lu.", rss, vss);
}

void DfrUtil::GetMeminfo(uint64_t &avaliableMem, uint64_t &freeMem, uint64_t &totalMem)
{
    ifstream meminfoStream("/proc/meminfo");
    if (!meminfoStream) {
        HIVIEW_LOGE("Fail to open /proc/meminfo, err: %{public}s", strerror(errno));
        return;
    }
    string meminfoLine;
    getline(meminfoStream, meminfoLine);
    if (!ConvertToULL(GetDightStrArr(meminfoLine).front(), totalMem)) {
        HIVIEW_LOGE("failed to get totalMem.");
        return;
    }
    getline(meminfoStream, meminfoLine);
    if (!ConvertToULL(GetDightStrArr(meminfoLine).front(), freeMem)) {
        HIVIEW_LOGE("failed to get freeMem.");
        return;
    }
    getline(meminfoStream, meminfoLine);
    if (!ConvertToULL(GetDightStrArr(meminfoLine).front(), avaliableMem)) {
        HIVIEW_LOGE("failed to get avaliableMem.");
        return;
    }
    meminfoStream.close();
    HIVIEW_LOGI("GET freeMem=%{public}lu, avaliableMem=%{public}lu, totalMem=%{public}lu.", freeMem, avaliableMem,
                totalMem);
}

list<string> DfrUtil::GetDightStrArr(const string &target)
{
    list<string> ret;
    string temp = "";
    for (size_t i = 0, len = target.size(); i < len; i++) {
        if (target[i] >= '0' && target[i] <= '9') {
            temp += target[i];
            continue;
        }
        if (temp.size() != 0) {
            ret.push_back(temp);
            temp = "";
        }
    }
    if (temp.size() != 0) {
        ret.push_back(temp);
    }
    ret.push_back("0");
    return ret;
}

void DfrUtil::SetAppEventHandle(shared_ptr<AppEventHandler> handler)
{
    appEventHandler_ = handler;
}

string DfrUtil::GetAllProcessIonInfo()
{
    auto fd = open(SYSLOAD_PATH, O_RDONLY);
    if (fd < 0) {
        HIVIEW_LOGE("failed to open %{public}s, err: %{public}s", SYSLOAD_PATH, strerror(errno));
        return "";
    }
    fdsan_exchange_owner_tag(fd, 0, FDSAN_TAG);
    uint64_t infoSize = sizeof(IonInfo);
    auto info = static_cast<IonInfo *>(calloc(1, infoSize));
    if (info == nullptr) {
        HIVIEW_LOGE("FAILED TO ALLOC MEMORY!");
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return "";
    }
    info->magic = SYSLOAD_DETECTOR_MAGIC;
    info->menu = MENU_DMA;
    info->type = TYPE_ALL_PID_INFO;
    info->reqMemSize = infoSize;
    int32_t ret = ioctl(fd, GET_PROC_DMA_INFO, info);
    if (ret != 0) {
        HIVIEW_LOGE("IOCTL READ ALL PROCESS ION INFO FAILED, ret: %{public}d, err: %{public}s", ret, strerror(errno));
        free(info);
        fdsan_close_with_tag(fd, FDSAN_TAG);
        return "";
    }
    string tempStr = info->allProcIonInfo;
    HIVIEW_LOGI("READ ALL PROCESS ION INFO FINSISHED, info: %{public}s", tempStr.c_str());
    free(info);
    fdsan_close_with_tag(fd, FDSAN_TAG);
    return tempStr;
}

void DfrUtil::RemoveOldProcFilePaths()
{
    time_t now = time(nullptr);
    std::lock_guard<ffrt::mutex> lock(procDumpMtx_);
    auto it = procFilePathMap_.begin();
    while (it != procFilePathMap_.end()) {
        if (now - it->second.second > MAP_ELEMT_OLD_TIME) {
            it = procFilePathMap_.erase(it);
        } else {
            ++it;
        }
    }
}

string DfrUtil::GetCacheProcInfo(const string &procFilePath)
{
    time_t now = time(nullptr);
    procDumpMtx_.lock();
    if (!procFilePathMap_.count(procFilePath) || now - procFilePathMap_[procFilePath].second > MAP_ELEMT_OLD_TIME) {
        HIVIEW_LOGI("the procfilepath %{public}s should been changed or inited.", procFilePath.c_str());
        if (procFilePath.find(SYSLOAD_PATH) != string::npos) {
            string allIonInfo = GetAllProcessIonInfo();
            procFilePathMap_[procFilePath] = pair<string, time_t>(allIonInfo, now);
            procDumpMtx_.unlock();
            return allIonInfo;
        }
        ifstream fin(procFilePath);
        if (!fin.is_open()) {
            procDumpMtx_.unlock();
            HIVIEW_LOGE("failed to open file: %{public}s.", procFilePath.c_str());
            return "";
        }
        stringstream finBuffer;
        finBuffer << fin.rdbuf();
        procFilePathMap_[procFilePath] = pair<string, time_t>(finBuffer.str(), now);
        fin.close();
    }
    string tmp = procFilePathMap_[procFilePath].first;
    procDumpMtx_.unlock();
    return tmp;
}

map<int, int> DfrUtil::ExtractMemValue(const string &infoStr)
{
    map<int, int> pidMemMap;
    vector<string> pairList = Split(infoStr, ',');

    for (const auto &item : pairList) {
        // pid:valueMem
        size_t pos = item.find(':');
        if (pos == string::npos) {
            continue;
        }
        string pidStr = item.substr(0, pos);
        string memStr = item.substr(pos + 1);

        int pid, memValue;
        if (SafeStoi(pidStr, pid) && SafeStoi(memStr, memValue)) {
            pidMemMap[pid] = memValue;
        } else {
            HIVIEW_LOGE("Failed to parse pid or memValue, item: %{public}s", item.c_str());
        }
    }
    return pidMemMap;
}

map<int, int> DfrUtil::GetIonValueFromKernel()
{
    // path not exist, append type used for ioctl
    string ionInfoStr = GetCacheProcInfo(string(SYSLOAD_PATH) + "/" + ION_TYPE);
    return ExtractMemValue(ionInfoStr);
}

bool DfrUtil::IsPhone()
{
    static State isPhone = UNKNOWN;
    if (isPhone == UNKNOWN) {
        TryToGetDeviceType();
        isPhone = deviceType_ == "phone" ? STATE_TRUE : STATE_FALSE;
    }
    return isPhone == STATE_TRUE;
}

bool DfrUtil::IsTablet()
{
    static State isTablet = UNKNOWN;
    if (isTablet == UNKNOWN) {
        TryToGetDeviceType();
        isTablet = deviceType_ == "tablet" ? STATE_TRUE : STATE_FALSE;
    }
    return isTablet == STATE_TRUE;
}

bool DfrUtil::IsPC()
{
    static State isPC = UNKNOWN;
    if (isPC == UNKNOWN) {
        TryToGetDeviceType();
        isPC = deviceType_ == "pc" || deviceType_ == "2in1" ? STATE_TRUE : STATE_FALSE;
    }
    return isPC == STATE_TRUE;
}

bool DfrUtil::IsWatch()
{
    static State isWatch = UNKNOWN;
    if (isWatch == UNKNOWN) {
        TryToGetDeviceType();
        isWatch = deviceType_ == "watch" ? STATE_TRUE : STATE_FALSE;
    }
    return isWatch == STATE_TRUE;
}

void DfrUtil::TryToGetDeviceType()
{
    if (deviceType_ != "unknown") {
        return;
    }
    deviceType_ = system::GetParameter(KEY_DEVICE_TYPE, "unknown");
    HIVIEW_LOGI("deviceType_ is %{public}s", deviceType_.c_str());
}

bool DfrUtil::ConvertToInt(const string &str, int &value)
{
    auto [ptr, ec] = from_chars(str.data(), str.data() + str.size(), value);
    return ec == errc{} && ptr == str.data() + str.size();
}

bool DfrUtil::ConvertToULL(const string &str, uint64_t &value)
{
    auto [ptr, ec] = from_chars(str.data(), str.data() + str.size(), value);
    return ec == errc{} && ptr == str.data() + str.size();
}

bool DfrUtil::IsDeviceLowmem()
{
    int64_t sizeProcNode = GetSizeInProcNode(MEM_INFO_PATH, "MemAvailable");
    int currentMem = (sizeProcNode <= static_cast<int64_t>(INT_MAX)) ? static_cast<int>(sizeProcNode) : -1;
    if (currentMem == -1) {
        HIVIEW_LOGE("Cannot get current device available memory");
        return false;
    }
    int lowmemThreshold = IsWatch() ? LOWMEM_THRESHOLD_WATCH : LOWMEM_THRESHOLD;
    if (currentMem < lowmemThreshold) {
        HIVIEW_LOGI("current device is LOW MEM! memory: %{public}d.", currentMem);
        return true;
    }
    HIVIEW_LOGD("current device is not LOW MEM! memory: %{public}d.", currentMem);
    return false;
}

bool DfrUtil::AppendMetadata(const string &newPath)
{
    struct stat rawheapInfo;
    if (stat(newPath.c_str(), &rawheapInfo) != 0) {
        HIVIEW_LOGE("get snapShot file stat failed");
        return false;
    }
    uint32_t rawheapSize = static_cast<uint32_t>(rawheapInfo.st_size);
    if (rawheapSize == 0) {
        HIVIEW_LOGE("rawheap size is 0");
        return false;
    }

    struct stat jsonInfo;
    if (stat(METADATA_PATH, &jsonInfo) != 0) {
        HIVIEW_LOGE("get snapShot file stat failed");
        return false;
    }
    uint32_t jsonSize = static_cast<uint32_t>(jsonInfo.st_size);

    ifstream finMetadata(METADATA_PATH);
    if (!finMetadata.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s", METADATA_PATH);
        return false;
    }

    ofstream fout;
    const string &newRealPath = RealPath(newPath);
    if (newRealPath.empty()) {
        HIVIEW_LOGE("fail to realpath: %{public}s", newPath.c_str());
        finMetadata.close();
        return false;
    }
    fout.open(newRealPath.c_str(), ios::out | ios::app);
    if (!fout.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s", newRealPath.c_str());
        finMetadata.close();
        return false;
    }
    HIVIEW_LOGI("success: rawheap size: %{public}u, metadata size: %{public}u", rawheapSize, jsonSize);
    fout << finMetadata.rdbuf();
    fout.write(reinterpret_cast<const char *>(&rawheapSize), sizeof(rawheapSize));
    fout.write(reinterpret_cast<const char *>(&jsonSize), sizeof(jsonSize));
    fout.close();
    finMetadata.close();
    return true;
}

int32_t DfrUtil::GetForegroundOsAccountLocalId()
{
    int32_t localId;
    int getLocalIdRet = AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(localId);
    if (getLocalIdRet != ERR_OK) {
        HIVIEW_LOGE("failed to GetForegroundOsAccountLocalId. ret: %{public}d", getLocalIdRet);
        return 0;
    }
    HIVIEW_LOGI("localId: %{public}d", localId);

    return localId;
}

int DfrUtil::GetFreeRomMb()
{
    struct statvfs st;

    if (statvfs("/data/storage/", &st) != 0) {
        HIVIEW_LOGE("failed to statvfs, errno: %{public}d", errno);
        return -1;
    }

    unsigned long long freeSize = static_cast<unsigned long long>(st.f_bsize) * st.f_bfree;
    unsigned long long freeSizeMb = freeSize / KB_PER_MB / BYTE_PER_KB;

    if (freeSizeMb > INT_MAX) {
        HIVIEW_LOGE("freeSize it too big, freeSizeMb: %{public}llu", freeSizeMb);
        return -1;
    }
    HIVIEW_LOGI("freeSizeMb: %{public}lluM", freeSizeMb);
    return static_cast<int>(freeSizeMb);
}

bool DfrUtil::LongerThanAWeek(time_t t1, time_t t2)
{
    return difftime(t1, t2) > SECONDS_IN_A_WEEK;
}

bool DfrUtil::IsApp(pid_t pid)
{
    pid_t uid = GetProcessUid(pid);
    return uid >= MIN_APP_USERID;
}

string DfrUtil::GetParentPath(const string &path)
{
    size_t lastSlash = path.rfind('/');
    if (lastSlash == string::npos) {
        return path;
    }
    return path.substr(0, lastSlash);
}

uint64_t DfrUtil::ExtractDumpTraceTime(const string &eventId)
{
    size_t pos = eventId.rfind('_');
    if (pos == string::npos || pos + 1 >= eventId.size()) {
        return 0;
    }
    struct tm tmFormat;
    string timeStr = eventId.substr(pos + 1);
    strptime(timeStr.c_str(), "%Y%m%d%H%M%S", &tmFormat);
    tmFormat.tm_isdst = -1;
    time_t ts = mktime(&tmFormat);
    return static_cast<uint64_t>(ts);
}

pair<string, vector<string>> DfrUtil::FreezeDumpTrace(uint64_t faultTime, const string &bundleName, bool isTelemetry)
{
    HIVIEW_LOGI("dump trace, isTelemetry: %{public}d", isTelemetry);
    pair<string, vector<string>> result;
    shared_ptr<UCollectUtil::TraceCollector> collector = UCollectUtil::TraceCollector::Create();
    if (!collector) {
        HIVIEW_LOGE("create trace collector failed!");
        return result;
    }
    CollectResult<std::vector<std::string>> collectResult;
    if (isTelemetry) {
        collectResult =
            collector->DumpTraceWithFilter(UCollect::TeleModule::RELIABILITY, MAX_DUMP_TRACE_LIMIT, faultTime);
    } else {
        collectResult =
            collector->DumpTraceWithDuration(UCollect::TraceCaller::RELIABILITY, MAX_DUMP_TRACE_LIMIT, faultTime);
    }
    if (collectResult.retCode == UCollect::UcError::SUCCESS) {
        result.second = collectResult.data;
        HIVIEW_LOGI("dump trace success, bundle: %{public}s", bundleName.c_str());
    } else {
        HIVIEW_LOGE("dump trace failed, faultTime: %{public}" PRIu64 ", error code: %{public}d", faultTime,
                    collectResult.retCode);
    }
    return result;
}

bool DfrUtil::IsOversea()
{
    static bool isOversea = system::GetParameter(GLOBAL_REGION, "") != REGION_CN;
    return isOversea;
}

bool DfrUtil::ShouldDumpRs()
{
    static bool shouldDumpRs = (!IsOversea() || IsBetaVersion());
    return shouldDumpRs;
}

string DfrUtil::GetTimeStrByEventId(const string &eventId)
{
    size_t pos = eventId.rfind('_');
    if (pos == std::string::npos || pos + 1 >= eventId.size()) {
        return "";
    }
    return eventId.substr(pos + 1);
}

void DfrUtil::CollectHiperf(const string &fileName, const vector<pid_t> &pids)
{
    std::shared_ptr<PerfCollector> perfCollector = PerfCollector::Create(PerfCaller::XPOWER);
    perfCollector->SetOutputFilename(fileName);
    perfCollector->SetTimeStopSec(PREF_COLLECT_TIME);
    perfCollector->SetSelectPids(pids);
    perfCollector->SetReport(true);
    perfCollector->SetFrequency(1000);  // 1000 : 1kHz
    CollectResult<bool> ret = perfCollector->StartPerf(EVENT_LOG_PATH);
    if (ret.retCode == UCollect::UcError::SUCCESS) {
        HIVIEW_LOGI("Success to call perf result : %{public}d", ret.data);
    } else {
        HIVIEW_LOGI("Failed to call perf result : %{public}d", ret.data);
    }
}

string DfrUtil::MaskFilename(const string &filename)
{
    size_t dotPos = filename.find_last_of('.');
    string namePart = filename.substr(0, dotPos);
    string extPart = (dotPos != string::npos) ? filename.substr(dotPos) : "";

    stringstream masked;
    for (size_t i = 0; i < namePart.size(); ++i) {
        if (i % ANONY_MASK_COUNT == ANONY_MASK_OFFSET) {
            masked << '*';
        } else {
            masked << namePart[i];
        }
    }

    return masked.str() + extPart;
}

string DfrUtil::AnonymizePath(const string &path)
{
    const string prefix = "/data/storage/el";

    if (path.find(prefix) == string::npos) {
        return path;
    }

    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == string::npos || lastSlash + 1 >= path.size()) {
        return path;
    }

    string dirPart = path.substr(0, lastSlash + 1);
    string filePart = path.substr(lastSlash + 1);

    return dirPart + MaskFilename(filePart);
}

string DfrUtil::AnonymizeInfo(const string &detailFdInfo)
{
    istringstream iss(detailFdInfo);
    string line;
    stringstream ss;
    while (getline(iss, line)) {
        ss << AnonymizePath(line) << endl;
    }

    return ss.str();
}

int DfrUtil::GetProcessGpuMemory(pid_t pid)
{
    UpdateGpuMemoryList();
    gpuMemoryListMtx_.lock();
    if (gpuMemoryList_.count(pid) == 0) {
        gpuMemoryListMtx_.unlock();
        return 0;
    }
    int memory = gpuMemoryList_[pid] * KB_PER_PAGE;
    gpuMemoryListMtx_.unlock();
    return memory;
}

void DfrUtil::UpdateGpuMemoryList()
{
    static time_t lastTime = 0;
    std::lock_guard<ffrt::mutex> lock(gpuMemoryListMtx_);
    time_t now = time(nullptr);
    if (now - lastTime <= MAP_ELEMT_OLD_TIME) {
        HIVIEW_LOGD("current time is not over 5 sec");
        return;
    }
    lastTime = now;
    gpuMemoryList_.clear();
    ifstream fin(GPU_MEMORY_PATH);
    if (!fin.is_open()) {
        HIVIEW_LOGE("failed to open file: %{public}s.", GPU_MEMORY_PATH);
        return;
    }
    stringstream finBuffer;
    finBuffer << fin.rdbuf();
    string line;
    while (getline(finBuffer, line)) {
        if (line.find("kmd-ctx") == string::npos) {
            continue;
        }
        vector<string> elementInfo;
        GetElementInfo(line, elementInfo, ' ');
        if (elementInfo.size() != GPU_MEMERY_ELEMENT_LENGTH) {
            HIVIEW_LOGE("current line has little info: %{public}s", line.c_str());
            continue;
        }
        pid_t processPid;
        int gpuMemory;  // page
        if (!SafeStoi(elementInfo[GPU_MEMERY_PID_INDEX], processPid) ||
            !SafeStoi(elementInfo[GPU_MEMERY_MEM_INDEX], gpuMemory)) {
            HIVIEW_LOGE("cannot get processPid or process gpu mem from current line: %{public}s", line.c_str());
            continue;
        }
        if (gpuMemoryList_.count(processPid) == 0) {
            gpuMemoryList_[processPid] = gpuMemory;
        } else {
            gpuMemoryList_[processPid] += gpuMemory;
        }
    }
    fin.close();
}

bool DfrUtil::GetElementInfo(const std::string &line, std::vector<std::string> &elements, char interval)
{
    std::string elementStr;
    std::stringstream strStream(line);
    std::vector<std::string> elementsTmp;
    while (std::getline(strStream, elementStr, interval)) {
        if (!elementStr.empty()) {
            elementsTmp.push_back(elementStr);
        }
    }
    if (elementsTmp.size() == 0) {
        return false;
    }
    elements = elementsTmp;
    return true;
}

bool DfrUtil::ShouldDetectAfterInterval(time_t lastDetectorTime, int intervalTime)
{
    time_t now = time(nullptr);
    if (now < lastDetectorTime) {
        return false;
    }
    if (lastDetectorTime != 0 && now - lastDetectorTime < static_cast<time_t>(intervalTime)) {
        return false;
    }
    return true;
}

void DfrUtil::UnfreezingProcess(pid_t pid)
{
    int uid = GetProcessUid(pid);
    string bundleName = GetApplicationNameByUid(uid);
    SuspendManager::SuspendManagerClient::GetInstance().ThawOneApplication(uid, bundleName, "ACTIVE_BY_DUMP_STACK");
    ffrt::this_task::sleep_for(chrono::microseconds(WAIT_TO_UNFREEZE_PROCESS * US_PER_SECOND));
}

bool DfrUtil::IsProcessRelatedLine(const std::string &line, size_t pos, const std::string &processName, char interval)
{
    std::vector<std::string> elementStrList;
    if (!GetElementInfo(line, elementStrList, interval)) {
        return false;
    }
    if (pos >= elementStrList.size()) {
        return false;
    }
    if (processName.find(elementStrList[pos]) == std::string::npos) {
        return false;
    }
    return true;
}

int DfrUtil::GetSlabNormal()
{
    std::ifstream file(PROC_SYSVIEW);
    if (!file.is_open()) {
        HIVIEW_LOGE("failed to open %{public}s", PROC_SYSVIEW);
        return 0;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("slab_normal") != std::string::npos) {
            std::istringstream iss(line);
            std::string key;
            int value;

            if (iss >> key >> value) {
                file.close();
                return value;
            }
        }
    }

    file.close();
    return 0;
}

string DfrUtil::RealPath(const string &path)
{
    struct stat st;
    char rtn[PATH_MAX] = {0};
    if (stat(path.c_str(), &st) == 0) {
        if (realpath(path.c_str(), rtn) == nullptr) {
            HIVIEW_LOGE("failed realpath: %{public}s, err: %{public}s", path.c_str(), strerror(errno));
            return "";
        }
        return rtn;
    }

    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == string::npos) {
        HIVIEW_LOGE("failed realpath: %{public}s, no parent dir", path.c_str());
        return "";
    }
    string parentDir = path.substr(0, lastSlash);
    if (stat(parentDir.c_str(), &st) != 0) {
        HIVIEW_LOGE("failed realpath: %{public}s, parent dir not exist", path.c_str());
        return "";
    }
    if (realpath(parentDir.c_str(), rtn) == nullptr) {
        HIVIEW_LOGE("failed realpath: %{public}s, because parent dir: %{public}s realpath failed", path.c_str(),
                    parentDir.c_str());
        return "";
    }

    return rtn + path.substr(lastSlash);
}

bool DfrUtil::IsProcessRelatedLine(const std::string &line, size_t pos, int pid, char interval)
{
    std::vector<std::string> elementStrList;
    if (!GetElementInfo(line, elementStrList, interval)) {
        return false;
    }
    if (pos >= elementStrList.size()) {
        return false;
    }
    int processPid = 0;
    if (!SafeStoi(elementStrList[pos], processPid)) {
        return false;
    }
    if (processPid != pid) {
        return false;
    }
    return true;
}

bool DfrUtil::IsValidBundleName(const string &bundleName)
{
    if (bundleName.empty() || bundleName.find("..") != string::npos ||
        bundleName.find('/') != string::npos || bundleName.find('\\') != string::npos) {
        HIVIEW_LOGE("invalid bundleName: %{public}s", bundleName.c_str());
        return false;
    }
    return true;
}

string DfrUtil::GetSandBoxCachePath(const string &bundleName)
{
    if (!IsValidBundleName(bundleName)) {
        return "";
    }
    int32_t accountId = GetForegroundOsAccountLocalId();
    if (accountId == 0) {
        HIVIEW_LOGE("failed to GetForegroundOsAccountLocalId");
        return "";
    }

    return "/data/app/el2/" + to_string(accountId) + "/base/" + bundleName + "/cache";
}

string DfrUtil::GetBundleQuotaPath(const string &bundleName)
{
    string cachePath = GetSandBoxCachePath(bundleName);
    if (cachePath.empty()) {
        return "";
    }

    return cachePath + "/rawheap";
}

}  // namespace HiviewDFX
}  // namespace OHOS
