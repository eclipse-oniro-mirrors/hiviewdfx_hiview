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

#include "common_util.h"

#include <regex>

#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
namespace {
DEFINE_LOG_TAG("UCollectUtil-CommonUtil");
constexpr char EXPORT_FILE_REGEX[] = "[0-9]{1,}(.*)";
constexpr char UNDERLINE[] = "_";
constexpr size_t FORMAT_DATE_LEN = 14;

std::string GetTimeFromFileName(const std::string& fileName, const std::string& prefix, const std::string& pidStr)
{
    std::string fileTimeSub = StringUtil::GetRightSubstr(fileName, prefix);
    if (!pidStr.empty()) {
        // pid_yyyymmddHHMMSS.txt
        fileTimeSub = StringUtil::GetRightSubstr(fileTimeSub, UNDERLINE);
    }
    return fileTimeSub;
}
}

bool CommonUtil::ParseTypeAndValue(const std::string &str, std::string &type, int64_t &value)
{
    std::string::size_type typePos = str.find(":");
    if (typePos != std::string::npos) {
        type = str.substr(0, typePos);
        std::string valueStr = str.substr(typePos + 1);
        std::string::size_type valuePos = valueStr.find("kB");
        if (valuePos == std::string::npos) {
            valuePos = valueStr.find("KB");
        }
        if (valuePos != std::string::npos) {
            valueStr.resize(valuePos);
            StrToNum(valueStr, value);
            return true;
        } else {
            StrToNum(valueStr, value);
            return true;
        }
    }
    return false;
}

void CommonUtil::GetDirRegexFiles(const std::string& path, const std::string& prefix,
    std::vector<std::string>& files, const std::string& pidStr)
{
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGE("failed to open dir=%{public}s", path.c_str());
        return;
    }
    std::regex reg = std::regex(prefix + EXPORT_FILE_REGEX);
    struct dirent* ptr = nullptr;
    while ((ptr = readdir(dir)) != nullptr) {
        if (ptr->d_type == DT_REG) {
            if (regex_match(ptr->d_name, reg)) {
                files.push_back(FileUtil::IncludeTrailingPathDelimiter(path) + std::string(ptr->d_name));
            }
        }
    }
    closedir(dir);
    std::sort(files.begin(), files.end(), [&prefix, &pidStr](const std::string& file1, const std::string& file2) {
        std::string fileTimeSub1 = GetTimeFromFileName(file1, prefix, pidStr);
        std::string fileTimeSub2 = GetTimeFromFileName(file2, prefix, pidStr);
        if (fileTimeSub1.substr(0, FORMAT_DATE_LEN) == fileTimeSub2.substr(0, FORMAT_DATE_LEN) &&
            fileTimeSub1.size() != fileTimeSub2.size()) { // compare yyyymmddHHMMSS_1.txt and yyyymmddHHMMSS_10.txt
            return fileTimeSub1.size() < fileTimeSub2.size();
        }
        return fileTimeSub1 < fileTimeSub2;
    });
}

int CommonUtil::GetFileNameNum(const std::string& fileName, const std::string& ext)
{
    int ret = 0;
    auto startPos = fileName.find(UNDERLINE);
    if (startPos == std::string::npos) {
        return ret;
    }
    auto endPos = fileName.find(ext);
    if (endPos == std::string::npos) {
        return ret;
    }
    if (endPos <= startPos + 1) {
        return ret;
    }
    return StringUtil::StrToInt(fileName.substr(startPos + 1, endPos - startPos - 1));
}

std::string CommonUtil::CreateExportFile(const std::string& path, int32_t maxFileNum, const std::string& prefix,
    const std::string& ext, const std::string& pidStr)
{
    if (!FileUtil::IsDirectory(path) && !FileUtil::ForceCreateDirectory(path)) {
        HIVIEW_LOGE("failed to create dir=%{public}s", path.c_str());
        return "";
    }

    std::vector<std::string> files;
    GetDirRegexFiles(path, prefix, files, pidStr);
    if (files.size() >= static_cast<size_t>(maxFileNum)) {
        for (size_t index = 0; index <= files.size() - static_cast<size_t>(maxFileNum); ++index) {
            HIVIEW_LOGI("remove file=%{public}s", FileUtil::ExtractFileName(files[index]).c_str());
            (void)FileUtil::RemoveFile(files[index]);
        }
    }

    uint64_t fileTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    std::string timeFormat = TimeUtil::TimestampFormatToDate(fileTime, "%Y%m%d%H%M%S");
    // file name e.g. prefix_[pid_]yyyymmddHHMMSS.txt
    std::string fileName;
    fileName.append(FileUtil::IncludeTrailingPathDelimiter(path)).append(prefix).append(pidStr).append(timeFormat);
    if (!files.empty()) {
        auto startPos = files.back().find(timeFormat);
        if (startPos != std::string::npos) {
            int fileNameNum = GetFileNameNum(files.back().substr(startPos), ext); // yyyymmddHHMMSS_1.txt
            fileName.append(UNDERLINE).append(std::to_string(++fileNameNum));
        }
    }
    fileName.append(ext);
    (void)FileUtil::CreateFile(fileName);
    HIVIEW_LOGI("create file=%{public}s", FileUtil::ExtractFileName(fileName).c_str());
    return fileName;
}

int32_t CommonUtil::ReadNodeWithOnlyNumber(const std::string& fileName)
{
    std::string content;
    if (!FileUtil::LoadStringFromFile(fileName, content)) {
        HIVIEW_LOGW("read node failed");
        return 0;
    }
    int32_t parsedVal = 0;
    // this string content might be empty or consist of some special charactors
    // so "std::stoi" and "StringUtil::StrToInt" aren't applicable here.
    std::stringstream ss(content);
    ss >> parsedVal;
    return parsedVal;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
