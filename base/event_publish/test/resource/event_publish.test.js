/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
import hiAppEvent from "@ohos.hiviewdfx.hiAppEvent"
import {describe, beforeAll, beforeEach, afterEach, afterAll} from 'deccjsunit/index'

describe("EventPublishJsTest", function () {
    beforeAll(function() {
        console.info('EventPublishJsTest beforeAll called')
        hiAppEvent.addWatcher({
            name: "WatcherOsEvent",
            appEventFilters: [
                {
                    domain: hiAppEvent.domain.OS,
                    names: [
                        "APP_CRASH",
                        "APP_FREEZE",
                        "APP_LAUNCH",
                        "SCROLL_JANK",
                        "CPU_USAGE_HIGH",
                        "BATTERY_USAGE",
                        "RESOURCE_OVERLIMIT",
                        "ADDRESS_SANITIZER",
                        "MAIN_THREAD_JANK",
                        "APP_HICOLLIE",
                        "APP_KILLED"
                    ]
                }
            ]
        })
    })

    afterAll(function() {
        console.info('EventPublishJsTest afterAll called')
    })

    beforeEach(function() {
        console.info('EventPublishJsTest beforeEach called')
    })

    afterEach(function() {
        console.info('EventPublishJsTest afterEach called')
    })
})
