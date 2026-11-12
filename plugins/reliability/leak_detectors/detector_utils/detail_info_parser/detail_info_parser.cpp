/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "detail_info_parser.h"
#include "fault_common.h"
#include "dfr_util.h"
#include <iostream>
#include <hiview_logger.h>

using namespace std;

namespace OHOS {
namespace HiviewDFX {
DEFINE_LOG_LABEL(0xD002DA0, "DetailInfoParser");

namespace {
const string UNDEFINE = "UNDEF";
constexpr int BYTE_TO_KB_SHIFT = 10;  // 2^10 bytes per KB
// DMA
const string PIXELMAP_SIZE = "PIXELMAP_SIZE";
const string XCOM_SIZE = "XCOM_SIZE";
const string WEB_SIZE = "WEB_SIZE";
const string HW_VIDEO_SIZE = "HW_VIDEO_SIZE";
const string SW_VIDEO_SIZE = "SW_VIDEO_SIZE";
const string EXTERNAL_SIZE = "EXTERNAL_SIZE";
const string OTHERS_SIZE = "OTHERS_SIZE";

// GPU
const string VULKAN_DEFAULT = "VULKAN_DEFAULT";
const string VULKAN_IMAGE = "VULKAN_IMAGE";
const string VULKAN_BUFFER = "VULKAN_BUFFER";
const string GLES_IMAGE = "GLES_IMAGE";
const string GLES_BUFFER = "GLES_BUFFER";
const string CL_IMAGE = "CL_IMAGE";
const string CL_BUFFER = "CL_BUFFER";

// DMA
const unordered_map<string, int32_t DetailDmaBufInfo::*> DmaBufTypeMap = {
    {PIXELMAP_SIZE, &DetailDmaBufInfo::pixelmapSize},
    {XCOM_SIZE, &DetailDmaBufInfo::xcomSize},
    {WEB_SIZE, &DetailDmaBufInfo::webSize},
    {HW_VIDEO_SIZE, &DetailDmaBufInfo::hwVideoSize},
    {SW_VIDEO_SIZE, &DetailDmaBufInfo::swVideoSize},
    {EXTERNAL_SIZE, &DetailDmaBufInfo::externalSize},
    {OTHERS_SIZE, &DetailDmaBufInfo::othersSize}};

const unordered_map<string, string> DMABUFTYPE_ALIGNMAP = {
    {"pixelmap", PIXELMAP_SIZE},
    {"xcomponent-", XCOM_SIZE},
    {"web-", WEB_SIZE},
    {"hw-video-", HW_VIDEO_SIZE},
    {"sw-video-", SW_VIDEO_SIZE},
    {"external", EXTERNAL_SIZE}};

// GPU
const unordered_map<string, int32_t DetailGpuInfo::*> GpuTypeMap = {
    {VULKAN_DEFAULT, &DetailGpuInfo::vulKanDefault},
    {VULKAN_IMAGE, &DetailGpuInfo::vulKanImage},
    {VULKAN_BUFFER, &DetailGpuInfo::vulKanBuffer},
    {GLES_IMAGE, &DetailGpuInfo::glesImage},
    {GLES_BUFFER, &DetailGpuInfo::glesBuffer},
    {CL_IMAGE, &DetailGpuInfo::clImage},
    {CL_BUFFER, &DetailGpuInfo::clBuffer}};

const vector<string> MEMORY_CLASS_VEC = {
    "graph", "ark ts heap", "arkts-static heap", ".db", "dev", "dmabuf", "guard", ".hap",
    "native heap", ".so", "stack", ".ttf", "jsvm heap", "arkweb-js heap", "arkweb-pa heap", "kotlin heap",
    "rn-hermes heap", "dart heap", "other"
};
// Smaps - 直接使用原始category名称作为key
const unordered_map<string, int DetailSmapsInfo::*> SmapsTypeMap = {
    {"native heap", &DetailSmapsInfo::nativeHeap},
    {"ark ts heap", &DetailSmapsInfo::arktsHeap},
    {".hap", &DetailSmapsInfo::hap},
    {"stack", &DetailSmapsInfo::stack},
    {".db", &DetailSmapsInfo::db},
    {".so", &DetailSmapsInfo::so},
    {"dev", &DetailSmapsInfo::dev},
    {".ttf", &DetailSmapsInfo::ttf},
    {"ashmem", &DetailSmapsInfo::ashmem},
    {"arkts-static heap", &DetailSmapsInfo::arktsStaHeap},
    {"kotlin heap", &DetailSmapsInfo::kotlinHeap},
    {"dart heap", &DetailSmapsInfo::dartHeap},
    {"rn-hermes heap", &DetailSmapsInfo::rnHermesHeap},
    {"jsvm heap", &DetailSmapsInfo::jsvmHeap},
    {"arkweb-js heap", &DetailSmapsInfo::arkwebV8},
    {"arkweb-pa heap", &DetailSmapsInfo::arkwebPa},
    {"AnonPage other", &DetailSmapsInfo::anonPageOther},
    {"FilePage other", &DetailSmapsInfo::filePageOther},
    {"other", &DetailSmapsInfo::others}};
}

DetailInfoParser::DetailInfoParser() {};

DetailInfoParser::~DetailInfoParser() {};

DetailDmaBufInfo DetailInfoParser::ParseIonProcessInfo(const string &data)
{
    DetailDmaBufInfo detailDmaBufInfo;
    if (data.empty()) {
        HIVIEW_LOGE("data from ioctl is empty");
        return detailDmaBufInfo;
    }

    int sizeColumnIndex = -1;
    int typeColumnIndex = -1;
    const auto totalInfo = DfrUtil::GetInstance().Split(data, '\n');
    if (totalInfo.empty()) {
        HIVIEW_LOGE("Split result is empty");
        return detailDmaBufInfo;
    }
    string title = totalInfo[0];
    const auto titleInfo = DfrUtil::GetInstance().Split(title, ' ');
    for (size_t i = 0; i < titleInfo.size(); i++) {
        if (titleInfo[i].find("size_bytes") != string::npos) {
            sizeColumnIndex = static_cast<int>(i);
        }
        if (titleInfo[i].find("leak_type") != string::npos) {
            typeColumnIndex = static_cast<int>(i);
        }
    }

    if (sizeColumnIndex == -1 || typeColumnIndex == -1) {
        HIVIEW_LOGE("Missing required columns in title, title is %{public}s", title.c_str());
        return detailDmaBufInfo;
    }

    for (size_t i = 1; i < totalInfo.size(); i++) {
        vector<string> dmabufInfo = DfrUtil::GetInstance().Split(totalInfo[i], ' ');
        if (static_cast<size_t>(sizeColumnIndex) >= dmabufInfo.size() ||
            static_cast<size_t>(typeColumnIndex) >= dmabufInfo.size()) {
            HIVIEW_LOGE("Index Out of Bounds Address. sizeColumnIndex is %{public}d, typeColumnIndex is %{public}d",
                        sizeColumnIndex, typeColumnIndex);
            return detailDmaBufInfo;
        }
        int size_bytes = 0;
        if (!DfrUtil::GetInstance().SafeStoi(dmabufInfo[sizeColumnIndex], size_bytes)) {
            HIVIEW_LOGE("failed to stoi, str: %{public}s", dmabufInfo[sizeColumnIndex].c_str());
            return detailDmaBufInfo;
        }
        AddDmabufSize(detailDmaBufInfo, AlignType(dmabufInfo[typeColumnIndex]), size_bytes / BYTE_PER_KB);
    }

    return detailDmaBufInfo;
}

DetailDmaBufInfo DetailInfoParser::GetIonProcessInfoAndParse(pid_t pid)
{
    string nodePath = "/proc/" + to_string(pid) + "/mm_dmabuf_info";
    std::ifstream file(nodePath);
    if (!file.is_open()) {
        HIVIEW_LOGE("can't open file : %{public}s", nodePath.c_str());
        DetailDmaBufInfo emptyInfo;
        return emptyInfo;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return ParseIonProcessInfo(buffer.str());
}

string DetailInfoParser::AlignType(const string &originalDmaBufType)
{
    for (const auto &[oldType, newType] : DMABUFTYPE_ALIGNMAP) {
        if (originalDmaBufType.find(oldType) != string::npos) {
            return newType;
        }
    }
    return OTHERS_SIZE;
}

void DetailInfoParser::AddDmabufSize(DetailDmaBufInfo &detailDmaBufInfo, const string &dmaBufType,
    const int &sizeBytes)
{
    auto it = DmaBufTypeMap.find(dmaBufType);
    if (it != DmaBufTypeMap.end()) {
        detailDmaBufInfo.*(it->second) += sizeBytes;
    } else {
        HIVIEW_LOGE("undefined dmabuf type : %{public}s", dmaBufType.c_str());
        return;
    }
    detailDmaBufInfo.totalSize += sizeBytes;
    return;
}

DetailGpuInfo DetailInfoParser::ParseGpuProcessInfo(const string &data)
{
    DetailGpuInfo detailGpuInfo;

    if (data.empty()) {
        HIVIEW_LOGE("data from ioctl is empty");
        return detailGpuInfo;
    }
    if (data.find("used summary:") != string::npos) {
        DetailInfoParser::ComputeGpuHookSize(data, detailGpuInfo);
    }
    return detailGpuInfo;
}

DetailGpuInfo DetailInfoParser::GetGpuProcessInfoAndParse(pid_t pid)
{
    string gpuData = DfrUtil::GetInstance().SendLeakCmdInfo(pid, CMD_GET_MEM_DETAIL_INFO, MTYPE_USER_GPU);
    return ParseGpuProcessInfo(gpuData);
}

void DetailInfoParser::ComputeGpuHookSize(const string &data, DetailGpuInfo &detailGpuInfo)
{
    string line;
    stringstream finBufferStream(data);
    // judege GpuHookType when "C:"
    if (data.find("C: ") != string::npos) {
        while (getline(finBufferStream, line)) {
            JudgeCGpuType(line, detailGpuInfo);
        }
        return;
    }
    if (data.find("Channel: ") != string::npos) {
        while (getline(finBufferStream, line)) {
            JudgeChannelGpuType(line, detailGpuInfo);
        }
        return;
    }
}

// 按 hookMap 前缀匹配行，命中则解析 "Total memory: xxx)"，换算为 KB 后写入对应 GPU 字段
static void AssignGpuMemoryByHookMap(const string &line, const unordered_map<string, string> &hookMap,
    DetailGpuInfo &detailGpuInfo)
{
    for (const auto &it : hookMap) {
        if (line.find(it.first) == string::npos) {
            continue;
        }
        auto fieldIt = GpuTypeMap.find(it.second);
        if (fieldIt == GpuTypeMap.end()) {
            continue;
        }
        int32_t totalBytes = static_cast<int32_t>(DfrUtil::GetInstance().GetDegital(line, "Total memory: ", ")"));
        detailGpuInfo.*(fieldIt->second) = totalBytes >> BYTE_TO_KB_SHIFT;
    }
}

void DetailInfoParser::JudgeCGpuType(const string &line, DetailGpuInfo &detailGpuInfo)
{
    const unordered_map<string, string> gpuHookMap = {
        {"C: vulkan default device (Total memory", VULKAN_DEFAULT},
        {"C: vulkan image (Total memory", VULKAN_IMAGE},
        {"C: vulkan buffer (Total memory", VULKAN_BUFFER},
        {"C: gles image (Total memory", GLES_IMAGE},
        {"C: gles buffer (Total memory", GLES_BUFFER},
        {"C: cl image (Total memory", CL_IMAGE},
        {"C: cl buffer (Total memory", CL_BUFFER},
    };
    AssignGpuMemoryByHookMap(line, gpuHookMap, detailGpuInfo);
}

void DetailInfoParser::JudgeChannelGpuType(const string &line, DetailGpuInfo &detailGpuInfo)
{
    const unordered_map<string, string> channelHookMap{
        {"Channel: Texture (Total memory", GLES_IMAGE},
        {"Channel: Buffer (Total memory", GLES_BUFFER},
    };
    AssignGpuMemoryByHookMap(line, channelHookMap, detailGpuInfo);
}

bool DetailInfoParser::CollectSmapsData(pid_t pid, const string &detectorTypeName, SmapsCollectResult &result)
{
    HIVIEW_LOGI("collect memory begin, pid:%{public}d, source:%{public}s", pid, detectorTypeName.c_str());
    shared_ptr<UCollectUtil::MemoryCollector> collector = UCollectUtil::MemoryCollector::Create();
    auto collectRet = collector->CollectProcessMemoryDetail(pid, GraphicMemOption::NONE);
    if (collectRet.retCode != UCollect::UcError::SUCCESS) {
        HIVIEW_LOGE("collect process memory error, ret:%{public}d", collectRet.retCode);
        return false;
    }
    HIVIEW_LOGI("collect memory end, pid:%{public}d, source:%{public}s", pid, detectorTypeName.c_str());
    result.memoryDetails = collectRet.data.details;
    result.totalRss = collectRet.data.totalRss;
    result.totalSwap = collectRet.data.totalSwap;
    result.totalPss = collectRet.data.totalPss;
    result.totalSwapPss = collectRet.data.totalSwapPss;
    return true;
}

void DetailInfoParser::ProcessMemoryItem(const MemoryItem &memoryItem, const string &memoryClassStr,
    MemoryClass memoryClass, SmapsAggContext &context)
{
    uint64_t value = context.statType == SmapsStatType::RSS_TYPE ? memoryItem.rss : memoryItem.pss;
    uint64_t swapValue = context.statType == SmapsStatType::RSS_TYPE ? memoryItem.swap : memoryItem.swapPss;
    uint64_t sum = value + swapValue;
    if (memoryItem.name.find("ashmem") != string::npos) {
        context.detailSmapsInfo.ashmem += static_cast<int>(sum);
        context.classTotalMem += sum;
        return;
    }
    if (memoryClass == MemoryClass::MEMORY_CLASS_OTHER) {
        if (memoryItem.iNode == 0) {
            context.detailSmapsInfo.anonPageOther += static_cast<int>(sum);
        } else {
            context.detailSmapsInfo.filePageOther += static_cast<int>(sum);
        }
        context.classTotalMem += sum;
        return;
    }
    auto it = SmapsTypeMap.find(memoryClassStr);
    if (it != SmapsTypeMap.end()) {
        context.detailSmapsInfo.*(it->second) += sum;
        context.classTotalMem += sum;
    }
}

void DetailInfoParser::AggregateMemoryItems(const vector<MemoryDetail> &memoryDetails, SmapsAggContext &context)
{
    for (const auto& memoryDetail : memoryDetails) {
        MemoryClass memoryClass = memoryDetail.memoryClass;
        if (static_cast<int>(memoryClass) < 0 ||
            static_cast<size_t>(memoryClass) >= MEMORY_CLASS_VEC.size()) {
            HIVIEW_LOGE("memoryClass:%{public}d is not exist", memoryClass);
            continue;
        }
        string memoryClassStr = MEMORY_CLASS_VEC[static_cast<int>(memoryClass)];
        for (const auto &memoryItem : memoryDetail.items) {
            ProcessMemoryItem(memoryItem, memoryClassStr, memoryClass, context);
        }
    }
}

DetailSmapsInfo DetailInfoParser::GetSmapsAndParse(const pid_t pid, const string &detectorTypeName,
    SmapsStatType statType)
{
    DetailSmapsInfo detailSmapsInfo;
    SmapsCollectResult collectResult;
    if (!CollectSmapsData(pid, detectorTypeName, collectResult)) {
        return detailSmapsInfo;
    }
    detailSmapsInfo.totalSize = (statType == SmapsStatType::RSS_TYPE) ?
        static_cast<int>(collectResult.totalRss + collectResult.totalSwap) :
        static_cast<int>(collectResult.totalPss + collectResult.totalSwapPss);
    uint64_t classTotalMem = 0;
    SmapsAggContext context{statType, detailSmapsInfo, classTotalMem};
    AggregateMemoryItems(collectResult.memoryDetails, context);
    detailSmapsInfo.others = detailSmapsInfo.totalSize - static_cast<int>(classTotalMem);
    return detailSmapsInfo;
}

DetailExtPssInfo DetailInfoParser::GetExtPssParseInfo(const pid_t pid, const string &detectorTypeName,
                                                      SmapsStatType statType)
{
    DetailExtPssInfo extPssInfo;
    HIVIEW_LOGI("parse exe info, pid:%{public}d, source:%{public}s", pid, detectorTypeName.c_str());
    extPssInfo.detailSmapsInfo = GetSmapsAndParse(pid, detectorTypeName, statType);
    extPssInfo.detailDmaBufInfo = GetIonProcessInfoAndParse(pid);
    extPssInfo.detailGpuInfo = GetGpuProcessInfoAndParse(pid);

    HIVIEW_LOGD("GetExtPssParseInfo result:"
                "Smaps: totalSize=%{public}d, nativeHeap=%{public}d, arktsHeap=%{public}d, hap=%{public}d, "
                "stack=%{public}d, db=%{public}d, so=%{public}d, dev=%{public}d, "
                "ttf=%{public}d, ashmem=%{public}d, arktsStaHeap=%{public}d, kotlinHeap=%{public}d, "
                "dartHeap=%{public}d, rnHermesHeap=%{public}d, jsvmHeap=%{public}d, arkwebV8=%{public}d, "
                "arkwebPa=%{public}d, anonPageOther=%{public}d, filePageOther=%{public}d, others=%{public}d"
                "DmaBuf: totalSize=%{public}d, pixelmapSize=%{public}d, xcomSize=%{public}d, webSize=%{public}d, "
                "hwVideoSize=%{public}d, swVideoSize=%{public}d, externalSize=%{public}d, othersSize=%{public}d"
                "Gpu: vulKanDefault=%{public}d, vulKanImage=%{public}d, vulKanBuffer=%{public}d, "
                "glesImage=%{public}d, glesBuffer=%{public}d, clImage=%{public}d, clBuffer=%{public}d",
                extPssInfo.detailSmapsInfo.totalSize, extPssInfo.detailSmapsInfo.nativeHeap,
                extPssInfo.detailSmapsInfo.arktsHeap, extPssInfo.detailSmapsInfo.hap,
                extPssInfo.detailSmapsInfo.stack, extPssInfo.detailSmapsInfo.db,
                extPssInfo.detailSmapsInfo.so, extPssInfo.detailSmapsInfo.dev,
                extPssInfo.detailSmapsInfo.ttf, extPssInfo.detailSmapsInfo.ashmem,
                extPssInfo.detailSmapsInfo.arktsStaHeap, extPssInfo.detailSmapsInfo.kotlinHeap,
                extPssInfo.detailSmapsInfo.dartHeap, extPssInfo.detailSmapsInfo.rnHermesHeap,
                extPssInfo.detailSmapsInfo.jsvmHeap, extPssInfo.detailSmapsInfo.arkwebV8,
                extPssInfo.detailSmapsInfo.arkwebPa, extPssInfo.detailSmapsInfo.anonPageOther,
                extPssInfo.detailSmapsInfo.filePageOther, extPssInfo.detailSmapsInfo.others,
                extPssInfo.detailDmaBufInfo.totalSize, extPssInfo.detailDmaBufInfo.pixelmapSize,
                extPssInfo.detailDmaBufInfo.xcomSize, extPssInfo.detailDmaBufInfo.webSize,
                extPssInfo.detailDmaBufInfo.hwVideoSize, extPssInfo.detailDmaBufInfo.swVideoSize,
                extPssInfo.detailDmaBufInfo.externalSize, extPssInfo.detailDmaBufInfo.othersSize,
                extPssInfo.detailGpuInfo.vulKanDefault, extPssInfo.detailGpuInfo.vulKanImage,
                extPssInfo.detailGpuInfo.vulKanBuffer, extPssInfo.detailGpuInfo.glesImage,
                extPssInfo.detailGpuInfo.glesBuffer, extPssInfo.detailGpuInfo.clImage,
                extPssInfo.detailGpuInfo.clBuffer);
    return extPssInfo;
}

}  // namespace HiviewDFX
}  // namespace OHOS