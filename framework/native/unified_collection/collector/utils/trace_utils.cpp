/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include <algorithm>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "cpu_collector.h"
#include "file_util.h"
#include "ffrt.h"
#include "hiview_logger.h"
#include "hiview_event_report.h"
#include "parameter_ex.h"
#include "securec.h"
#include "string_util.h"
#include "trace_utils.h"
#include "trace_worker.h"

using namespace std::chrono_literals;
using OHOS::HiviewDFX::TraceWorker;

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
const std::string UNIFIED_SHARE_PATH = "/data/log/hiview/unified_collection/trace/share/";
const std::string UNIFIED_SPECIAL_PATH = "/data/log/hiview/unified_collection/trace/special/";
const std::string UNIFIED_SHARE_TEMP_PATH = UNIFIED_SHARE_PATH + "temp/";
const std::string RELIABILITY = "Reliability";
const std::string XPERF = "Xperf";
const std::string XPOWER = "Xpower";
const std::string BETACLUB = "BetaClub";
const std::string APP = "APP";
const std::string HIVIEW = "Hiview";
const std::string SCREEN = "Screen";
const std::string OTHER = "Other";
const uint32_t UNIFIED_SHARE_COUNTS = 25;
const uint32_t UNIFIED_APP_SHARE_COUNTS = 40;
const uint32_t UNIFIED_SPECIAL_XPERF = 3;
const uint32_t UNIFIED_SPECIAL_RELIABILITY = 3;
const uint32_t UNIFIED_SPECIAL_OTHER = 5;
const uint32_t UNIFIED_SPECIAL_BETACLUB = 2;
const uint32_t UNIFIED_SPECIAL_SCREEN = 1;
constexpr uint32_t READ_MORE_LENGTH = 100 * 1024;
const double CPU_LOAD_THRESHOLD = 0.03;
const uint32_t MAX_TRY_COUNT = 6;
}

enum {
    SHARE = 0,
    SPECIAL = 1,
};

UcError TransCodeToUcError(TraceErrorCode ret)
{
    if (CODE_MAP.find(ret) == CODE_MAP.end()) {
        HIVIEW_LOGE("ErrorCode is not exists.");
        return UcError::UNSUPPORT;
    } else {
        return CODE_MAP.at(ret);
    }
}

class CleanPolicy {
public:
    explicit CleanPolicy(int type) : type_(type) {}
    virtual ~CleanPolicy() {}
    void DoClean();

protected:
    virtual bool IsMine(const std::string &fileName) = 0;
    virtual uint32_t MyThreshold() = 0;

private:
    void LoadAllFiles(std::vector<std::string> &files);
    int type_;
};

void CleanPolicy::LoadAllFiles(std::vector<std::string> &files)
{
    // set path
    std::string path;
    if (type_ == SHARE) {
        path = UNIFIED_SHARE_PATH;
    } else {
        path = UNIFIED_SPECIAL_PATH;
    }
    // Load all files under the path
    FileUtil::GetDirFiles(path, files);
}

void CleanPolicy::DoClean()
{
    // Load all files under the path
    std::vector<std::string> files;
    LoadAllFiles(files);

    // Filter files that belong to me
    std::map<uint64_t, std::vector<std::string>> myFiles;
    for (const auto &file : files) {
        if (IsMine(file)) {
            struct stat fileInfo;
            stat(file.c_str(), &fileInfo);
            std::vector<std::string> fileLists;
            if (myFiles.find(fileInfo.st_mtime) != myFiles.end()) {
                fileLists = myFiles[fileInfo.st_mtime];
                fileLists.push_back(file);
                myFiles[fileInfo.st_mtime] = fileLists;
            } else {
                fileLists.push_back(file);
                myFiles.insert(std::pair<uint64_t, std::vector<std::string>>(fileInfo.st_mtime, fileLists));
            }
        }
    }

    HIVIEW_LOGI("myFiles size : %{public}zu, MyThreshold : %{public}u.", myFiles.size(), MyThreshold());

    // Clean up old files
    while (myFiles.size() > MyThreshold()) {
        for (const auto &file : myFiles.begin()->second) {
            FileUtil::RemoveFile(file);
            HIVIEW_LOGI("remove file : %{public}s is deleted.", file.c_str());
        }
        myFiles.erase(myFiles.begin());
    }
}

