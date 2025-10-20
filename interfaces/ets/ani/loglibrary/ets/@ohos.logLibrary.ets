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

import { AsyncCallback, BusinessError } from '@ohos.base';

class LogEntryInner implements logLibrary.LogEntry {
    name: string = "";
    mtime: long = 0;
    size: long = 0;
}

interface Results {
    code: int;
    message: string;
}

class ResultsInner implements Results {
    code: int = 0;
    message: string = "";
}

export default namespace logLibrary {
    loadLibraryWithPermissionCheck("loglibrary_ani", "@ohos.logLibrary");
    export interface LogEntry {
        name: string;
        mtime: long;
        size: long;
    }

    export native function list(logType: string): LogEntry[];

    native function copySync(logType: string, logName: string, dest: string): Results;

    export function copy(logType: string, logName: string, dest: string): Promise<void> {
        return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
            let cb = (): Results => { return copySync(logType, logName, dest) }
            taskpool.execute(cb).then((ret: Any): void => {
                let retError = new BusinessError<void>();
                let copyResults: Results = ret as Results;
                if (copyResults.code === 0) {
                    resolve(undefined);
                } else {
                    retError.code = copyResults.code;
                    retError.message = copyResults.message as string;
                    reject(retError);
                }
            });
        })
    }

    export function copy(logType: string, logName: string, dest: string, callback: AsyncCallback<void>): void {
        let cb = (): Results => { return copySync(logType, logName, dest) }
        taskpool.execute(cb).then((ret: Any): void => {
            let retError = new BusinessError<void>();
            let copyResults: Results = ret as Results;
            retError.code = copyResults.code;
            retError.message = copyResults.message as string;
            callback(retError, undefined);
        });
    }

    native function moveSync(logType: string, logName: string, dest: string): Results;

    export function move(logType: string, logName: string, dest: string): Promise<void> {
        return new Promise<void>((resolve: (v: undefined) => void, reject: (e: BusinessError<void>) => void): void => {
            let cb = (): Results => { return moveSync(logType, logName, dest) }
            taskpool.execute(cb).then((ret: Any): void => {
                let retError = new BusinessError<void>();
                let moveResults: Results = ret as Results;
                if (moveResults.code === 0) {
                    resolve(undefined);
                } else {
                    retError.code = moveResults.code;
                    retError.message = moveResults.message as string;
                    reject(retError)
                }
            });
        })
    }

    export function move(logType: string, logName: string, dest: string, callback: AsyncCallback<void>): void {
        let cb = (): Results => { return moveSync(logType, logName, dest) }
        taskpool.execute(cb).then((ret: Any): void => {
            let retError = new BusinessError<void>();
            let moveResults: Results = ret as Results;
            retError.code = moveResults.code;
            retError.message = moveResults.message as string;
            callback(retError, undefined);
        });
    }

    export native function remove(logType: string, logName: string): void;
}
