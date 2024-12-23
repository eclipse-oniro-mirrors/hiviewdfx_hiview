/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "io_calculator.h"

#include "file_util.h"
#include "hiview_logger.h"
#include "string_util.h"

namespace OHOS {
namespace HiviewDFX {
namespace UCollectUtil {
DEFINE_LOG_TAG("UCollectUtil-IoCalculator");
namespace {
constexpr int EMMC_SIZE_INDEX = 10;
}

int64_t IoCalculator::GetEMMCSize(const std::string &path)
{
    int64_t eMMCSize = -1;
    std::string sizeStr;
    std::string filePath = path + "/size";
    if (FileUtil::FileExists(filePath)) {
        sizeStr = FileUtil::GetFirstLine(filePath);
    } else {
        for (int index = 0; index < EMMC_SIZE_INDEX; ++index) {
            filePath = path + "/block/mmcblk" + std::to_string(index) + "/size";
            if (FileUtil::FileExists(filePath)) {
                sizeStr = FileUtil::GetFirstLine(filePath);
                break;
            }
        }
    }

    if (!sizeStr.empty()) {
        StringUtil::ConvertStringTo<int64_t>(sizeStr, eMMCSize);
    } else {
        HIVIEW_LOGE("load file=%{public}s failed.", filePath.c_str());
    }
    return eMMCSize;
}

std::string IoCalculator::GetEMMCManfid(const std::string &path)
{
    std::string filePath = path + "/manfid";
    std::string eMMCManfid = FileUtil::GetFirstLine(filePath);
    if (eMMCManfid.empty()) {
        HIVIEW_LOGE("load file=%{public}s failed.", filePath.c_str());
        return eMMCManfid;
    }

    int manfid = StringUtil::StrToInt(eMMCManfid);
    switch (manfid) {
        case 0X000003:
        case 0X000045:
        case 0X000002:
            eMMCManfid = "0x03";
            break;
        case 0X000011:
        case 0X000098:
        case 0X000198:
            eMMCManfid = "0x02";
            break;
        case 0X000090:
        case 0X0000AD:
        case 0X0001AD:
            eMMCManfid = "0x04";
            break;
        case 0X0000FE:
        case 0X00002C:
        case 0X000013:
            eMMCManfid = "0x05";
            break;
        case 0X000015:
        case 0X0000CE:
        case 0X0001CE:
            eMMCManfid = "0x01";
            break;
        default:
            break;
    }
    return eMMCManfid;
}

double IoCalculator::PercentValue(uint64_t preValue, uint64_t currValue, uint64_t period)
{
    if (period == 0) {
        return 0;
    }
    uint64_t subValue = (currValue > preValue) ? (currValue - preValue) : 0;
    return ((subValue * 1.0) / period);
}
} // UCollectUtil
} // HiViewDFX
} // OHOS
