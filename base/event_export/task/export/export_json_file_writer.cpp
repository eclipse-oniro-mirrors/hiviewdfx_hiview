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

#include "export_json_file_writer.h"

#include <chrono>
#include <cstdio>
#include <tuple>
#include <sstream>

#include "file_util.h"
#include "hiview_global.h"
#include "hiview_logger.h"
#include "hiview_zip_util.h"
#include "parameter.h"
#include "parameter_ex.h"
#include "time_util.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-ExportJsonFileWriter");
namespace {
constexpr int64_t MB_TO_BYTE = 1024 * 1024;
constexpr int64_t INVALID_EVENT_SEQ = -1;
constexpr char SYSEVENT_EXPORT_TMP_DIR[] = "tmp";
constexpr char SYSEVENT_EXPORT_DIR[] = "sys_event_export";
constexpr char EXPORT_JSON_FILE_NAME[] = "HiSysEvent.json";
constexpr char SYS_EVENT_EXPORT_DIR_NAME[] = "sys_event_export";
constexpr char ZIP_FILE_DELIM[] = "_";
constexpr char CUR_HEADER_VERSION[] = "1.0";
constexpr char H_HEADER_KEY[] = "HEADER";
constexpr char H_VERSION_KEY[] = "VERSION";
constexpr char H_MSG_ID_KEY[] = "MESSAGE_ID";
constexpr char H_MANUFACTURE_KEY[] = "MANUFACTURER";
constexpr char H_NAME_KEY[] = "NAME";
constexpr char H_BRAND_KEY[] = "BRAND";
constexpr char H_DEVICE_KEY[] = "DEVICE";
constexpr char H_ID_KEY[] = "ID";
constexpr char H_MODEL_KEY[] = "MODEL";
constexpr char H_CATEGORY_KEY[] = "CATEGORY";
constexpr char H_SYSTEM_KEY[] = "SYSTEM";
constexpr char H_OHOS_VER_KEY[] = "OHOS_VER";
constexpr char DOMAINS_KEY[] = "DOMAINS";
constexpr char DOMAIN_INFO_KEY[] = "DOMAIN_INFO";
constexpr char EVENTS_KEY[] = "EVENTS";
constexpr char DATA_KEY[] = "DATA";
constexpr char DEFAULT_MSG_ID[] = "00000000000000000000000000000000";

std::string GenerateDeviceId()
{
    constexpr int32_t deviceIdLength = 65;
    char id[deviceIdLength] = {0};
    if (GetDevUdid(id, deviceIdLength) == 0) {
        return std::string(id);
    }
    return "";
}

std::string GetDeviceId()
{
    static std::string deviceId = GenerateDeviceId();
    return deviceId;
}
 
cJSON* CreateHeaderJsonObj()
{
    cJSON* header = cJSON_CreateObject();
    if (header == nullptr) {
        HIVIEW_LOGE("failed to create header json object");
        return nullptr;
    }
    cJSON_AddStringToObject(header, H_VERSION_KEY, CUR_HEADER_VERSION);
    cJSON_AddStringToObject(header, H_MSG_ID_KEY, DEFAULT_MSG_ID);
    return header;
}

cJSON* CreateManufacturerJsonObj()
{
    cJSON* manufacturer = cJSON_CreateObject();
    if (manufacturer == nullptr) {
        HIVIEW_LOGE("failed to create manufacturer json object");
        return nullptr;
    }
    cJSON_AddStringToObject(manufacturer, H_NAME_KEY, Parameter::GetManufactureStr().c_str());
    cJSON_AddStringToObject(manufacturer, H_BRAND_KEY, Parameter::GetBrandStr().c_str());
    return manufacturer;
}

cJSON* CreateDeviceJsonObj()
{
    cJSON* device = cJSON_CreateObject();
    if (device == nullptr) {
        HIVIEW_LOGE("failed to create device json object");
        return nullptr;
    }
    cJSON_AddStringToObject(device, H_ID_KEY, GetDeviceId().c_str());
    cJSON_AddStringToObject(device, H_MODEL_KEY, Parameter::GetProductModelStr().c_str());
    cJSON_AddStringToObject(device, H_NAME_KEY, Parameter::GetMarketNameStr().c_str());
    cJSON_AddStringToObject(device, H_CATEGORY_KEY, Parameter::GetDeviceTypeStr().c_str());
    return device;
}

cJSON* CreateSystemObj(const std::string& sysVersion)
{
    cJSON* system = cJSON_CreateObject();
    if (system == nullptr) {
        HIVIEW_LOGE("failed to create system json object");
        return nullptr;
    }
    cJSON_AddStringToObject(system, H_VERSION_KEY, sysVersion.c_str());
    cJSON_AddStringToObject(system, H_OHOS_VER_KEY, Parameter::GetSysVersionDetailsStr().c_str());
    return system;
}

cJSON* CreateJsonObjectByVersion(const std::string& sysVersion)
{
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        HIVIEW_LOGE("failed to create root json object");
        return nullptr;
    }
    // add header
    auto headerObj = CreateHeaderJsonObj();
    if (headerObj != nullptr) {
        cJSON_AddItemToObject(root, H_HEADER_KEY, headerObj);
    }
    // add manufacturer
    auto manufacturerObj = CreateManufacturerJsonObj();
    if (manufacturerObj != nullptr) {
        cJSON_AddItemToObject(root, H_MANUFACTURE_KEY, manufacturerObj);
    }
    auto deviceObj = CreateDeviceJsonObj();
    if (deviceObj != nullptr) {
        cJSON_AddItemToObject(root, H_DEVICE_KEY, deviceObj);
    }
    auto systemObj = CreateSystemObj(sysVersion);
    if (systemObj != nullptr) {
        cJSON_AddItemToObject(root, H_SYSTEM_KEY, systemObj);
    }
    return root;
}

