/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "file_util.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <istream>
#include <regex>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/xattr.h>
#include <unistd.h>
#include <vector>

#include "common_utils.h"
#include "directory_ex.h"
#include "file_ex.h"
#include "hiview_logger.h"

#define FDSAN_FILEUTIL_TAG 0xD002D10 // hiview domainid

namespace OHOS {
namespace HiviewDFX {
namespace FileUtil {
using namespace std;
namespace {
constexpr int VALUE_MOD = 200000;
DEFINE_LOG_TAG("FileUtil");

bool CheckAndCreateDirectory(const std::string &tmpDirPath)
{
    if (!FileExists(tmpDirPath)) {
        return ForceCreateDirectory(tmpDirPath, FILE_PERM_775);
    }
    return true;
}
}

bool LoadStringFromFile(const std::string& filePath, std::string& content)
{
    return OHOS::LoadStringFromFile(filePath, content);
}

bool LoadLinesFromFile(const std::string& filePath, std::vector<std::string>& lines)
{
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            lines.emplace_back(line);
        }
        file.close();
        return true;
    }
    return false;
}

bool LoadStringFromFd(int fd, std::string& content)
{
    return OHOS::LoadStringFromFd(fd, content);
}

bool SaveStringToFile(const std::string& filePath, const std::string& content, bool truncated)
{
    return OHOS::SaveStringToFile(filePath, content, truncated);
}

bool SaveStringToFd(int fd, const std::string& content)
{
    return OHOS::SaveStringToFd(fd, content);
}

bool FileExists(const std::string& fileName)
{
    return OHOS::FileExists(fileName);
}

std::string ExtractFilePath(const std::string& fileFullName)
{
    return OHOS::ExtractFilePath(fileFullName);
}

std::string ExtractFileName(const std::string& fileFullName)
{
    return OHOS::ExtractFileName(fileFullName);
}

std::string ExtractFileExt(const std::string& fileName)
{
    return OHOS::ExtractFileExt(fileName);
}

std::string IncludeTrailingPathDelimiter(const std::string& path)
{
    return OHOS::IncludeTrailingPathDelimiter(path);
}

std::string ExcludeTrailingPathDelimiter(const std::string& path)
{
    return OHOS::ExcludeTrailingPathDelimiter(path);
}

void GetDirFiles(const std::string& path, std::vector<std::string>& files, bool isRecursive)
{
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        return;
    }

    while (true) {
        struct dirent* ptr = readdir(dir);
        if (ptr == nullptr) {
            break;
        }

        // current dir or parent dir
        if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) {
            continue;
        } else if (ptr->d_type == DT_DIR && isRecursive) {
            std::string pathStringWithDelimiter = IncludeTrailingPathDelimiter(path) + string(ptr->d_name);
            GetDirFiles(pathStringWithDelimiter, files);
        } else {
            files.push_back(IncludeTrailingPathDelimiter(path) + string(ptr->d_name));
        }
    }
    closedir(dir);
}

void GetDirDirs(const std::string& path, std::vector<std::string>& dirs)
{
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        return;
    }

    while (true) {
        struct dirent *ptr = readdir(dir);
        if (ptr == nullptr) {
            break;
        }
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
            continue;
        } else if (ptr->d_type == DT_DIR) {
            dirs.push_back(IncludeTrailingPathDelimiter(path) + string(ptr->d_name));
        }
    }
    closedir(dir);
}

void GetDirFileInfos(const std::string& path, std::vector<std::pair<std::string, struct stat>>& fileInfos)
{
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        return;
    }
    while (true) {
        struct dirent *ptr = readdir(dir);
        if (ptr == nullptr) {
            break;
        }
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0 || ptr->d_type == DT_DIR) {
            continue;
        }
        string filePath = IncludeTrailingPathDelimiter(path) + string(ptr->d_name);
        struct stat fileStat;
        if (stat(filePath.c_str(), &fileStat) < 0) {
            continue;
        }
        fileInfos.emplace_back(filePath, fileStat);
    }
    closedir(dir);
}

bool ForceCreateDirectory(const string& path, mode_t mode)
{
    string::size_type index = 0;
    do {
        index = path.find('/', index + 1);
        string subPath = (index == string::npos) ? path : path.substr(0, index);
        if (access(subPath.c_str(), F_OK) != 0) {
            if (IsKeyDirectory(subPath)) {
                HIVIEW_LOGW("can not create key dir=%{public}s", subPath.c_str());
                return false;
            }
            if (mkdir(subPath.c_str(), mode) != 0) {
                return false;
            }
        }
    } while (index != string::npos);
    return access(path.c_str(), F_OK) == 0;
}