class ShareCleanPolicy : public CleanPolicy {
public:
    explicit ShareCleanPolicy(int type) : CleanPolicy(type) {}
    ~ShareCleanPolicy() override {}

protected:
    bool IsMine(const std::string &fileName) override
    {
        return true;
    }

    uint32_t MyThreshold() override
    {
        return UNIFIED_SHARE_COUNTS;
    }
};

class AppShareCleanPolicy : public CleanPolicy {
public:
    explicit AppShareCleanPolicy(int type) : CleanPolicy(type) {}
    ~AppShareCleanPolicy() override {}

protected:
    bool IsMine(const std::string &fileName) override
    {
        if (fileName.find("/" + APP) != std::string::npos) {
            return true;
        }
        return false;
    }

    uint32_t MyThreshold() override
    {
        return UNIFIED_APP_SHARE_COUNTS;
    }
};

class AppSpecialCleanPolicy : public CleanPolicy {
public:
    explicit AppSpecialCleanPolicy(int type) : CleanPolicy(type) {}
    ~AppSpecialCleanPolicy() override {}

protected:
    bool IsMine(const std::string &fileName) override
    {
        if (fileName.find("/" + APP) != std::string::npos) {
            return true;
        }
        return false;
    }

    uint32_t MyThreshold() override
    {
        return 0;
    }
};

class SpecialXperfCleanPolicy : public CleanPolicy {
public:
    explicit SpecialXperfCleanPolicy(int type) : CleanPolicy(type) {}
    ~SpecialXperfCleanPolicy() override {}

protected:
    bool IsMine(const std::string &fileName) override
    {
        // check xperf trace
        size_t posXperf = fileName.find(XPERF);
        return posXperf != std::string::npos;
    }

    uint32_t MyThreshold() override
    {
        return UNIFIED_SPECIAL_XPERF;
    }
};

class SpecialReliabilityCleanPolicy : public CleanPolicy {
public:
    explicit SpecialReliabilityCleanPolicy(int type) : CleanPolicy(type) {}
    ~SpecialReliabilityCleanPolicy() override {}

protected:
    bool IsMine(const std::string &fileName) override
    {
        // check Reliability trace
        size_t posReliability = fileName.find(RELIABILITY);
        return posReliability != std::string::npos;
    }

    uint32_t MyThreshold() override
    {
        return UNIFIED_SPECIAL_RELIABILITY;
    }
};

class SpecialOtherCleanPolicy : public CleanPolicy {
public:
    explicit SpecialOtherCleanPolicy(int type) : CleanPolicy(type) {}
    ~SpecialOtherCleanPolicy() override {}

protected:
    bool IsMine(const std::string &fileName) override
    {
        // check Other trace
        return fileName.find(OTHER) != std::string::npos;
    }

    uint32_t MyThreshold() override
    {
        return UNIFIED_SPECIAL_OTHER;
    }
};

class SpecialBetaClubCleanPolicy : public CleanPolicy {
    public:
        explicit SpecialBetaClubCleanPolicy(int type) : CleanPolicy(type) {}
        ~SpecialBetaClubCleanPolicy() override {}

    protected:
        bool IsMine(const std::string &fileName) override
        {
            // check BetaClub trace
            return fileName.find(BETACLUB) != std::string::npos;
        }

        uint32_t MyThreshold() override
        {
            return UNIFIED_SPECIAL_BETACLUB;
        }
    };

class SpecialScreenCleanPolicy : public CleanPolicy {
    public:
        explicit SpecialScreenCleanPolicy(int type) : CleanPolicy(type) {}
        ~SpecialScreenCleanPolicy() override {}

