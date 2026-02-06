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

#ifndef ANIMATOR_MONITOR_H
#define ANIMATOR_MONITOR_H

#include <map>
#include <mutex>
#include "animator_monitor.h"
#include "perf_constants.h"
#include "perf_model.h"
#include "scene_monitor.h"
#include <set>

namespace OHOS {
namespace HiviewDFX {

class AnimatorMonitor : public IAnimatorCallback, public IFrameCallback {
public:
    static AnimatorMonitor& GetInstance();
    AnimatorMonitor();
    ~AnimatorMonitor();
    // outer interface for animator
    void RegisterAnimatorCallback(IAnimatorCallback* cb);
    void UnregisterAnimatorCallback(IAnimatorCallback* cb);
    void Start(const std::string& sceneId, PerfActionType type, const std::string& note);
    void End(const std::string& sceneId, bool isRsRender);
    void SetSubHealthInfo(const SubHealthInfo& info);
    bool IsSubHealthScene();

    // inner interface for animator
    void OnAnimatorStart(const std::string& sceneId, PerfActionType type, const std::string& note) override;
    void OnAnimatorStop(const std::string& sceneId, bool isRsRender) override;
    void OnVsyncEvent(int64_t vsyncTime, int64_t duration, double jank, const std::string& windowName) override;
    bool RecordsIsEmpty();

private:
    void FlushDataBase(AnimatorRecord* record, DataBase& data);
    void ReportAnimateStart(const std::string& sceneId, AnimatorRecord* record);
    void ReportAnimateEnd(const std::string& sceneId, AnimatorRecord* record);
    AnimatorRecord* GetRecord(const std::string& sceneId);
    void RemoveRecord(const std::string& sceneId);
    bool isValidSceneId(const std::string& sceneId);