cJSON* CreateEventsJsonArray(const std::string& domain,
    const std::unordered_map<int64_t, std::pair<std::string, std::string>>& events)
{
    // events
    cJSON* eventsJsonArray = cJSON_CreateArray();
    if (eventsJsonArray == nullptr) {
        HIVIEW_LOGE("failed to create events json array");
        return nullptr;
    }
    cJSON* dataJsonArray = cJSON_CreateArray();
    if (dataJsonArray == nullptr) {
        HIVIEW_LOGE("failed to create data json array");
        return nullptr;
    }
    for (const auto& event : events) {
        cJSON* eventItem = cJSON_Parse(event.second.second.c_str());
        cJSON_AddItemToArray(dataJsonArray, eventItem);
    }
    cJSON* anonymousJsonObj = cJSON_CreateObject();
    if (anonymousJsonObj == nullptr) {
        HIVIEW_LOGE("failed to create anonymousJsonObj json object");
        return nullptr;
    }
    cJSON_AddItemToObject(anonymousJsonObj, DATA_KEY, dataJsonArray);
    cJSON_AddItemToArray(eventsJsonArray, anonymousJsonObj);
    return eventsJsonArray;
}

std::string GetHiSysEventJsonTempDir(const std::string& moduleName, const std::string version)
{
    auto& context = HiviewGlobal::GetInstance();
    if (context == nullptr) {
        return "";
    }
    std::string tmpDir = context->GetHiViewDirectory(HiviewContext::DirectoryType::WORK_DIRECTORY);
    tmpDir = FileUtil::IncludeTrailingPathDelimiter(tmpDir.append(SYSEVENT_EXPORT_DIR));
    tmpDir = FileUtil::IncludeTrailingPathDelimiter(tmpDir.append(SYSEVENT_EXPORT_TMP_DIR));
    tmpDir = FileUtil::IncludeTrailingPathDelimiter(tmpDir.append(moduleName));
    tmpDir = FileUtil::IncludeTrailingPathDelimiter(tmpDir.append(version));
    if (!FileUtil::IsDirectory(tmpDir) && !FileUtil::ForceCreateDirectory(tmpDir)) {
        return "";
    }
    auto ret = FileUtil::ChangeModeDirectory(tmpDir, FileUtil::DEFAULT_FILE_MODE | S_IXUSR | S_IWOTH | S_IXOTH);
    if (!ret) {
        HIVIEW_LOGE("failed to chmod directory %{public}s.", tmpDir.c_str());
    }
    return tmpDir;
}