    protected:
        bool IsMine(const std::string &fileName) override
        {
            // check Screen trace
            return fileName.find(SCREEN) != std::string::npos;
        }

        uint32_t MyThreshold() override
        {
            return UNIFIED_SPECIAL_SCREEN;
        }
    };

std::shared_ptr<CleanPolicy> GetCleanPolicy(int type, UCollect::TraceCaller &caller)
{
    if (type == SHARE) {
        if (caller == UCollect::TraceCaller::APP) {
            return std::make_shared<AppShareCleanPolicy>(type);
        }

        return std::make_shared<ShareCleanPolicy>(type);
    }

    if (caller == UCollect::TraceCaller::XPERF) {
        return std::make_shared<SpecialXperfCleanPolicy>(type);
    }

    if (caller == UCollect::TraceCaller::RELIABILITY) {
        return std::make_shared<SpecialReliabilityCleanPolicy>(type);
    }

    if (caller == UCollect::TraceCaller::APP) {
        return std::make_shared<AppSpecialCleanPolicy>(type);
    }

    if (caller == UCollect::TraceCaller::BETACLUB) {
        return std::make_shared<SpecialBetaClubCleanPolicy>(type);
    }

    if (caller == UCollect::TraceCaller::SCREEN) {
        return std::make_shared<SpecialScreenCleanPolicy>(type);
    }
    return std::make_shared<SpecialOtherCleanPolicy>(type);
}

void FileRemove(UCollect::TraceCaller &caller)
{
    std::shared_ptr<CleanPolicy> shareCleaner = GetCleanPolicy(SHARE, caller);
    std::shared_ptr<CleanPolicy> specialCleaner = GetCleanPolicy(SPECIAL, caller);
    switch (caller) {
        case UCollect::TraceCaller::XPOWER:
        case UCollect::TraceCaller::HIVIEW:
            shareCleaner->DoClean();
            break;
        case UCollect::TraceCaller::RELIABILITY:
        case UCollect::TraceCaller::XPERF:
            shareCleaner->DoClean();
            specialCleaner->DoClean();
            break;
        case UCollect::TraceCaller::APP:
            shareCleaner->DoClean();
            specialCleaner->DoClean();
            break;
        default:
            specialCleaner->DoClean();
            break;
    }
}

void CheckAndCreateDirectory(const std::string &tmpDirPath)
{
    if (!FileUtil::FileExists(tmpDirPath)) {
        if (FileUtil::ForceCreateDirectory(tmpDirPath, FileUtil::FILE_PERM_775)) {
            HIVIEW_LOGD("create listener log directory %{public}s succeed.", tmpDirPath.c_str());
        } else {
            HIVIEW_LOGE("create listener log directory %{public}s failed.", tmpDirPath.c_str());
        }
    }
}

bool CreateMultiDirectory(const std::string &dirPath)
{
    uint32_t dirPathLen = dirPath.length();
    if (dirPathLen > PATH_MAX) {
        return false;
    }
    char tmpDirPath[PATH_MAX] = { 0 };
    for (uint32_t i = 0; i < dirPathLen; ++i) {
        tmpDirPath[i] = dirPath[i];
        if (tmpDirPath[i] == '/') {
            CheckAndCreateDirectory(tmpDirPath);
        }
    }
    return true;
}

const std::string EnumToString(UCollect::TraceCaller &caller)
{
    switch (caller) {
        case UCollect::TraceCaller::RELIABILITY:
            return RELIABILITY;
        case UCollect::TraceCaller::XPERF:
            return XPERF;
        case UCollect::TraceCaller::XPOWER:
            return XPOWER;
        case UCollect::TraceCaller::BETACLUB:
            return BETACLUB;
        case UCollect::TraceCaller::APP:
            return APP;
        case UCollect::TraceCaller::HIVIEW:
            return HIVIEW;
        case UCollect::TraceCaller::SCREEN:
            return SCREEN;
        default:
            return OTHER;
    }
}

