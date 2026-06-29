/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

import hiRetrieval from '@ohos.hiviewdfx.hiRetrieval'

import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'

describe('hiRetrievalJsUnitTest', function () {
  beforeAll(function() {

    /**
     * @tc.setup: setup invoked before all test cases
     */
    console.info('hiRetrievalJsUnitTest beforeAll called')
  })

  afterAll(function() {

    /**
     * @tc.teardown: teardown invoked after all test cases
     */
    console.info('hiRetrievalJsUnitTest afterAll called')
  })

  beforeEach(function() {

    /**
     * @tc.setup: setup invoked before each test case
     */
    console.info('hiRetrievalJsUnitTest beforeEach called')
    if (hiRetrieval.isParticipant()) {
      hiRetrieval.quit();
    }
  })

  afterEach(function() {

    /**
     * @tc.teardown: teardown invoked after each test case
     */
    console.info('hiRetrievalJsUnitTest afterEach called')
  })

  const SUCCESS = 0;

  const NOT_INIT_ERROR = 36000001;

  /**
   * @tc.desc: Test normal business for apis of hiRetrieval module
   * @tc.level: Level 0
   * @tc.name: hiRetrievalJsUnitTest001
   * @tc.number: hiRetrievalJsUnitTest001
   * @tc.type: Function
   * @tc.size: MediumTest
   */
  it('hiRetrievalJsUnitTest001', 0, async function (done) {
    try {
      let cfg = {
        userType: "testUserType",
        deviceType: "deviceType",
        deviceModel: "deviceModel"
      }
      hiRetrieval.participate(cfg);
    } catch (err) {
      expect(err.code == NOT_INIT_ERROR).assertTrue();
    }
    try {
      hiRetrieval.run();
    } catch (err) {
      expect(err.code == NOT_INIT_ERROR).assertTrue();
    }
    try {
      hiRetrieval.quit();
    } catch (err) {
      expect(err.code == NOT_INIT_ERROR).assertTrue();
    }
    expect(hiRetrieval.isParticipant() == false).assertTrue();
    expect(hiRetrieval.getLastParticipationTimestamp() >= 0).assertTrue();
    let curCfg = hiRetrieval.getCurrentConfig();
    expect(!curCfg.userType).assertTrue();
    expect(!curCfg.deviceType).assertTrue();
    expect(!curCfg.deviceModel).assertTrue();
    done();
  })

  /**
   * @tc.desc: Test abnormal business for apis of hiRetrieval module
   * @tc.level: Level 0
   * @tc.name: hiRetrievalJsUnitTest002
   * @tc.number: hiRetrievalJsUnitTest002
   * @tc.type: Function
   * @tc.size: MediumTest
   */
  it('hiRetrievalJsUnitTest002', 0, async function (done) {
    hiRetrieval.init();
    let cfg = {
      userType: "testUserType",
      deviceType: "deviceType",
      deviceModel: "deviceModel"
    }
    try {
      hiRetrieval.participate(cfg);
    } catch (err) {
      expect(false).assertTrue();
    }
    expect(hiRetrieval.isParticipant() == true).assertTrue();
    expect(hiRetrieval.getLastParticipationTimestamp() > 0).assertTrue();
    let curCfg = hiRetrieval.getCurrentConfig();
    expect(curCfg.userType == cfg.userType).assertTrue();
    expect(curCfg.deviceType == cfg.deviceType).assertTrue();
    expect(curCfg.deviceModel == cfg.deviceModel).assertTrue();
    try {
      hiRetrieval.run();
    } catch (err) {
      expect(false).assertTrue();
    }
    try {
      hiRetrieval.quit();
    } catch (err) {
      expect(false).assertTrue();
    }
    curCfg = hiRetrieval.getCurrentConfig();
    expect(!curCfg.userType).assertTrue();
    expect(!curCfg.deviceType).assertTrue();
    expect(!curCfg.deviceModel).assertTrue();
    expect(hiRetrieval.isParticipant() == false).assertTrue();
    expect(hiRetrieval.getLastParticipationTimestamp() > 0).assertTrue();
    try {
      hiRetrieval.quit();
    } catch (err) {
      expect(err.code == NOT_INIT_ERROR).assertTrue();
    }
    done();
  })
});