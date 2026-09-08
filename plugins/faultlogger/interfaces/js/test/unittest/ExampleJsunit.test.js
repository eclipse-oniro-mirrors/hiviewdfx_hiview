/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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
import faultlogger from '@ohos.faultLogger'
import hiSysEvent from '@ohos.hiSysEvent'
import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'
import faultloggerTestNapi from "libfaultlogger_test_napi.so"

describe("FaultlogJsTest", function () {
    const moduleName = "com.ohos.hiviewtest.faultlogjs";

    beforeAll(async function() {
        try {
            await addJsCrashFaultLog();
            await addCppCrashFaultLog();
            console.log("add FaultLog success");
        } catch (err) {
            console.info(err);
        }
        await msleep(7000);
    })

    async function addJsCrashFaultLog() {
        return hiSysEvent.write({
            domain: "ACE",
            name: "JS_ERROR",
            eventType: hiSysEvent.EventType.FAULT,
            params: {
                PACKAGE_NAME: moduleName,
                PROCESS_NAME: moduleName,
                MSG: "faultlogger testcase test.",
                REASON: "faultlogger testcase test."
            }
        })
    }

    async function addCppCrashFaultLog() {
        if (!faultloggerTestNapi.triggerCppCrash()) {
            throw "failed to add CPP_CRASH faultLog";
        }
    }

    function msleep(time) {
        return new Promise((resolve, reject) => {
            setTimeout(() => resolve("done!"), time)
        });
    }

    function testCallbackQuery(faultType) {
        return new Promise((resolve, reject) => {
            faultlogger.query(faultType, (error, ret) => {
                if (error) {
                    reject(error);
                } else {
                    resolve(ret);
                }
            })
        })
    }
    /**
     * test
     *
     * @tc.number: FaultlogJsException_001
     * @tc.name: FaultlogJsException_001
     * @tc.desc: API8 检验函数参数输入错误时程序是否会崩溃
     * @tc.require: AR000GICT2
     * @tc.author:
     * @tc.type: Function
     * @tc.size: MediumTest
     * @tc.level: Level 0
     */
    it('FaultlogJsException_001', 0, async function (done) {
        console.info("---------------------------FaultlogJsException_001----------------------------------");
        try {
            let ret1 = faultlogger.querySelfFaultLog("faultloggertestsummary01");
            console.info("FaultlogJsException_001 ret1 == " + ret1);
            let a = expect(ret1).assertEqual(undefined);
            console.info('ret1 assertEqual(undefined) ' + a);

            let ret2 = faultlogger.querySelfFaultLog(faultlogger.FaultType.JS_CRASH, "faultloggertestsummary01");
            console.info("FaultlogJsException_001 ret2 == " + ret2);
            expect(ret2).assertEqual(undefined);

            let ret3 = faultlogger.querySelfFaultLog();
            console.info("FaultlogJsException_001 ret3 == " + ret3);
            expect(ret3).assertEqual(undefined);
            done();
            return;
        } catch(err) {
            console.info(err);
        }
        expect(false).assertTrue();
        done();
    })

    /**
     * test
     *
     * @tc.number: FaultlogJsException_002
     * @tc.name: FaultlogJsException_002
     * @tc.desc: API9 检验函数参数输入错误时程序是否会崩溃并校验错误码
     * @tc.require: issueI5VRCC
     * @tc.author:
     * @tc.type: Function
     * @tc.size: MediumTest
     * @tc.level: Level 0
     */
     it('FaultlogJsException_002', 0, function () {
        console.info("---------------------------FaultlogJsException_002----------------------------------");
        try {
            let ret = faultlogger.query("faultloggertestsummary02");
            console.info("FaultlogJsException_002 ret == " + ret);
            return;
        } catch(err) {
            console.info(err.code);
            console.info(err.message);
            expect(err.code == 401).assertTrue();
        }
    })

    /**
     * test
     *
     * @tc.number: FaultlogJsException_003
     * @tc.name: FaultlogJsException_003
     * @tc.desc: API9 检验函数参数输入错误时程序是否会崩溃并校验错误码
     * @tc.require: issueI5VRCC
     * @tc.author:
     * @tc.type: Function
     * @tc.size: MediumTest
     * @tc.level: Level 0
     */
     it('FaultlogJsException_003', 0, function () {
        console.info("---------------------------FaultlogJsException_003----------------------------------");
        try {
            let ret = faultlogger.query(faultlogger.FaultType.JS_CRASH, "faultloggertestsummary03");
            console.info("FaultlogJsException_003 ret == " + ret);
            return;
        } catch(err) {
            console.info(err.code);
            console.info(err.message);
            expect(err.code == 401).assertTrue();
        }
    })

    /**
     * test
     *
     * @tc.number: FaultlogJsException_004
     * @tc.name: FaultlogJsException_004
     * @tc.desc: API9 检验函数参数输入错误时程序是否会崩溃并校验错误码
     * @tc.require: issueI5VRCC
     * @tc.author:
     * @tc.type: Function
     * @tc.size: MediumTest
     * @tc.level: Level 0
     */
     it('FaultlogJsException_004', 0, function () {
        console.info("---------------------------FaultlogJsException_004----------------------------------");
        try {
            let ret = faultlogger.query();
            console.info("FaultlogJsException_004 ret == " + ret);
            return;
        } catch(err) {
            console.info(err.code);
            console.info(err.message);
            expect(err.code == 401).assertTrue();
        }
    })

    /**
     * test
     *
     * @tc.number: FaultlogJsException_005
     * @tc.name: FaultlogJsException_005
     * @tc.desc: API9 检验函数参数输入错误时程序是否会崩溃并校验错误码
     * @tc.require: issueI5VRCC
     * @tc.author:
     * @tc.type: Function
     * @tc.size: MediumTest
     * @tc.level: Level 0
     */
    it('FaultlogJsException_005', 0, function () {
        console.info("---------------------------FaultlogJsException_005----------------------------------");
        try {
            let ret = faultlogger.query("aaa", "bbb", "ccc");
            console.info("FaultlogJsException_005 ret == " + ret);
            return;
        } catch(err) {
            console.info(err.code);
            console.info(err.message);
            expect(err.code == 401).assertTrue();
        }
    })

    async function checkCppCrashFaultLogInfo(faultLogInfo) {
        if (faultLogInfo.reason.indexOf("Signal:SIGABRT") === -1) {
            throw "Can not find \"Signal:SIGABRT\"";
        }
        if (faultLogInfo.fullLog.indexOf("Fault thread info") === -1) {
            throw "Can not find \"Fault thread info\"";
        }
    }

    /**
     * test
     *
     * @tc.number: FaultlogJsTest_005
     * @tc.name: FaultlogJsTest_005
     * @tc.desc: API9 检验查询CPP_CRASH类型的数据及内容
     * @tc.require: issueI5VRCC
     * @tc.author:
     * @tc.type: Function
     * @tc.size: MediumTest
     * @tc.level: Level 0
     */
     it('FaultlogJsTest_005', 0, async function (done) {
        console.info("---------------------------FaultlogJsTest_005----------------------------------");
        try {
            let retPromise = await faultlogger.query(faultlogger.FaultType.CPP_CRASH);
            console.info("FaultlogJsTest_005 query retPromise length:" + retPromise.length);
            expect(retPromise.length).assertLarger(0);
            await checkCppCrashFaultLogInfo(retPromise[0]);
            let retCallback = await testCallbackQuery(faultlogger.FaultType.CPP_CRASH);
            console.info("FaultlogJsTest_005 query retCallback length:" + retPromise.length);
            expect(retCallback.length).assertLarger(0);
            await checkCppCrashFaultLogInfo(retCallback[0]);
        } catch (err) {
            console.info(`FaultlogJsTest_005 error: ${err}`);
            expect(false).assertTrue();
        }
        done();
    })

    function checkNoSpecificFaultLogList(faultLogInfos) {
        expect(faultLogInfos[0].type).assertEqual(faultlogger.FaultType.CPP_CRASH);
        expect(faultLogInfos[1].type).assertEqual(faultlogger.FaultType.JS_CRASH);
        expect(faultLogInfos[1].timestamp).assertLess(faultLogInfos[0].timestamp);
    }

    /**
     * test
     *
     * @tc.number: FaultlogJsTest_007
     * @tc.name: FaultlogJsTest_007
     * @tc.desc: API9 校验查询NO_SPECIFIC类型数据，及查询数据的顺序
     * @tc.require: issueI5VRCC
     * @tc.author:
     * @tc.type: Function
     * @tc.size: MediumTest
     * @tc.level: Level 0
     */
    it('FaultlogJsTest_007', 0, async function (done) {
        console.info("---------------------------FaultlogJsTest_007----------------------------------");
        try {
            let retPromise = await faultlogger.query(faultlogger.FaultType.NO_SPECIFIC);
            console.info("FaultlogJsTest_007 query retPromise length:" + retPromise.length);
            expect(retPromise.length).assertLarger(1);
            checkNoSpecificFaultLogList(retPromise);

            let retCallback = await testCallbackQuery(faultlogger.FaultType.NO_SPECIFIC);
            console.info("FaultlogJsTest_007 query retCallback length:" + retCallback.length);
            expect(retCallback.length).assertLarger(1);
            checkNoSpecificFaultLogList(retCallback);
        } catch (err) {
            console.info(`FaultlogJsTest_007 error: ${err}`);
            expect(false).assertTrue();
        }
        done();
    })
})