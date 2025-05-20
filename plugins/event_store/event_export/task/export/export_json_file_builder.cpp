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

#include "export_json_file_builder.h"

#include "event_export_util.h"
#include "focused_event_util.h"
#include "hiview_logger.h"
#include "parameter.h"
#include "parameter_ex.h"

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_TAG("HiView-EventExportFlow");
namespace {
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
constexpr char H_PATCH_VER_KEY[] = "PATCH_VER";
constexpr char DOMAINS_KEY[] = "DOMAINS";
constexpr char DOMAIN_INFO_KEY[] = "DOMAIN_INFO";
constexpr char EVENTS_KEY[] = "EVENTS";
constexpr char DATA_KEY[] = "DATA";
constexpr char DEFAULT_MSG_ID[] = "00000000000000000000000000000000";
constexpr char CUR_HEADER_VERSION[] = "1.0";

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
    cJSON_AddStringToObject(device, H_ID_KEY, EventExportUtil::GetDeviceId().c_str());
    cJSON_AddStringToObject(device, H_MODEL_KEY, Parameter::GetProductModelStr().c_str());
    cJSON_AddStringToObject(device, H_NAME_KEY, Parameter::GetMarketNameStr().c_str());
    cJSON_AddStringToObject(device, H_CATEGORY_KEY, Parameter::GetDeviceTypeStr().c_str());
    return device;
}

cJSON* CreateSystemObj(const EventVersion& eventVersion)
{
    cJSON* system = cJSON_CreateObject();
    if (system == nullptr) {
        HIVIEW_LOGE("failed to create system json object");
        return nullptr;
    }
    cJSON_AddStringToObject(system, H_VERSION_KEY, eventVersion.systemVersion.c_str());
    cJSON_AddStringToObject(system, H_OHOS_VER_KEY, Parameter::GetSysVersionDetailsStr().c_str());
    cJSON_AddStringToObject(system, H_PATCH_VER_KEY, eventVersion.patchVersion.c_str());
    return system;
}

cJSON* CreateEventsJsonArray(const std::string& domain,
    const std::vector<std::pair<std::string, std::string>>& events)
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
        cJSON_Delete(eventsJsonArray);
        return nullptr;
    }
    for (const auto& event : events) {
        cJSON* eventItem = cJSON_Parse(event.second.c_str());
        if (FocusedEventUtil::IsFocusedEvent(domain, event.first)) {
            HIVIEW_LOGI("write event to json: [%{public}s|%{public}s]", domain.c_str(),
                event.first.c_str());
        }
        if (eventItem == nullptr) {
            HIVIEW_LOGW("failed to create json for event: [%{public}s|%{public}s]", domain.c_str(),
                event.first.c_str());
            continue;
        }
        cJSON_AddItemToArray(dataJsonArray, eventItem);
    }
    cJSON* anonymousJsonObj = cJSON_CreateObject();
    if (anonymousJsonObj == nullptr) {
        HIVIEW_LOGE("failed to create anonymousJsonObj json object");
        cJSON_Delete(dataJsonArray);
        cJSON_Delete(eventsJsonArray);
        return nullptr;
    }
    cJSON_AddItemToObjectCS(anonymousJsonObj, DATA_KEY, dataJsonArray);
    cJSON_AddItemToArray(eventsJsonArray, anonymousJsonObj);
    return eventsJsonArray;
}

cJSON* CreateDomainsJson(const CachedEventMap& sysEvents)
{
    cJSON* domainsArrayJson = cJSON_CreateArray();
    if (domainsArrayJson == nullptr) {
        HIVIEW_LOGE("failed to create json array");
        return nullptr;
    }
    for (const auto& sysEvent : sysEvents) {
        cJSON* domainJsonObj = cJSON_CreateObject();
        if (domainJsonObj == nullptr) {
            continue;
        }
        // domain info
        cJSON* domainInfoJsonObj = cJSON_CreateObject();
        if (domainInfoJsonObj == nullptr) {
            HIVIEW_LOGE("failed to create domain info json object");
            cJSON_Delete(domainJsonObj);
            continue;
        }
        cJSON_AddStringToObject(domainInfoJsonObj, H_NAME_KEY, sysEvent.first.c_str());
        cJSON_AddItemToObjectCS(domainJsonObj, DOMAIN_INFO_KEY, domainInfoJsonObj);
        cJSON* eventsJsonObj = CreateEventsJsonArray(sysEvent.first, sysEvent.second);
        if (eventsJsonObj == nullptr) {
            HIVIEW_LOGE("failed to create json object for event array");
            cJSON_Delete(domainJsonObj);
            continue;
        }
        cJSON_AddItemToObjectCS(domainJsonObj, EVENTS_KEY, eventsJsonObj);
        cJSON_AddItemToArray(domainsArrayJson, domainJsonObj);
    }
    return domainsArrayJson;
}

bool PersistJsonStr(cJSON* root, std::string& ret)
{
    char* parsedJsonStr = cJSON_PrintUnformatted(root);
    if (parsedJsonStr == nullptr) {
        HIVIEW_LOGE("formatted json str is null");
        return false;
    }
    ret = std::string(parsedJsonStr);
    cJSON_free(parsedJsonStr);
    return true;
}
}

bool ExportJsonFileBuilder::Build(const CachedEventMap& eventMap, std::string& buildStr)
{
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        HIVIEW_LOGE("failed to create event json root");
        return false;
    }

    if (!BuildHeader(root)) {
        HIVIEW_LOGE("failed to build event json file header");
        cJSON_Delete(root);
        return false;
    }
    if (!BuildContent(root, eventMap)) {
        HIVIEW_LOGE("failed to build event json file content");
        cJSON_Delete(root);
        return false;
    }
    bool peristRet = PersistJsonStr(root, buildStr);
    cJSON_Delete(root);
    return peristRet;
}

bool ExportJsonFileBuilder::BuildHeader(cJSON* root)
{
    // add header
    auto headerObj = CreateHeaderJsonObj();
    if (headerObj == nullptr) {
        return false;
    }
    cJSON_AddItemToObjectCS(root, H_HEADER_KEY, headerObj);
    // add manufacturer
    auto manufacturerObj = CreateManufacturerJsonObj();
    if (manufacturerObj == nullptr) {
        return false;
    }
    cJSON_AddItemToObjectCS(root, H_MANUFACTURE_KEY, manufacturerObj);
    // add device info
    auto deviceObj = CreateDeviceJsonObj();
    if (deviceObj == nullptr) {
        return false;
    }
    cJSON_AddItemToObjectCS(root, H_DEVICE_KEY, deviceObj);
    // add system version info
    auto systemObj = CreateSystemObj(eventVersion_);
    if (systemObj == nullptr) {
        return false;
    }
    cJSON_AddItemToObjectCS(root, H_SYSTEM_KEY, systemObj);
    return true;
}

bool ExportJsonFileBuilder::BuildContent(cJSON* root, const CachedEventMap& eventMap)
{
    // add domains
    cJSON* domainsJson = CreateDomainsJson(eventMap);
    if (domainsJson == nullptr) {
        return false;
    }
    cJSON_AddItemToObjectCS(root, DOMAINS_KEY, domainsJson);
    return true;
}
} // HiviewDFX
} // OHOS