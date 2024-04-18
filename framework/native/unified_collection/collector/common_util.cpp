/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
const std::string EXPORT_FILE_REGEX = "[0-9]{14}(.*)";
const std::string UNDERLINE = "_";
}
template <typename T> bool CommonUtil::StrToNum(const std::string &sString, T &tX)
{
    std::istringstream iStream(sString);
    return (iStream >> tX) ? true : false;
}

bool CommonUtil::ParseTypeAndValue(const std::string &str, std::string &type, int32_t &value)
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

bool CommonUtil::StartWith(const std::string& str, const std::string& sub)
{
    return str.find(sub) == 0;
}

bool CommonUtil::EndWith(const std::string& str, const std::string& sub)
{
    size_t index = str.rfind(sub);
    if (index == std::string::npos) {
        return false;
    }
    return index + sub.size() == str.size();
}

void CommonUtil::GetDirRegexFiles(const std::string& path, const std::string& pattern,
    std::vector<std::string>& files)
{
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        HIVIEW_LOGE("failed to open dir=%{public}s", path.c_str());
        return;
    }
    std::regex reg = std::regex(pattern);
    struct dirent* ptr = nullptr;
    while ((ptr = readdir(dir)) != nullptr) {
        if (ptr->d_type == DT_REG) {
            if (regex_match(ptr->d_name, reg)) {
                files.push_back(FileUtil::IncludeTrailingPathDelimiter(path) + std::string(ptr->d_name));
            }
        }
    }
    closedir(dir);
    std::sort(files.begin(), files.end());
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
    const std::string& ext)
{
    if (!FileUtil::IsDirectory(path) && !FileUtil::ForceCreateDirectory(path)) {
        HIVIEW_LOGE("failed to create dir=%{public}s", path.c_str());
        return "";
    }

    std::vector<std::string> files;
    GetDirRegexFiles(path, prefix + EXPORT_FILE_REGEX, files);
    if (files.size() >= static_cast<size_t>(maxFileNum)) {
        for (size_t index = 0; index <= files.size() - static_cast<size_t>(maxFileNum); ++index) {
            HIVIEW_LOGI("remove file=%{public}s", files[index].c_str());
            (void)FileUtil::RemoveFile(files[index]);
        }
    }

    uint64_t fileTime = TimeUtil::GetMilliseconds() / TimeUtil::SEC_TO_MILLISEC;
    std::string timeFormat = TimeUtil::TimestampFormatToDate(fileTime, "%Y%m%d%H%M%S");
    std::string fileName;
    fileName.append(path).append(prefix).append(timeFormat);
    if (!files.empty()) {
        auto startPos = files.back().find(timeFormat);
        if (startPos != std::string::npos) {
            int fileNameNum = GetFileNameNum(files.back().substr(startPos), ext); // yyyymmddHHMMSS_1.txt
            fileName.append(UNDERLINE).append(std::to_string(++fileNameNum));
        }
    }
    fileName.append(ext);
    (void)FileUtil::CreateFile(fileName);
    HIVIEW_LOGI("create file=%{public}s", fileName.c_str());
    return fileName;
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
