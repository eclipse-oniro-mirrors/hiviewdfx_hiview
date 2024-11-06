/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef TBOX_H
#define TBOX_H

#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "common_defines.h"
#include "common_utils.h"
#include "file_util.h"

namespace OHOS {
namespace HiviewDFX {
class Tbox {
public:
    Tbox();
    ~Tbox();
    static std::string CalcFingerPrint(const std::string& val, size_t mask, int mode);
    static bool GetPartial(const std::string& src, const std::string& res, std::string& des);
    static bool IsCallStack(const std::string& line);
    static std::string GetStackName(const std::string& line);
    static bool HasCausedBy(const std::string& line);
    static void FilterTrace(std::map<std::string, std::string>& eventInfo, std::string eventType = "");
    static bool WaitForDoneFile(const std::string& file, unsigned int timeout);
    //get fault happen time
    static int64_t GetHappenTime(const std::string& src, const std::string& regex);

public:
    static const std::string ARRAY_STR;
    static const std::string CAUSEDBY_HEADER;
    static const std::string SUPPRESSED_HEADER;
};
}
}
#endif /* TBOX_H */
