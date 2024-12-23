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
#include <fcntl.h>
#include <map>
#include <mutex>
#include <sys/file.h>
#include <unistd.h>

#include "trace_manager.h"

#include "file_util.h"
#include "hilog/log.h"
#include "hitrace_dump.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "parameter_ex.h"
#include "trace_collector.h"
#include "trace_utils.h"
#include "trace_worker.h"

using OHOS::HiviewDFX::Hitrace::TraceErrorCode;
using OHOS::HiviewDFX::UCollect::UcError;
using OHOS::HiviewDFX::Hitrace::TraceMode;

namespace OHOS {
namespace HiviewDFX {
namespace {
DEFINE_LOG_TAG("UCollectUtil-TraceCollector");
std::mutex g_traceLock;
TraceMode g_recoverMode = Parameter::IsBetaVersion() ? TraceMode::SERVICE_MODE : TraceMode::CLOSE;
const std::string UNIFIED_SHARE_PATH = "/data/log/hiview/unified_collection/trace/share/";
const std::string UNIFIED_SHARE_TEMP_PATH = UNIFIED_SHARE_PATH + "temp/";
}

int32_t TraceManager::OpenSnapshotTrace(const std::vector<std::string> &tagGroups)
{
    std::lock_guard<std::mutex> lock(g_traceLock);
    HIVIEW_LOGI("start to open snapshot trace.");
    // service mode
    if (OHOS::HiviewDFX::Hitrace::GetTraceMode() ==
        OHOS::HiviewDFX::Hitrace::TraceMode::SERVICE_MODE) {
        HIVIEW_LOGE("now is snapshot status, open snapshot failed.");
        return UcError::TRACE_WRONG_MODE;
    }

    // recording mode
    if (OHOS::HiviewDFX::Hitrace::GetTraceMode() ==
        OHOS::HiviewDFX::Hitrace::TraceMode::CMD_MODE) {
        HIVIEW_LOGE("now is recording status, open snapshot failed.");
        return UcError::TRACE_IS_OCCUPIED;
    }

    TraceErrorCode ret = OHOS::HiviewDFX::Hitrace::OpenTrace(tagGroups);
    if (ret == TraceErrorCode::SUCCESS) {
        g_recoverMode = TraceMode::SERVICE_MODE;
    }
    return TransCodeToUcError(ret);
}

int32_t TraceManager::OpenRecordingTrace(const std::string &args)
{
    std::lock_guard<std::mutex> lock(g_traceLock);
    HIVIEW_LOGI("start to open recording trace.");
    // recording mode
    if (OHOS::HiviewDFX::Hitrace::GetTraceMode() ==
        OHOS::HiviewDFX::Hitrace::TraceMode::CMD_MODE) {
        HIVIEW_LOGE("now is recording status, open recording failed.");
        return UcError::TRACE_IS_OCCUPIED;
    }

    // service mode
    if (OHOS::HiviewDFX::Hitrace::GetTraceMode() ==
        OHOS::HiviewDFX::Hitrace::TraceMode::SERVICE_MODE) {
        HIVIEW_LOGI("TraceMode is switching: snapshot close, recording open.");
        OHOS::HiviewDFX::Hitrace::CloseTrace();
    }

    TraceErrorCode ret = OHOS::HiviewDFX::Hitrace::OpenTrace(args);
    return TransCodeToUcError(ret);
}

int32_t TraceManager::CloseTrace()
{
    std::lock_guard<std::mutex> lock(g_traceLock);
    HIVIEW_LOGI("start to close trace.");
    TraceErrorCode ret = OHOS::HiviewDFX::Hitrace::CloseTrace();
    g_recoverMode = TraceMode::CLOSE;
    return TransCodeToUcError(ret);
}

int32_t TraceManager::RecoverTrace()
{
    std::lock_guard<std::mutex> lock(g_traceLock);
    HIVIEW_LOGI("start to recover trace.");
    TraceErrorCode ret = OHOS::HiviewDFX::Hitrace::CloseTrace();

    if (g_recoverMode == TraceMode::SERVICE_MODE) {
        HIVIEW_LOGI("recover trace to Snapshot.");
        const std::vector<std::string> tagGroups = {"scene_performance"};
        TraceErrorCode ret = OHOS::HiviewDFX::Hitrace::OpenTrace(tagGroups);
        return TransCodeToUcError(ret);
    }
    HIVIEW_LOGI("recover trace to close.");

    return TransCodeToUcError(ret);
}

int32_t TraceManager::GetTraceMode()
{
    return OHOS::HiviewDFX::Hitrace::GetTraceMode();
}

void TraceManager::RecoverTmpTrace()
{
    std::vector<std::string> traceFiles;
    FileUtil::GetDirFiles(UNIFIED_SHARE_TEMP_PATH, traceFiles, false);
    HIVIEW_LOGI("traceFiles need recover: %{public}zu", traceFiles.size());
    for (auto &filePath : traceFiles) {
        std::string fileName = FileUtil::ExtractFileName(filePath);
        HIVIEW_LOGI("unfinished trace file: %{public}s", fileName.c_str());
        std::string originTraceFile = StringUtil::ReplaceStr("/data/log/hitrace/" + fileName, ".zip", ".sys");
        if (!FileUtil::FileExists(originTraceFile)) {
            HIVIEW_LOGI("source file not exist: %{public}s", originTraceFile.c_str());
            FileUtil::RemoveFile(UNIFIED_SHARE_TEMP_PATH + fileName);
            continue;
        }
        int fd = open(originTraceFile.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd == -1) {
            HIVIEW_LOGI("open source file failed: %{public}s", originTraceFile.c_str());
            continue;
        }
        // add lock before zip trace file, in case hitrace delete origin trace file.
        if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
            HIVIEW_LOGI("get source file lock failed: %{public}s", originTraceFile.c_str());
            close(fd);
            continue;
        }
        HIVIEW_LOGI("originTraceFile path: %{public}s", originTraceFile.c_str());
        UcollectionTask traceTask = [=]() {
            ZipTraceFile(originTraceFile, UNIFIED_SHARE_PATH + fileName);
            flock(fd, LOCK_UN);
            close(fd);
        };
        TraceWorker::GetInstance().HandleUcollectionTask(traceTask);
    }
}
} // HiviewDFX
} // OHOS