bool ForceRemoveDirectory(const std::string& path, bool isNeedDeleteGivenDirSelf)
{
    return OHOS::ForceRemoveDirectory(path);
}

uint64_t GetFileSize(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) ? 0 : static_cast<uint64_t>(st.st_size);
}

bool RemoveFile(const std::string& fileName)
{
    return remove(fileName.c_str()) == 0 || errno == ENOENT;
}

uint64_t GetFolderSize(const std::string& path)
{
    return OHOS::GetFolderSize(path);
}

double GetDeviceValidSize(const std::string& partitionName)
{
    struct statfs stat;
    int err = statfs(partitionName.c_str(), &stat);
    if (err != 0) {
        return 0;
    }
    return static_cast<double>(stat.f_bfree) * static_cast<double>(stat.f_bsize);
}

// inner function, and param is legitimate
bool ChangeMode(const string& fileName, const mode_t& mode)
{
    return (chmod(fileName.c_str(), mode) == 0);
}

bool ChangeModeFile(const string& fileName, const mode_t& mode)
{
    if (access(fileName.c_str(), F_OK) != 0) {
        return false;
    }

    return ChangeMode(fileName, mode);
}

bool ChangeModeDirectory(const std::string& path, const mode_t& mode)
{
    return OHOS::ChangeModeDirectory(path, mode);
}

bool PathToRealPath(const std::string& path, std::string& realPath)
{
    return OHOS::PathToRealPath(path, realPath);
}

void CreateDirWithDefaultPerm(const std::string& path, uid_t aidRoot, uid_t aidSystem)
{
    FileUtil::ForceCreateDirectory(path);
    chown(path.c_str(), aidRoot, aidSystem);
}

void FormatPath2UnixStyle(std::string &path)
{
    // unimplemented
}

int CreateFile(const std::string &path, mode_t mode)
{
    if (FileExists(path)) {
        return 0;
    } else {
        std::ofstream fout(path);
        if (!fout.is_open()) {
            return -1;
        }
        fout.flush();
        fout.close();
        if (!ChangeMode(path, mode)) {
            return -1;
        }
    }
    return 0;
}

int CopyFile(const std::string &src, const std::string &des)
{
    std::ifstream fin(src, ios::binary);
    std::ofstream fout(des, ios::binary);
    if (!fin.is_open()) {
        return -1;
    }
    if (!fout.is_open()) {
        return -1;
    }
    fout << fin.rdbuf();
    if (fout.fail()) {
        fout.clear();
    }
    fout.flush();
    return 0;
}

int CopyFileFast(const std::string &src, const std::string &des, uint32_t truncatedFileSize)
{
    int fdIn = open(src.c_str(), O_RDONLY);
    if (fdIn < 0) {
        return -1;
    }
    fdsan_exchange_owner_tag(fdIn, 0, FDSAN_FILEUTIL_TAG);
    int fdOut = open(des.c_str(), O_CREAT | O_RDWR, 0664);
    if (fdOut < 0) {
        fdsan_close_with_tag(fdIn, FDSAN_FILEUTIL_TAG);
        return -1;
    }
    fdsan_exchange_owner_tag(fdOut, 0, FDSAN_FILEUTIL_TAG);
    struct stat st;
    uint64_t totalLen = stat(src.c_str(), &st) ? 0 : static_cast<uint64_t>(st.st_size);
    std::string truncateMsg = "";
    if (truncatedFileSize != 0 && totalLen > truncatedFileSize) {
        totalLen = truncatedFileSize;
        truncateMsg = "\n[truncated]";
    }
    uint64_t copyTotalLen = 0;
    while (copyTotalLen < totalLen) {
        ssize_t copyLen = sendfile(fdOut, fdIn, nullptr, totalLen - copyTotalLen);
        if (copyLen <= 0) {
            break;
        }
        copyTotalLen += static_cast<uint64_t>(copyLen);
    }
    if (!truncateMsg.empty()) {
        SaveStringToFd(fdOut, truncateMsg);
    }
    fdsan_close_with_tag(fdIn, FDSAN_FILEUTIL_TAG);
    fdsan_close_with_tag(fdOut, FDSAN_FILEUTIL_TAG);
    return copyTotalLen == totalLen ? 0 : -1;
}