bool ZipDbFile(const std::string& src, const std::string& dest)
{
    HIVIEW_LOGI("zip file: %{public}s to %{public}s", src.c_str(), dest.c_str());
    bool ret = HiviewZipUtil::ZipFile(src, dest);
    if (!ret) {
        return false;
    }
    ret = FileUtil::ChangeModeFile(dest, FileUtil::DEFAULT_FILE_MODE | S_IXUSR | S_IWOTH | S_IXOTH);
    if (!ret) {
        HIVIEW_LOGE("failed to chmod file %{public}s.", dest.c_str());
        return false;
    }
    // delete json file
    FileUtil::RemoveFile(src);
    return true;
}

void AppendZipFile(std::string& dir)
{
    dir.append("HSE").append(ZIP_FILE_DELIM)
        .append(Parameter::GetBrandStr()).append(std::string(ZIP_FILE_DELIM))
        .append(Parameter::GetProductModelStr()).append(std::string(ZIP_FILE_DELIM))
        .append(Parameter::GetSysVersionStr()).append(std::string(ZIP_FILE_DELIM))
        .append(GetDeviceId()).append(std::string(ZIP_FILE_DELIM))
        .append(TimeUtil::GetFormattedTimestampEndWithMilli()).append(std::string(".zip"));
}

std::string GetTmpZipFile(const std::string& baseDir, const std::string& moduleName,
    const std::string& version)
{
    std::string dir = FileUtil::IncludeTrailingPathDelimiter(baseDir);
    dir = FileUtil::IncludeTrailingPathDelimiter(dir.append(SYSEVENT_EXPORT_TMP_DIR));
    dir = FileUtil::IncludeTrailingPathDelimiter(dir.append(moduleName));
    dir = FileUtil::IncludeTrailingPathDelimiter(dir.append(version));
    if (!FileUtil::IsDirectory(dir) && !FileUtil::ForceCreateDirectory(dir)) {
        return "";
    }
    auto ret = FileUtil::ChangeModeDirectory(dir, FileUtil::DEFAULT_FILE_MODE | S_IXUSR | S_IWOTH | S_IXOTH);
    if (!ret) {
        HIVIEW_LOGE("failed to chmod directory %{public}s.", dir.c_str());
    }
    AppendZipFile(dir);
    return dir;
}

std::string GetZipFile(const std::string& baseDir)
{
    std::string dir = FileUtil::IncludeTrailingPathDelimiter(baseDir);
    dir = FileUtil::IncludeTrailingPathDelimiter(dir.append(SYS_EVENT_EXPORT_DIR_NAME));
    if (!FileUtil::IsDirectory(dir) && !FileUtil::ForceCreateDirectory(dir)) {
        return "";
    }
    auto ret = FileUtil::ChangeModeDirectory(dir, FileUtil::DEFAULT_FILE_MODE | S_IXUSR | S_IWOTH | S_IXOTH);
    if (!ret) {
        HIVIEW_LOGE("failed to chmod directory %{public}s.", dir.c_str());
    }
    AppendZipFile(dir);
    return dir;
}

void PersistJsonStrToLocalFile(cJSON* root, const std::string& localFile)
{
    if (root == nullptr) {
        HIVIEW_LOGE("root of json is null");
        return;
    }
    char* parsedJsonStr = cJSON_PrintUnformatted(root);
    if (parsedJsonStr == nullptr) {
        HIVIEW_LOGE("formatted json str is null");
        return;
    }
    FILE* file = fopen(localFile.c_str(), "w+");
    if (file == nullptr) {
        HIVIEW_LOGE("failed to open file: %{public}s.", localFile.c_str());
        cJSON_free(parsedJsonStr);
        return;
    }
    (void)fprintf(file, "%s", parsedJsonStr);
    (void)fclose(file);
    cJSON_free(parsedJsonStr);
}
}