    mutable std::mutex mMutex;
    int64_t subHealthRecordTime = 0;
    std::vector<IAnimatorCallback*> animatorCallbacks;
    std::map<std::string, AnimatorRecord*> mRecords;
    std::set<std::string> validSceneIds = {
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_ICON,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR_IN_LOCKSCREEN,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_RECENT,
        PerfConstants::START_APP_ANI_FORM,
        PerfConstants::INTO_HOME_ANI,
        PerfConstants::SCREENLOCK_SCREEN_OFF_ANIM,
        PerfConstants::PASSWORD_UNLOCK_ANI,
        PerfConstants::FACIAL_FLING_UNLOCK_ANI,
        PerfConstants::FACIAL_UNLOCK_ANI,
        PerfConstants::FINGERPRINT_UNLOCK_ANI,
        PerfConstants::META_BALLS_TURBO_CHARGING_ANIMATION,
        PerfConstants::ABILITY_OR_PAGE_SWITCH,
        PerfConstants::LAUNCHER_APP_SWIPE_TO_HOME,
        PerfConstants::APP_LIST_FLING,
        PerfConstants::APP_SWIPER_FLING,
        PerfConstants::APP_SWIPER_SCROLL,
        PerfConstants::APP_TAB_SWITCH,
        PerfConstants::VOLUME_BAR_SHOW,
        PerfConstants::PC_SPLIT_EXIT_ANIMATE_ON_DRAG,
        PerfConstants::PC_SPLIT_EXIT_ANIMATE_ON_RECOVER,
        PerfConstants::PC_SPLIT_EXIT_ANIMATE_ON_MINIMIZE,
        PerfConstants::PC_SPLIT_EXIT_ANIMATE_ON_MAXIMIZE,
        PerfConstants::PC_SPLIT_EXIT_ANIMATE_ON_CLOSE,
        PerfConstants::PC_SPLIT_EXIT_ANIMATE_ON_SPLIT,
        PerfConstants::PC_SPLIT_EXIT_ANIMATE_DEFAULT,
        PerfConstants::PC_APP_CENTER_GESTURE_OPERATION,
        PerfConstants::PC_GESTURE_TO_RECENT,
        PerfConstants::PC_SHORTCUT_SHOW_DESKTOP,
        PerfConstants::PC_SHORTCUT_RESTORE_DESKTOP,
        PerfConstants::PC_SHOW_DESKTOP_GESTURE_OPERATION,
        PerfConstants::PC_ALT_TAB_TO_RECENT,
        PerfConstants::PC_SHORTCUT_TO_RECENT,
        PerfConstants::PC_EXIT_RECENT,
        PerfConstants::PC_SHORTCUT_TO_APP_CENTER,
        PerfConstants::PC_SHORTCUT_TO_APP_CENTER_ON_RECENT,
        PerfConstants::PC_SHORTCUT_EXIT_APP_CENTER,
        PerfConstants::APP_TRANSITION_TO_OTHER_APP,
        PerfConstants::APP_TRANSITION_FROM_OTHER_APP,
        PerfConstants::APP_TRANSITION_FROM_OTHER_SERVICE,
        PerfConstants::SNAP_RECENT_ANI,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_DOCK,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_MISSON,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_MENU,
        PerfConstants::LAUNCHER_APP_BACK_TO_HOME,
        PerfConstants::EXIT_RECENT_2_HOME_ANI,
        PerfConstants::WINDOW_RECT_RESIZE,
        PerfConstants::WINDOW_RECT_MOVE,
        PerfConstants::SHOW_INPUT_METHOD_ANIMATION,
        PerfConstants::HIDE_INPUT_METHOD_ANIMATION,
        PerfConstants::SCREEN_ROTATION_ANI,
        PerfConstants::CLOSE_FOLDER_ANI,
        PerfConstants::LAUNCHER_SPRINGBACK_SCROLL,
        PerfConstants::WINDOW_TITLE_BAR_MINIMIZED,
        PerfConstants::APP_EXIT_FROM_WINDOW_TITLE_BAR_CLOSED,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_OTHER,
        PerfConstants::SCROLLER_ANIMATION,
        PerfConstants::WINDOW_TITLE_BAR_MAXIMIZED,
        PerfConstants::WINDOW_TITLE_BAR_RECOVER,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_TRANSITION,
        PerfConstants::ABILITY_OR_PAGE_SWITCH_INTERACTIVE,
        PerfConstants::SCREENLOCK_SCREEN_INTO_PIN,
        PerfConstants::CLEAR_1_RECENT_ANI,
        PerfConstants::CLEAR_All_RECENT_ANI,
        PerfConstants::RECENT_REALIGN_ANI,
        PerfConstants::INTO_CC_ANI,
        PerfConstants::EXIT_CC_ANI,
        PerfConstants::INTO_CC_FROM_NC,
        PerfConstants::INTO_CC_SUB_BLUETOOTH_ANI,
        PerfConstants::EXIT_CC_SUB_BLUETOOTH_ANI,
        PerfConstants::INTO_CC_SUB_WIFI_ANI,
        PerfConstants::EXIT_CC_SUB_WIFI_ANI,
        PerfConstants::INTO_CC_MEDIA_ANI,
        PerfConstants::EXIT_CC_MEDIA_ANI,
        PerfConstants::INTO_NC_ANI,
        PerfConstants::INTO_NC_FROM_CC,
        PerfConstants::CLEAR_NT_ANI,
        PerfConstants::SCROLL_NC_LIST_ANI,
        PerfConstants::EXIT_NC_ANI,
        PerfConstants::VOLUME_BAR_CHANGE_ON,
        PerfConstants::VOLUME_BAR_SLIDE,
        PerfConstants::VOLUME_BAR_EXPAND,
        PerfConstants::VOLUME_BAR_COLLAPSE,
        PerfConstants::VOLUME_BAR_TOUCHED,
        PerfConstants::FOLD_EXPAND_SPLIT_VIEW,
        PerfConstants::FOLD_TO_EXPAND_AA,
        PerfConstants::EXPAND_TO_FOLD_AA,
        PerfConstants::FOLD_TO_EXPAND_DOCK_BACKGROUND_SCALE_TWO,
        PerfConstants::EXPAND_TO_FOLD_INDICATOR,
        PerfConstants::FOLD_TO_EXPAND_WINDOWS,
        PerfConstants::EXPAND_TO_FOLD_WINDOWS,
        PerfConstants::LAUNCHER_BIGFOLDER_OPEN,
        PerfConstants::LAUNCHER_SMALLFOLDER_OPEN,
        PerfConstants::LAUNCHER_FOLDER_OPEN,
        PerfConstants::OPEN_ALBUM,
        PerfConstants::OPEN_BROWSER,
        PerfConstants::BROWSER_SWIPE,
        PerfConstants::CLOSE_BROWSER,
        PerfConstants::ANIMATE_TO_POSITION,
        PerfConstants::EXPAND_SCREEN_ROTATION_ANI,
        PerfConstants::CANTACTS_DIALER_BUTTON_PRESS,
        PerfConstants::CONTACTS_DIALER_HIDE,
        PerfConstants::CONTACTS_DIALER_SHOW,
        PerfConstants::SCREENSHOT_SCALE_ANIMATION,
        PerfConstants::SCREENSHOT_DISMISS_ANIMATION,
        PerfConstants::SCREENSHOT_DISMISS_ANIMATION_BY_USER,
        PerfConstants::SCREENRECORD_ANIMATION,
        PerfConstants::SCREENRECORD_DISMISS_ANIMATION,
        PerfConstants::SCREENRECORD_DISMISS_ANIMATION_BY_USER,
        PerfConstants::AOD_TO_LOCKSCREEN,
        PerfConstants::AOD_TO_LAUNCHER,
        PerfConstants::LOCKSCREEN_TO_LAUNCHER,
        PerfConstants::LOCKSCREEN_TO_AOD,
        PerfConstants::LAUNCHER_TO_AOD,
        PerfConstants::SCENE_CAP_TO_CARD_ANIM,
        PerfConstants::SCENE_CARD_TO_CAP_ANIM,
        PerfConstants::SCENE_LIST_TO_CAP_ANIM,
        PerfConstants::SCENE_CAP_TO_LIST_ANIM,
        PerfConstants::SCENE_LIST_SWIPE_ANIM,
        PerfConstants::SCREENLOCK_INTO_EDITOR_ANIM,
        PerfConstants::SCREENLOCK_EXIT_EDITOR_ANIM,
        PerfConstants::SCREEN_OFF_TO_SCREENLOCK_END,
        PerfConstants::SCROLL_2_AA,
        PerfConstants::INTO_AA_ANI,
        PerfConstants::EXIT_AA_ANI,
        PerfConstants::INTO_SEARCH_ANI,
        PerfConstants::EXIT_SEARCH_ANI,
        PerfConstants::FORMSTACK_SLIDE_BACK,
        PerfConstants::FORMSTACK_SLIDE_DOWN,
        PerfConstants::FORMSTACK_SLIDE_UP,
        PerfConstants::FORMSTACK_SWITCH_CARD,
        PerfConstants::FORM_MANAGER_CREATE_FORM,
        PerfConstants::INTO_LV_ANIM,
        PerfConstants::EXIT_LV_ANIM,
        PerfConstants::LV_INTO_APP_ANIM,
        PerfConstants::CAMERA_UE_GO_GALLERY,
        PerfConstants::EDITMODE_ENTER,
        PerfConstants::EDITMODE_EXIT,
        PerfConstants::LAUNCHER_OVER_SCROLL,
        PerfConstants::DRAG_ITEM_ANI,
        PerfConstants::WEB_LIST_FLING,
        PerfConstants::START_APP_ANI_MENU,
        PerfConstants::PC_CLICK_ARROW_RESTORE_DESKTOP,
        PerfConstants::PC_CLICK_ARROW_SHOW_DESKTOP,
        PerfConstants::PC_DOCK_EXIT_APP_CENTER,
        PerfConstants::PC_DOCK_INTO_APP_CENTER,
        PerfConstants::PC_INTO_APP_CENTER_ON_RECENT,
        PerfConstants::PC_INTO_RECENT,
        PerfConstants::PC_SPLIT_SCROLL_RECENT,
        PerfConstants::PC_SPLIT_START_ANIMATE,
        PerfConstants::SMARTDOCK_RECENTANIM_FIRSTOPEN,
        PerfConstants::PC_SHORTCUT_GLOBAL_SEARCH,
        PerfConstants::SWITCH_DESKTOP,
        PerfConstants::APP_ASSOCIATED_START,
        PerfConstants::CONTACTS_DIALER_BUTTON_PRESS,
        PerfConstants::LAUNCHER_CARD_TEMP_SHOW,
        PerfConstants::GESTURE_TO_RECENTS,
        PerfConstants::START_APP_ANI_AG,
        PerfConstants::CORE_METHOD_DESKTOP_SHOW,
        PerfConstants::APP_START,
        PerfConstants::PC_SPLIT_DRAG_DIVIDER_ANIMATE,
        PerfConstants::PC_STARTUP_TIME,
        PerfConstants::PC_WAKEUP_LATENCY,
        PerfConstants::APP_EXIT_FROM_RECENT,
        PerfConstants::APP_EXIT_FROM_RECENT,
        PerfConstants::COLLABORATION_ANIMATION,
        PerfConstants::CUSTOM_ANIMATOR_ROTATE90ACW,
        PerfConstants::EXIT_APP_CENTER,
        PerfConstants::PC_RESTORE_DESKTOP_GESTURE_OPERATION,
        PerfConstants::PC_SHOW_DESKTOP_GESTURE,
        PerfConstants::PC_TO_RECENT_GESTURE,
        PerfConstants::PC_ONE_FIN_SHOW_DESKTOP_GESTURE,
        PerfConstants::PC_ONE_FINGER_TO_RECENT_GESTURE,
        PerfConstants::WINDOW_DO_RESET_SCALE_ANIMATION,
        PerfConstants::WINDOW_DO_SCALE_ANIMATION,
        PerfConstants::AUTO_APP_SWIPER_FLING,
		//only for watch
        PerfConstants::WATCH_SCROLL_CARD_LIST_ANI,
        PerfConstants::WATCH_WATCHFACE_LONGPRESS_TO_LIST,
        PerfConstants::WATCH_WATCHFACELIST_CLICK_TO_EDIT,
        PerfConstants::WATCH_WATCHFACESELECT_TO_WATCHFACE,
        PerfConstants::WATCH_WATCHFACE_STYLE_SWIPE,
        PerfConstants::WATCH_POWER_DOUBLE_CLICK_TO_RECENTS,
        PerfConstants::WATCH_INTO_CARD_ANI,
        PerfConstants::WATCH_EXIT_CARD_ANI,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_APPCENTER,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_CONTROLCENTER,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_WATCHFACE,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_NEGATIVESCREEN,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_CARD,
        PerfConstants::LAUNCHER_APP_LAUNCH_FROM_WATCHFUNCKEY,
        PerfConstants::ENTER_ONE_STEP_SPLIT,
        PerfConstants::ENTER_SPLIT,
        PerfConstants::SPLIT_TO_FLOAT,
        PerfConstants::ADJUST_SPLIT_RATIO,
        PerfConstants::SPLIT_SWAP_ANIMATION,
        PerfConstants::SPLIT_BACK_SEC,
        PerfConstants::SPLIT_DRAG_EXIT,
        PerfConstants::SPLIT_STYLE_EXCHANGE,
        PerfConstants::ENTER_ONE_STEP_FLOAT,
        PerfConstants::FLOAT_START_FROM_DOCK,
        PerfConstants::FLOAT_START_FROM_SIDEBAR,
        PerfConstants::EXIT_FLOAT_BY_CLICK,
        PerfConstants::FLOAT_TO_FULL,
        PerfConstants::MINIMIZE_FLOAT_BY_CLICK,
        PerfConstants::DRAG_FLOAT,
        PerfConstants::MINI_FLOAT_TO_DEFAULT,
        PerfConstants::ENTER_ONE_STEP_MIDSCENE,
        PerfConstants::ENTER_MIDSCENE,
        PerfConstants::MIDSCENE_REPLACE,
        PerfConstants::MIDSCENE_TO_FULL,
        PerfConstants::MIDSCENE_TO_SPLIT,
        PerfConstants::MIDSCENE_FOCUS_TRANSFER,
        PerfConstants::MIDSCENE_ROTATION,
        PerfConstants::MIDSCENE_TO_RECENT,
        PerfConstants::SAVE_COMBINATION,
        PerfConstants::START_COMBINATION
    };
};

}
}

#endif // ANIMATOR_MONITOR_H