bool IsKeyDirectory(const std::string& dirPath)
{
    std::string keyDirPattern = R"(^/data/(?:app|service|chipset)?(?:/el\d+)?(?:/\d+)?/?$)";
    std::string fullDirPath = IncludeTrailingPathDelimiter(dirPath);
    return std::regex_match(fullDirPath, std::regex(keyDirPattern));
}

bool IsDirectory(const std::string &path)
{
    struct stat statBuffer;
    if (stat(path.c_str(), &statBuffer) == 0 && S_ISDIR(statBuffer.st_mode)) {
        return true;
    }
    return false;
}

bool GetLastLine(std::istream &fin, std::string &line, uint32_t maxLen)
{
    if (fin.tellg() <= 0) {
        return false;
    } else {
        fin.seekg(-1, fin.cur);
    }
    uint32_t count = 0;
    while (fin.good() && fin.peek() == fin.widen('\n') && fin.tellg() > 0 && count < maxLen) {
        fin.seekg(-1, fin.cur);
        count++;
    }
    if (!fin.good() || count >= maxLen) {
        return false;
    }
    if (fin.tellg() == 0) {
        return true;
    }
    count = 0;
    while (fin.good() && fin.peek() != fin.widen('\n') && fin.tellg() > 0 && count < maxLen) {
        fin.seekg(-1, fin.cur);
        count++;
    }
    if (!fin.good() || count >= maxLen) {
        return false;
    }
    if (fin.tellg() != 0) {
        fin.seekg(1, fin.cur);
    }
    auto oldPos = fin.tellg();
    getline(fin, line);
    fin.seekg(oldPos);
    return true;
}

std::string GetFirstLine(const std::string& path)
{
    std::ifstream inFile(path.c_str());
    if (!inFile) {
        return "";
    }
    std::string firstLine;
    getline(inFile, firstLine);
    inFile.close();
    return firstLine;
}

bool IsLegalPath(const std::string& path)
{
    if (path.find("./") != std::string::npos ||
        path.find("../") != std::string::npos) {
        return false;
    }
    return true;
}

bool RenameFile(const std::string& src, const std::string& dest)
{
    if (std::rename(src.c_str(), dest.c_str()) == 0) {
        return true;
    }
    return false;
}

bool GetDirXattr(const std::string& dir, const std::string& name, std::string& value)
{
    char buf[BUF_SIZE_256] = {0};
    if (getxattr(dir.c_str(), name.c_str(), buf, BUF_SIZE_256) == -1) {
        return false;
    }
    value = buf;
    return true;
}

int64_t GetLastModifiedTimeStamp(const std::string& filePath)
{
    struct stat fileInfo {0};
    if (stat(filePath.c_str(), &fileInfo) != ERR_OK) {
        return 0;
    }
    return fileInfo.st_mtime;
}

int GetUserId(int32_t uid)
{
    return uid / VALUE_MOD;
}

std::string GetSandBoxLogPath(int32_t uid, const std::string& pathHolder, const std::string& subPath)
{
    int userId = GetUserId(uid);
    if (pathHolder.empty()) {
        return "";
    }
    return "/data/app/el2/" + std::to_string(userId) + "/log/" + pathHolder + "/" + subPath;
}

std::string GetSandBoxBasePath(int32_t uid, const std::string& pathHolder)
{
    int userId = GetUserId(uid);
    if (pathHolder.empty()) {
        return "";
    }
    return "/data/app/el2/" + std::to_string(userId) + "/base/" + pathHolder + "/cache/hiappevent";
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
            if (!CheckAndCreateDirectory(tmpDirPath)) {
                return false;
            }
        }
    }
    return true;
}

bool IsSymlink(const std::string& path)
{
    struct stat pathStat;
    if (lstat(path.c_str(), &pathStat) != 0) {
        return false;
    }
    return S_ISLNK(pathStat.st_mode);
}

std::string ReadSymlink(const std::string &path)
{
    char buffer[PATH_MAX] = {0};
    ssize_t readSize = readlink(path.c_str(), buffer, sizeof(buffer) - 1);
    if (readSize >= 0) {
        buffer[readSize] = '\0';
        return std::string(buffer);
    }
    return "";
}
} // namespace FileUtil
} // namespace HiviewDFX
} // namespace OHOS