std::vector<std::string> GetUnifiedFiles(TraceRetInfo ret, UCollect::TraceCaller &caller)
{
    if (caller == UCollect::TraceCaller::OTHER || caller == UCollect::TraceCaller::BETACLUB ||
        caller == UCollect::TraceCaller::SCREEN) {
        return GetUnifiedSpecialFiles(ret, caller);
    }
    if (caller == UCollect::TraceCaller::XPOWER || caller == UCollect::TraceCaller::HIVIEW) {
        return GetUnifiedShareFiles(ret, caller);
    }
    GetUnifiedSpecialFiles(ret, caller);
    return GetUnifiedShareFiles(ret, caller);
}

void CheckCurrentCpuLoad()
{
    std::shared_ptr<UCollectUtil::CpuCollector> collector = UCollectUtil::CpuCollector::Create();
    int32_t pid = getpid();
    auto collectResult = collector->CollectProcessCpuStatInfo(pid);
    HIVIEW_LOGI("first get cpu load %{public}f", collectResult.data.cpuLoad);
    uint32_t retryTime = 0;
    while (collectResult.data.cpuLoad > CPU_LOAD_THRESHOLD && retryTime < MAX_TRY_COUNT) {
        ffrt::this_task::sleep_for(5s);
        collectResult = collector->CollectProcessCpuStatInfo(pid);
        HIVIEW_LOGI("retry get cpu load %{public}f", collectResult.data.cpuLoad);
        retryTime++;
    }
}

std::string AddVersionInfoToZipName(const std::string &srcZipPath)
{
    std::string displayVersion = Parameter::GetDisplayVersionStr();
    std::string versionStr = StringUtil::ReplaceStr(StringUtil::ReplaceStr(displayVersion, "_", "-"), " ", "_");
    return StringUtil::ReplaceStr(srcZipPath, ".zip", "@" + versionStr + ".zip");
}

