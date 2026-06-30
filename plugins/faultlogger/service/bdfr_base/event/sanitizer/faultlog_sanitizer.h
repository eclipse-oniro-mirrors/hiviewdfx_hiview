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
#ifndef FAULTLOG_SANITIZER_H
#define FAULTLOG_SANITIZER_H

#include "faultlog_event_pipeline.h"
#include "sys_event.h"

namespace OHOS {
namespace HiviewDFX {
struct MapInfo {
    uint64_t start;
    uint64_t end;
    std::string fileName;
};

struct LoadInfo {
    uint64_t pc;
    uint64_t relativePc;
    uint64_t mapBase;
    std::string fullPath;
};
class FaultLogSanitizer final : public FaultLogEventPipeline {
public:
    FaultLogSanitizer();
private:
    bool ShouldParseSandBoxPath(const std::string& line);
    bool ConvertPathFromOriginLine(const std::string& line, std::string& pathPrefix, const std::string& bundleName);
    bool SafeStoull(const std::string& str, uint64_t& value, int base);
    bool ExtractLoadInfo(const std::string& line, const std::vector<MapInfo>& maps, const std::string& bundleName,
                         LoadInfo& info);
    std::string ProcessArkTsLine(const std::string& line, const std::string& packageName,
                                 const std::vector<MapInfo>& maps);
    std::vector<MapInfo> LoadMaps(std::ifstream& file);
    bool ParserArkTsStackInfo(const std::string& moduleName, const std::string& path);
    bool ForkProcessParseArkTsStackInfo(const std::string& moduleName, const std::string& path);

    FaultLogInfo FillFaultLogInfo(SysEvent& sysEvent) override;
    bool ReportToAppEvent(std::shared_ptr<SysEvent> sysEvent) const override;
    std::string GetFaultModule(SysEvent& sysEvent) const override;
    void UpdateFaultLogInfo() override;
    void UpdateSysEvent(SysEvent& sysEvent) override;
    static void ParseSanitizerEasyEvent(SysEvent& sysEvent);
    static void ReportSanitizerToAppEvent(std::shared_ptr<SysEvent> sysEvent);
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
