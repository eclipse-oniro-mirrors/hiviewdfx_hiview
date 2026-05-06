/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include "version_config_parser.h"

using namespace OHOS::HiviewDFX;

TEST(VersionConfigParserTest, ParsePreserveConfig_BoolValue) {
    Json::Value baseJsonInfo;
    baseJsonInfo["preserve"] = true;
    VersionConfigParser parser;
    EXPECT_EQ(parser.ParsePreserveConfig(baseJsonInfo), 1);
}

TEST(VersionConfigParserTest, ParseCollectConfig_BoolValue) {
    Json::Value baseJsonInfo;
    baseJsonInfo["collect"] = false;
    VersionConfigParser parser;
    EXPECT_EQ(parser.ParseCollectConfig(baseJsonInfo), 0);
}

TEST(VersionConfigParserTest, ParseVersionConfig_ObjectValue) {
    Json::Value versionConfig;
    versionConfig["beta"] = true;
    versionConfig["commercial"] = true;
    VersionConfigParser parser;
    EXPECT_EQ(parser.ParseVersionConfig(versionConfig), 1);
}
