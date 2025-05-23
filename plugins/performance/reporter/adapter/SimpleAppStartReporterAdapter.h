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
#ifndef SIMPLE_APP_START_REPORTER_ADAPTER_H
#define SIMPLE_APP_START_REPORTER_ADAPTER_H

#include "IAppStartReporter.h"
#include "IAppStartReportInfrastructure.h"

namespace OHOS {
namespace HiviewDFX {
class SimpleAppStartReporterAdapter : public IAppStartReporter {
public:
    explicit SimpleAppStartReporterAdapter(IAppStartReportInfrastructure* impl);
    void ReportNormal(const AppStartReportEvent& event) override;
    void ReportCritical(const AppStartReportEvent& event) override;

protected:
    IAppStartReportInfrastructure* reporter{nullptr};

    AppStartReportData ConvertReportEventToData(const AppStartReportEvent& event);
};
} // HiviewDFX
} // OHOS
#endif