bool ExportJsonFileWriter::PackJsonStrToFile(EventsDividedInDomainGroupType& cachedToPackEvents)
{
    cJSON* root = CreateJsonObjectByVersion(eventVersion_);
    if (root == nullptr) {
        return false;
    }
    cJSON* domainsJsonArray = cJSON_CreateArray();
    if (domainsJsonArray == nullptr) {
        return false;
    }
    for (const auto& cachedToPackEvent : cachedToPackEvents) {
        cJSON* domainJsonObj = cJSON_CreateObject();
        if (domainJsonObj == nullptr) {
            continue;
        }
        // domain info
        cJSON* domainInfoJsonObj = cJSON_CreateObject();
        if (domainInfoJsonObj == nullptr) {
            HIVIEW_LOGE("failed to create domain info json object");
            continue;
        }
        cJSON_AddStringToObject(domainInfoJsonObj, H_NAME_KEY, cachedToPackEvent.first.c_str());
        cJSON_AddItemToObject(domainJsonObj, DOMAIN_INFO_KEY, domainInfoJsonObj);
        cJSON* eventsJsonObj = CreateEventsJsonArray(cachedToPackEvent.first, cachedToPackEvent.second);
        if (eventsJsonObj == nullptr) {
            continue;
        }
        cJSON_AddItemToObject(domainJsonObj, EVENTS_KEY, eventsJsonObj);
        cJSON_AddItemToArray(domainsJsonArray, domainJsonObj);
    }
    // add domains
    cJSON_AddItemToObject(root, DOMAINS_KEY, domainsJsonArray);
    // create export json file HiSysEvent.json
    auto packagedFile = GetHiSysEventJsonTempDir(moduleName_, eventVersion_).append(EXPORT_JSON_FILE_NAME);
    HIVIEW_LOGD("packagedFile: %{public}s", packagedFile.c_str());
    PersistJsonStrToLocalFile(root, packagedFile);
    // zip json file into a temporary zip file
    auto tmpZipFile = GetTmpZipFile(exportDir_, moduleName_, eventVersion_);
    if (!ZipDbFile(packagedFile, tmpZipFile)) {
        HIVIEW_LOGE("failed to zip %{public}s to %{public}s", packagedFile.c_str(), tmpZipFile.c_str());
        return false;
    }
    // move tmp zip file to output directory
    auto zipFile = GetZipFile(exportDir_);
    HIVIEW_LOGD("zipFile: %{public}s", zipFile.c_str());
    if (!FileUtil::RenameFile(tmpZipFile, zipFile)) {
        HIVIEW_LOGE("failed to rename %{public}s to %{public}s", tmpZipFile.c_str(), zipFile.c_str());
        return false;
    }
    cJSON_Delete(root);
    totalJsonStrSize_ = 0;
    cachedToPackEvents.clear(); // clear cache;
    return true;
}

ExportJsonFileWriter::ExportJsonFileWriter(const std::string& moduleName, const std::string& eventVersion,
    const std::string& exportDir, int64_t maxFileSize)
{
    moduleName_ = moduleName;
    eventVersion_ = eventVersion;
    exportDir_ = exportDir;
    maxFileSize_ = maxFileSize * MB_TO_BYTE;
}

void ExportJsonFileWriter::SetMaxSequenceWriteListener(MaxSequenceWriteListener listener)
{
    maxSequenceWriteListener_ = listener;
}

bool ExportJsonFileWriter::AppendEvent(const std::string& domain, int64_t seq, const std::string& name,
    const std::string& eventStr)
{
    int64_t eventSize = static_cast<int64_t>(eventStr.size());
    maxEventSeq_ = std::max(maxEventSeq_, seq);
    if (totalJsonStrSize_ + eventSize > maxFileSize_ && !Write()) {
        HIVIEW_LOGE("failed to write export events");
        return false;
    }
    auto iter = eventInDomains_.find(domain);
    if (iter == eventInDomains_.end()) {
        eventInDomains_.emplace(domain, std::unordered_map<int64_t, std::pair<std::string, std::string>> {
            {seq, std::make_pair(name, eventStr)},
        });
        totalJsonStrSize_ += eventSize;
        return true;
    }
    iter->second.emplace(seq, std::make_pair(name, eventStr));
    totalJsonStrSize_ += eventSize;
    return true;
}

bool ExportJsonFileWriter::Write(bool isLastPartialQuery)
{
    bool ret = PackJsonStrToFile(eventInDomains_);
    if (ret && isLastPartialQuery && maxSequenceWriteListener_ != nullptr && maxEventSeq_ != INVALID_SEQ_VAL) {
        maxSequenceWriteListener_(maxEventSeq_);
    }
    return ret;
}
} // HiviewDFX
} // OHOS