void ZipTraceFile(const std::string &srcSysPath, const std::string &destZipPath)
{
    HIVIEW_LOGI("start ZipTraceFile src: %{public}s, dst: %{public}s", srcSysPath.c_str(), destZipPath.c_str());
    FILE *srcFp = fopen(srcSysPath.c_str(), "rb");
    if (srcFp == nullptr) {
        return;
    }
    zip_fileinfo zipInfo;
    errno_t result = memset_s(&zipInfo, sizeof(zipInfo), 0, sizeof(zipInfo));
    if (result != EOK) {
        (void)fclose(srcFp);
        return;
    }
    std::string zipFileName = FileUtil::ExtractFileName(destZipPath);
    zipFile zipFile = zipOpen((UNIFIED_SHARE_TEMP_PATH + zipFileName).c_str(), APPEND_STATUS_CREATE);
    if (zipFile == nullptr) {
        HIVIEW_LOGE("zipOpen failed");
        (void)fclose(srcFp);
        return;
    }
    CheckCurrentCpuLoad();
    HiviewEventReport::ReportCpuScene("5");
    std::string sysFileName = FileUtil::ExtractFileName(srcSysPath);
    zipOpenNewFileInZip(
        zipFile, sysFileName.c_str(), &zipInfo, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    int errcode = 0;
    char buf[READ_MORE_LENGTH] = {0};
    while (!feof(srcFp)) {
        size_t numBytes = fread(buf, 1, sizeof(buf), srcFp);
        if (numBytes <= 0) {
            HIVIEW_LOGE("zip file failed, size is zero");
            errcode = -1;
            break;
        }
        zipWriteInFileInZip(zipFile, buf, static_cast<unsigned int>(numBytes));
        if (ferror(srcFp)) {
            HIVIEW_LOGE("zip file failed: %{public}s, errno: %{public}d.", srcSysPath.c_str(), errno);
            errcode = -1;
            break;
        }
    }
    (void)fclose(srcFp);
    zipCloseFileInZip(zipFile);
    zipClose(zipFile, nullptr);
    if (errcode != 0) {
        return;
    }
    std::string destZipPathWithVersion = AddVersionInfoToZipName(destZipPath);
    FileUtil::RenameFile(UNIFIED_SHARE_TEMP_PATH + zipFileName, destZipPathWithVersion);
    HIVIEW_LOGI("finish rename file %{public}s", destZipPathWithVersion.c_str());
}

void CopyFile(const std::string &src, const std::string &dst)
{
    int ret = FileUtil::CopyFile(src, dst);
    if (ret != 0) {
        HIVIEW_LOGE("copy file failed, file is %{public}s.", src.c_str());
    }
}

/*
 * apply to xperf, xpower and reliability
 * trace path eg.:
 *     /data/log/hiview/unified_collection/trace/share/
 *     trace_20230906111617@8290-81765922_{device}_{version}.zip
*/
std::vector<std::string> GetUnifiedShareFiles(TraceRetInfo ret, UCollect::TraceCaller &caller)
{
    if (ret.errorCode != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("DumpTrace: failed to dump trace, error code (%{public}d).", ret.errorCode);
        return {};
    }

    if (!FileUtil::FileExists(UNIFIED_SHARE_TEMP_PATH)) {
        if (!CreateMultiDirectory(UNIFIED_SHARE_TEMP_PATH)) {
            HIVIEW_LOGE("failed to create multidirectory.");
            return {};
        }
    }

    std::vector<std::string> files;
    for (const auto &tracePath : ret.outputFiles) {
        std::string traceFile = FileUtil::ExtractFileName(tracePath);
        const std::string destZipPath = UNIFIED_SHARE_PATH + StringUtil::ReplaceStr(traceFile, ".sys", ".zip");
        const std::string tempDestZipPath = UNIFIED_SHARE_TEMP_PATH + FileUtil::ExtractFileName(destZipPath);
        const std::string destZipPathWithVersion = AddVersionInfoToZipName(destZipPath);
        // for zip if the file has not been compressed
        if (!FileUtil::FileExists(destZipPathWithVersion) && !FileUtil::FileExists(tempDestZipPath)) {
            // new empty file is used to restore tasks in queue
            FileUtil::SaveStringToFile(tempDestZipPath, " ", true);
            UcollectionTask traceTask = [=]() {
                ZipTraceFile(tracePath, destZipPath);
            };
            TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
        }
        files.push_back(destZipPathWithVersion);
        HIVIEW_LOGI("trace file : %{public}s.", destZipPathWithVersion.c_str());
    }

    // file delete
    FileRemove(caller);

    return files;
}

/*
 * apply to BetaClub and Other Scenes
 * trace path eg.:
 * /data/log/hiview/unified_collection/trace/special/BetaClub_trace_20230906111633@8306-299900816.sys
*/
std::vector<std::string> GetUnifiedSpecialFiles(TraceRetInfo ret, UCollect::TraceCaller &caller)
{
    if (ret.errorCode != TraceErrorCode::SUCCESS) {
        HIVIEW_LOGE("Failed to dump trace, error code (%{public}d).", ret.errorCode);
        return {};
    }

    if (!FileUtil::FileExists(UNIFIED_SPECIAL_PATH)) {
        if (!CreateMultiDirectory(UNIFIED_SPECIAL_PATH)) {
            HIVIEW_LOGE("Failed to dump trace, error code (%{public}d).", ret.errorCode);
            return {};
        }
    }

    std::vector<std::string> files;
    for (const auto &trace : ret.outputFiles) {
        std::string traceFile = FileUtil::ExtractFileName(trace);
        const std::string dst = UNIFIED_SPECIAL_PATH + EnumToString(caller) + "_" + traceFile;
        // for copy if the file has not been copied
        if (!FileUtil::FileExists(dst)) {
            UcollectionTask traceTask = [=]() {
                CopyFile(trace, dst);
            };
            TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
        }
        files.push_back(dst);
        HIVIEW_LOGI("trace file : %{public}s.", dst.c_str());
    }

    // file delete
    FileRemove(caller);
    return files;
}
} // HiViewDFX
} // OHOS
