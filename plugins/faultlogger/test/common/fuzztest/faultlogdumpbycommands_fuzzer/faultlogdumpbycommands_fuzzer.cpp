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

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

#include "faultlog_dump.h"
#include "faultlog_manager.h"
#include "faultlogdumpbycommands_fuzzer.h"
#include "fuzz_data_source.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace {
constexpr int32_t MAX_CMD_COUNT = 5;
constexpr size_t MAX_STR_LEN = 50;
}

void FuzzDumpByCommands(const uint8_t* data, size_t size)
{
    FuzzDataSource source(data, size);
    std::vector<std::string> cmds;
    for (int32_t i = 0; i < MAX_CMD_COUNT; ++i) {
        std::string cmd;
        if (!source.GetString(cmd, MAX_STR_LEN)) {
            break;
        }
        cmds.push_back(cmd);
    }

    auto faultLogManager = std::make_shared<FaultLogManager>();
    faultLogManager->Init();

    int fd = open("/dev/null", O_WRONLY);
    if (fd < 0) {
        return;
    }
    FaultLogDump dump(fd, faultLogManager);
    DumpRequest request;
    int32_t status = 0;
    (void)dump.ParseDumpCommands(cmds, request, status);
    close(fd);
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    OHOS::FuzzDumpByCommands(data, size);
    return 0;
}
