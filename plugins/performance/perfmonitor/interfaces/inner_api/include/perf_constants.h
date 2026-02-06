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

#ifndef XPERF_SCENE_ID_H
#define XPERF_SCENE_ID_H

#include <string>


namespace OHOS {
namespace HiviewDFX {

constexpr int32_t US_TO_MS = 1000;
constexpr int32_t NS_TO_MS = 1000000;
constexpr int32_t NS_TO_S = 1000000000;

constexpr uint32_t JANK_STATS_VERSION = 2;
constexpr char DEFAULT_SCENE_ID[] = "NONE_ANIMATION";

constexpr int64_t RESPONSE_TIMEOUT = 600000000;
constexpr int64_t ALL_RESPONSE_TIMEOUT = 10000000000LL;
constexpr int64_t STARTAPP_FRAME_TIMEOUT = 1000000000;
constexpr float SINGLE_FRAME_TIME = 16600000;
constexpr int64_t MIN_GC_INTERVAL = 1000000000;
constexpr int DEFAULT_THRESHOLD_JANK = 15;
constexpr int32_t JANK_SKIPPED_THRESHOLD = DEFAULT_THRESHOLD_JANK;
constexpr int32_t DEFAULT_JANK_REPORT_THRESHOLD = 3;
constexpr uint32_t DEFAULT_VSYNC = 16;
// Obtain the last three digits of the full path
constexpr uint32_t PATH_DEPTH = 3;

constexpr uint32_t JANK_FRAME_6_LIMIT = 0;
constexpr uint32_t JANK_FRAME_15_LIMIT = 1;
constexpr uint32_t JANK_FRAME_20_LIMIT = 2;
constexpr uint32_t JANK_FRAME_36_LIMIT = 3;
constexpr uint32_t JANK_FRAME_48_LIMIT = 4;
constexpr uint32_t JANK_FRAME_60_LIMIT = 5;
constexpr uint32_t JANK_FRAME_120_LIMIT = 6;
constexpr uint32_t JANK_FRAME_180_LIMIT = 7;
constexpr uint32_t JANK_STATS_SIZE = 8;

constexpr int32_t VAILD_JANK_SUB_HEALTH_INTERVAL = 100;

class PerfConstants {
public:
    // start app from launcher icon sceneid
    static constexpr char LAUNCHER_APP_LAUNCH_FROM_ICON[] = "LAUNCHER_APP_LAUNCH_FROM_ICON";

    // start app from notificationbar
    static constexpr char LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR[] = "LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR";

    // start app from lockscreen
    static constexpr char LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR_IN_LOCKSCREEN[] =
        "LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR_IN_LOCKSCREEN";

    // start app from recent
    static constexpr char LAUNCHER_APP_LAUNCH_FROM_RECENT[] = "LAUNCHER_APP_LAUNCH_FROM_RECENT";

    // start app from Card
    static constexpr char START_APP_ANI_FORM[] = "START_APP_ANI_FORM";

    // into home ani
    static constexpr char INTO_HOME_ANI[] = "INTO_HOME_ANI";

    // screenlock screen off ani
    static constexpr char SCREENLOCK_SCREEN_OFF_ANIM[] = "SCREENLOCK_SCREEN_OFF_ANIM";

    // password unlock ani
    static constexpr char PASSWORD_UNLOCK_ANI[] = "PASSWORD_UNLOCK_ANI";

    // facial fling unlock ani
    static constexpr char FACIAL_FLING_UNLOCK_ANI[] = "FACIAL_FLING_UNLOCK_ANI";

    // facial unlock ani
    static constexpr char FACIAL_UNLOCK_ANI[] = "FACIAL_UNLOCK_ANI";

    // fingerprint unlock ani
    static constexpr char FINGERPRINT_UNLOCK_ANI[] = "FINGERPRINT_UNLOCK_ANI";

    // charging dynamic ani
    static constexpr char META_BALLS_TURBO_CHARGING_ANIMATION[] = "META_BALLS_TURBO_CHARGING_ANIMATION";

    // app ablitity page switch
    static constexpr char ABILITY_OR_PAGE_SWITCH[] = "ABILITY_OR_PAGE_SWITCH";

    // app exit to home by geturing slide out
    static constexpr char LAUNCHER_APP_SWIPE_TO_HOME[] = "LAUNCHER_APP_SWIPE_TO_HOME";

    // app list fling
    static constexpr char APP_LIST_FLING[] = "APP_LIST_FLING";

    // app swiper fling
    static constexpr char APP_SWIPER_FLING[] = "APP_SWIPER_FLING";

    // app swiper scroll
    static constexpr char APP_SWIPER_SCROLL[] = "APP_SWIPER_SCROLL";

    // app tab switch
    static constexpr char APP_TAB_SWITCH[] = "APP_TAB_SWITCH";

    // volume bar show
    static constexpr char VOLUME_BAR_SHOW[] = "VOLUME_BAR_SHOW";

    // PC split exit animate on drag
    static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_DRAG[] = "PC_SPLIT_EXIT_ANIMATE_ON_DRAG";

    // PC split exit animate on recover
    static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_RECOVER[] = "PC_SPLIT_EXIT_ANIMATE_ON_RECOVER";

    // PC split exit animate on minimize
    static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_MINIMIZE[] = "PC_SPLIT_EXIT_ANIMATE_ON_MINIMIZE";

    // PC split exit animate on maximize
    static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_MAXIMIZE[] = "PC_SPLIT_EXIT_ANIMATE_ON_MAXIMIZE";

    // PC split exit animate on close
    static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_CLOSE[] = "PC_SPLIT_EXIT_ANIMATE_ON_CLOSE";

    // PC split exit animate on split
    static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_SPLIT[] = "PC_SPLIT_EXIT_ANIMATE_ON_SPLIT";

    // PC split exit animate default
    static constexpr char PC_SPLIT_EXIT_ANIMATE_DEFAULT[] = "PC_SPLIT_EXIT_ANIMATE_DEFAULT";

    // PC app center gesture operation
    static constexpr char PC_APP_CENTER_GESTURE_OPERATION[] = "PC_APP_CENTER_GESTURE_OPERATION";

    // PC gesture to recent
    static constexpr char PC_GESTURE_TO_RECENT[] = "PC_GESTURE_TO_RECENT";

    // PC shortcut show desktop
    static constexpr char PC_SHORTCUT_SHOW_DESKTOP[] = "PC_SHORTCUT_SHOW_DESKTOP";

    // PC shortcut restore desktop
    static constexpr char PC_SHORTCUT_RESTORE_DESKTOP[] = "PC_SHORTCUT_RESTORE_DESKTOP";

    // PC show desktop gesture operation
    static constexpr char PC_SHOW_DESKTOP_GESTURE_OPERATION[] = "PC_SHOW_DESKTOP_GESTURE_OPERATION";

    // PC alt + tab to recent
    static constexpr char PC_ALT_TAB_TO_RECENT[] = "PC_ALT_TAB_TO_RECENT";

    // PC shortcut to recent
    static constexpr char PC_SHORTCUT_TO_RECENT[] = "PC_SHORTCUT_TO_RECENT";

    // PC exit recent
    static constexpr char PC_EXIT_RECENT[] = "PC_EXIT_RECENT";

    // PC shoutcut to app center
    static constexpr char PC_SHORTCUT_TO_APP_CENTER[] = "PC_SHORTCUT_TO_APP_CENTER";

    // PC shoutcut to app center on recent
    static constexpr char PC_SHORTCUT_TO_APP_CENTER_ON_RECENT[] = "PC_SHORTCUT_TO_APP_CENTER_ON_RECENT";

    // PC shoutcut exit app center
    static constexpr char PC_SHORTCUT_EXIT_APP_CENTER[] = "PC_SHORTCUT_EXIT_APP_CENTER";

    // A app jump to another app
    static constexpr char APP_TRANSITION_TO_OTHER_APP[] = "APP_TRANSITION_TO_OTHER_APP";

    // another app jamps back to A app
    static constexpr char APP_TRANSITION_FROM_OTHER_APP[] = "APP_TRANSITION_FROM_OTHER_APP";

    // another service jamps to A app
    static constexpr char APP_TRANSITION_FROM_OTHER_SERVICE[] = "APP_TRANSITION_FROM_OTHER_SERVICE";

    // mutitask scroll
    static constexpr char SNAP_RECENT_ANI[] = "SNAP_RECENT_ANI";

    // start app from dock
    static constexpr char LAUNCHER_APP_LAUNCH_FROM_DOCK[] = "LAUNCHER_APP_LAUNCH_FROM_DOCK";

    // start app from misson
    static constexpr char LAUNCHER_APP_LAUNCH_FROM_MISSON[] = "LAUNCHER_APP_LAUNCH_FROM_MISSON";

    // app exit from back to home
    static constexpr char LAUNCHER_APP_BACK_TO_HOME[] = "LAUNCHER_APP_BACK_TO_HOME";

    // app exit from multitasking
    static constexpr char EXIT_RECENT_2_HOME_ANI[] = "EXIT_RECENT_2_HOME_ANI";

    // PC window resize
    static constexpr char WINDOW_RECT_RESIZE[] = "WINDOW_RECT_RESIZE";

    // PC window move
    static constexpr char WINDOW_RECT_MOVE[] = "WINDOW_RECT_MOVE";

    // input method show
    static constexpr char SHOW_INPUT_METHOD_ANIMATION[] = "SHOW_INPUT_METHOD_ANIMATION";

    // input method hide
    static constexpr char HIDE_INPUT_METHOD_ANIMATION[] = "HIDE_INPUT_METHOD_ANIMATION";

    // screen rotation
    static constexpr char SCREEN_ROTATION_ANI[] = "SCREEN_ROTATION_ANI";

    // folder close
    static constexpr char CLOSE_FOLDER_ANI[] = "CLOSE_FOLDER_ANI";

    // launcher spring back scroll
    static constexpr char LAUNCHER_SPRINGBACK_SCROLL[] = "LAUNCHER_SPRINGBACK_SCROLL";

    // window title bar minimized
    static constexpr char WINDOW_TITLE_BAR_MINIMIZED[] = "WINDOW_TITLE_BAR_MINIMIZED";

    // window title bar closed
    static constexpr char APP_EXIT_FROM_WINDOW_TITLE_BAR_CLOSED[] = "APP_EXIT_FROM_WINDOW_TITLE_BAR_CLOSED";

    // PC start app from other
    static constexpr char LAUNCHER_APP_LAUNCH_FROM_OTHER[] = "LAUNCHER_APP_LAUNCH_FROM_OTHER";

    // scroller animation
    static constexpr char SCROLLER_ANIMATION[] = "SCROLLER_ANIMATION";

    // pc title bar maximized
    static constexpr char WINDOW_TITLE_BAR_MAXIMIZED[] = "WINDOW_TITLE_BAR_MAXIMIZED";

    // pc title bar recover
    static constexpr char WINDOW_TITLE_BAR_RECOVER[] = "WINDOW_TITLE_BAR_RECOVER";

    // PC start app from transition
    static constexpr char LAUNCHER_APP_LAUNCH_FROM_TRANSITION[] = "LAUNCHER_APP_LAUNCH_FROM_TRANSITION";

    // navigation interactive animation
    static constexpr char ABILITY_OR_PAGE_SWITCH_INTERACTIVE[] = "ABILITY_OR_PAGE_SWITCH_INTERACTIVE";

    //screenlock into pin
    static constexpr char SCREENLOCK_SCREEN_INTO_PIN[] = "SCREENLOCK_SCREEN_INTO_PIN";

    static constexpr char CLEAR_1_RECENT_ANI[] = "CLEAR_1_RECENT_ANI";

    static constexpr char CLEAR_All_RECENT_ANI[] = "CLEAR_All_RECENT_ANI";

    static constexpr char RECENT_REALIGN_ANI[] = "RECENT_REALIGN_ANI";

    static constexpr char INTO_CC_ANI[] = "INTO_CC_ANI";

    static constexpr char EXIT_CC_ANI[] = "EXIT_CC_ANI";

    static constexpr char INTO_CC_FROM_NC[] = "INTO_CC_FROM_NC";

    static constexpr char INTO_CC_SUB_BLUETOOTH_ANI[] = "INTO_CC_SUB_BLUETOOTH_ANI";

    static constexpr char EXIT_CC_SUB_BLUETOOTH_ANI[] = "EXIT_CC_SUB_BLUETOOTH_ANI";

    static constexpr char INTO_CC_SUB_WIFI_ANI[] = "INTO_CC_SUB_WIFI_ANI";

    static constexpr char EXIT_CC_SUB_WIFI_ANI[] = "EXIT_CC_SUB_WIFI_ANI";

    static constexpr char INTO_CC_MEDIA_ANI[] = "INTO_CC_MEDIA_ANI";

    static constexpr char EXIT_CC_MEDIA_ANI[] = "EXIT_CC_MEDIA_ANI";

    static constexpr char INTO_NC_ANI[] = "INTO_NC_ANI";

    static constexpr char INTO_NC_FROM_CC[] = "INTO_NC_FROM_CC";

    static constexpr char CLEAR_NT_ANI[] = "CLEAR_NT_ANI";

    static constexpr char SCROLL_NC_LIST_ANI[] = "SCROLL_NC_LIST_ANI";

    static constexpr char EXIT_NC_ANI[] = "EXIT_NC_ANI";

    static constexpr char VOLUME_BAR_CHANGE_ON[] = "VOLUME_BAR_CHANGE_ON";

    static constexpr char VOLUME_BAR_SLIDE[] = "VOLUME_BAR_SLIDE";

    static constexpr char VOLUME_BAR_EXPAND[] = "VOLUME_BAR_EXPAND";

    static constexpr char VOLUME_BAR_COLLAPSE[] = "VOLUME_BAR_COLLAPSE";

    static constexpr char VOLUME_BAR_TOUCHED[] = "VOLUME_BAR_TOUCHED";

    static constexpr char FOLD_EXPAND_SPLIT_VIEW[] = "FOLD_EXPAND_SPLIT_VIEW";

    static constexpr char FOLD_TO_EXPAND_AA[] = "FOLD_TO_EXPAND_AA";

    static constexpr char EXPAND_TO_FOLD_AA[] = "EXPAND_TO_FOLD_AA";

    static constexpr char FOLD_TO_EXPAND_DOCK_BACKGROUND_SCALE_TWO[] = "FOLD_TO_EXPAND_DOCK_BACKGROUND_SCALE_TWO";

    static constexpr char EXPAND_TO_FOLD_INDICATOR[] = "EXPAND_TO_FOLD_INDICATOR";

    static constexpr char FOLD_TO_EXPAND_WINDOWS[] = "FOLD_TO_EXPAND_WINDOWS";

    static constexpr char EXPAND_TO_FOLD_WINDOWS[] = "EXPAND_TO_FOLD_WINDOWS";

    static constexpr char LAUNCHER_BIGFOLDER_OPEN[] = "LAUNCHER_BIGFOLDER_OPEN";

    static constexpr char LAUNCHER_SMALLFOLDER_OPEN[] = "LAUNCHER_SMALLFOLDER_OPEN";

    static constexpr char LAUNCHER_FOLDER_OPEN[] = "LAUNCHER_FOLDER_OPEN";

    static constexpr char OPEN_ALBUM[] = "OPEN_ALBUM";

    static constexpr char OPEN_BROWSER[] = "OPEN_BROWSER";

    static constexpr char BROWSER_SWIPE[] = "BROWSER_SWIPE";

    static constexpr char CLOSE_BROWSER[] = "CLOSE_BROWSER";

    static constexpr char ANIMATE_TO_POSITION[] = "ANIMATE_TO_POSITION";

    static constexpr char EXPAND_SCREEN_ROTATION_ANI[] = "EXPAND_SCREEN_ROTATION_ANI";

    static constexpr char CANTACTS_DIALER_BUTTON_PRESS[] = "CANTACTS_DIALER_BUTTON_PRESS";

    static constexpr char CONTACTS_DIALER_HIDE[] = "CONTACTS_DIALER_HIDE";

    static constexpr char CONTACTS_DIALER_SHOW[] = "CONTACTS_DIALER_SHOW";

    static constexpr char SCREENSHOT_SCALE_ANIMATION[] = "SCREENSHOT_SCALE_ANIMATION";

    static constexpr char SCREENSHOT_DISMISS_ANIMATION[] = "SCREENSHOT_DISMISS_ANIMATION";

    static constexpr char SCREENSHOT_DISMISS_ANIMATION_BY_USER[] = "SCREENSHOT_DISMISS_ANIMATION_BY_USER";

    static constexpr char SCREENRECORD_ANIMATION[] = "SCREENRECORD_ANIMATION";

    static constexpr char SCREENRECORD_DISMISS_ANIMATION[] = "SCREENRECORD_DISMISS_ANIMATION";

    static constexpr char SCREENRECORD_DISMISS_ANIMATION_BY_USER[] = "SCREENRECORD_DISMISS_ANIMATION_BY_USER";

    static constexpr char AOD_TO_LOCKSCREEN[] = "AOD_TO_LOCKSCREEN";

    static constexpr char AOD_TO_LAUNCHER[] = "AOD_TO_LAUNCHER";

    static constexpr char LOCKSCREEN_TO_LAUNCHER[] = "LOCKSCREEN_TO_LAUNCHER";

    static constexpr char LOCKSCREEN_TO_AOD[] = "LOCKSCREEN_TO_AOD";

    static constexpr char LAUNCHER_TO_AOD[] = "LAUNCHER_TO_AOD";

    static constexpr char SCENE_CAP_TO_CARD_ANIM[] = "SCENE_CAP_TO_CARD_ANIM";

    static constexpr char SCENE_CARD_TO_CAP_ANIM[] = "SCENE_CARD_TO_CAP_ANIM";

    static constexpr char SCENE_LIST_TO_CAP_ANIM[] = "SCENE_LIST_TO_CAP_ANIM";

    static constexpr char SCENE_CAP_TO_LIST_ANIM[] = "SCENE_CAP_TO_LIST_ANIM";

    static constexpr char SCENE_LIST_SWIPE_ANIM[] = "SCENE_LIST_SWIPE_ANIM";

    static constexpr char SCREENLOCK_INTO_EDITOR_ANIM[] = "SCREENLOCK_INTO_EDITOR_ANIM";

    static constexpr char SCREENLOCK_EXIT_EDITOR_ANIM[] = "SCREENLOCK_EXIT_EDITOR_ANIM";

    static constexpr char SCREEN_OFF_TO_SCREENLOCK_END[] = "SCREEN_OFF_TO_SCREENLOCK_END";

    static constexpr char SCROLL_2_AA[] = "SCROLL_2_AA";

    static constexpr char INTO_AA_ANI[] = "INTO_AA_ANI";

    static constexpr char EXIT_AA_ANI[] = "EXIT_AA_ANI";

    static constexpr char INTO_SEARCH_ANI[] = "INTO_SEARCH_ANI";

    static constexpr char EXIT_SEARCH_ANI[] = "EXIT_SEARCH_ANI";

    static constexpr char FORMSTACK_SLIDE_BACK[] = "FORMSTACK_SLIDE_BACK";

    static constexpr char FORMSTACK_SLIDE_DOWN[] = "FORMSTACK_SLIDE_DOWN";

    static constexpr char FORMSTACK_SLIDE_UP[] = "FORMSTACK_SLIDE_UP";

    static constexpr char FORMSTACK_SWITCH_CARD[] = "FORMSTACK_SWITCH_CARD";

    static constexpr char FORM_MANAGER_CREATE_FORM[] = "FORM_MANAGER_CREATE_FORM";

    static constexpr char INTO_LV_ANIM[] = "INTO_LV_ANIM";

    static constexpr char EXIT_LV_ANIM[] = "EXIT_LV_ANIM";

    static constexpr char LV_INTO_APP_ANIM[] = "LV_INTO_APP_ANIM";

    static constexpr char CAMERA_UE_GO_GALLERY[] = "CAMERA_UE_GO_GALLERY";

    static constexpr char EDITMODE_ENTER[] = "EDITMODE_ENTER";

    static constexpr char EDITMODE_EXIT[] = "EDITMODE_EXIT";

    static constexpr char LAUNCHER_OVER_SCROLL[] = "LAUNCHER_OVER_SCROLL";

    static constexpr char DRAG_ITEM_ANI[] = "DRAG_ITEM_ANI";
    
    static constexpr char WEB_LIST_FLING[] = "WEB_LIST_FLING";

    static constexpr char START_APP_ANI_MENU[] = "START_APP_ANI_MENU";

    static constexpr char PC_CLICK_ARROW_RESTORE_DESKTOP[] = "PC_CLICK_ARROW_RESTORE_DESKTOP";

    static constexpr char PC_CLICK_ARROW_SHOW_DESKTOP[] = "PC_CLICK_ARROW_SHOW_DESKTOP";
    
    static constexpr char PC_DOCK_EXIT_APP_CENTER[] = "PC_DOCK_EXIT_APP_CENTER";

    static constexpr char PC_DOCK_INTO_APP_CENTER[] = "PC_DOCK_INTO_APP_CENTER";

    static constexpr char PC_INTO_APP_CENTER_ON_RECENT[] = "PC_INTO_APP_CENTER_ON_RECENT";

    static constexpr char PC_INTO_RECENT[] = "PC_INTO_RECENT";

    static constexpr char PC_SPLIT_SCROLL_RECENT[] = "PC_SPLIT_SCROLL_RECENT";

    static constexpr char PC_SPLIT_START_ANIMATE[] = "PC_SPLIT_START_ANIMATE";

    static constexpr char SMARTDOCK_RECENTANIM_FIRSTOPEN[] = "SMARTDOCK_RECENTANIM_FIRSTOPEN";

    static constexpr char PC_SHORTCUT_GLOBAL_SEARCH[] = "PC_SHORTCUT_GLOBAL_SEARCH";

    static constexpr char SWITCH_DESKTOP[] = "SWITCH_DESKTOP";
    
    static constexpr char APP_ASSOCIATED_START[] = "APP_ASSOCIATED_START";

    static constexpr char CONTACTS_DIALER_BUTTON_PRESS[] = "CONTACTS_DIALER_BUTTON_PRESS";

    static constexpr char LAUNCHER_CARD_TEMP_SHOW[] = "LAUNCHER_CARD_TEMP_SHOW";

    static constexpr char GESTURE_TO_RECENTS[] = "GESTURE_TO_RECENTS";

    static constexpr char START_APP_ANI_AG[] = "START_APP_ANI_AG";

    static constexpr char CORE_METHOD_DESKTOP_SHOW[] = "CORE_METHOD_DESKTOP_SHOW";

    static constexpr char APP_START[] = "APP_START";

    static constexpr char PC_SPLIT_DRAG_DIVIDER_ANIMATE[] = "PC_SPLIT_DRAG_DIVIDER_ANIMATE";

    static constexpr char PC_STARTUP_TIME[] = "PC_STARTUP_TIME";

    static constexpr char PC_WAKEUP_LATENCY[] = "PC_WAKEUP_LATENCY";

    static constexpr char APP_EXIT_FROM_RECENT[] = "APP_EXIT_FROM_RECENT";

    static constexpr char COLLABORATION_ANIMATION[] = "COLLABORATION_ANIMATION";

    static constexpr char CUSTOM_ANIMATOR_ROTATE90ACW[] = "CUSTOM_ANIMATOR rotate90Acw";

    static constexpr char EXIT_APP_CENTER[] = "EXIT_APP_CENTER";

    static constexpr char PC_RESTORE_DESKTOP_GESTURE_OPERATION[] = "PC_RESTORE_DESKTOP_GESTURE_OPERATION";

    static constexpr char PC_SHOW_DESKTOP_GESTURE[] = "PC_SHOW_DESKTOP_GESTURE";

    static constexpr char PC_TO_RECENT_GESTURE[] = "PC_TO_RECENT_GESTURE";

    static constexpr char PC_ONE_FIN_SHOW_DESKTOP_GESTURE[] = "PC_ONE_FIN_SHOW_DESKTOP_GESTURE";

    static constexpr char PC_ONE_FINGER_TO_RECENT_GESTURE[] = "PC_ONE_FINGER_TO_RECENT_GESTURE";

    static constexpr char WINDOW_DO_RESET_SCALE_ANIMATION[] = "WINDOW_DO_RESET_SCALE_ANIMATION";

    static constexpr char WINDOW_DO_SCALE_ANIMATION[] = "WINDOW_DO_SCALE_ANIMATION";

    static constexpr char AUTO_APP_SWIPER_FLING[] = "AUTO_APP_SWIPER_FLING";

    // only for watch
    static constexpr char WATCH_SCROLL_CARD_LIST_ANI[] = "WATCH_SCROLL_CARD_LIST_ANI";

    static constexpr char WATCH_WATCHFACE_LONGPRESS_TO_LIST[] = "WATCH_WATCHFACE_LONGPRESS_TO_LIST";

    static constexpr char WATCH_WATCHFACELIST_CLICK_TO_EDIT[] = "WATCH_WATCHFACELIST_CLICK_TO_EDIT";

    static constexpr char WATCH_WATCHFACESELECT_TO_WATCHFACE[] = "WATCH_WATCHFACESELECT_TO_WATCHFACE";

    static constexpr char WATCH_WATCHFACE_STYLE_SWIPE[] = "WATCH_WATCHFACE_STYLE_SWIPE";

    static constexpr char WATCH_POWER_DOUBLE_CLICK_TO_RECENTS[] = "WATCH_POWER_DOUBLE_CLICK_TO_RECENTS";

    static constexpr char WATCH_INTO_CARD_ANI[] = "WATCH_INTO_CARD_ANI";

    static constexpr char WATCH_EXIT_CARD_ANI[] = "WATCH_EXIT_CARD_ANI";

    static constexpr char LAUNCHER_APP_LAUNCH_FROM_WATCHFUNCKEY[] = "LAUNCHER_APP_LAUNCH_FROM_WATCHFUNCKEY";

    static constexpr char LAUNCHER_APP_LAUNCH_FROM_CONTROLCENTER[] = "LAUNCHER_APP_LAUNCH_FROM_CONTROLCENTER";

    static constexpr char LAUNCHER_APP_LAUNCH_FROM_WATCHFACE[] = "LAUNCHER_APP_LAUNCH_FROM_WATCHFACE";

    static constexpr char LAUNCHER_APP_LAUNCH_FROM_NEGATIVESCREEN[] = "LAUNCHER_APP_LAUNCH_FROM_NEGATIVESCREEN";

    static constexpr char LAUNCHER_APP_LAUNCH_FROM_CARD[] = "LAUNCHER_APP_LAUNCH_FROM_CARD";

    static constexpr char LAUNCHER_APP_LAUNCH_FROM_APPCENTER[] = "LAUNCHER_APP_LAUNCH_FROM_APPCENTER";
    
    static constexpr char ENTER_ONE_STEP_SPLIT[] = "ENTER_ONE_STEP_SPLIT";

    static constexpr char ENTER_SPLIT[] = "ENTER_SPLIT";

    static constexpr char SPLIT_TO_FLOAT[] = "SPLIT_TO_FLOAT";

    static constexpr char ADJUST_SPLIT_RATIO[] = "ADJUST_SPLIT_RATIO";

    static constexpr char SPLIT_SWAP_ANIMATION[] = "SPLIT_SWAP_ANIMATION";

    static constexpr char SPLIT_BACK_SEC[] = "SPLIT_BACK_SEC";

    static constexpr char SPLIT_DRAG_EXIT[] = "SPLIT_DRAG_EXIT";
    
    static constexpr char ENTER_ONE_STEP_FLOAT[] = "ENTER_ONE_STEP_FLOAT";

    static constexpr char FLOAT_START_FROM_DOCK[] = "FLOAT_START_FROM_DOCK";

    static constexpr char FLOAT_START_FROM_SIDEBAR[] = "FLOAT_START_FROM_SIDEBAR";

    static constexpr char EXIT_FLOAT_BY_CLICK[] = "EXIT_FLOAT_BY_CLICK";

    static constexpr char FLOAT_TO_FULL[] = "FLOAT_TO_FULL";

    static constexpr char MINIMIZE_FLOAT_BY_CLICK[] = "MINIMIZE_FLOAT_BY_CLICK";

    static constexpr char DRAG_FLOAT[] = "DRAG_FLOAT";

    static constexpr char MINI_FLOAT_TO_DEFAULT[] = "MINI_FLOAT_TO_DEFAULT";

    static constexpr char SPLIT_STYLE_EXCHANGE[] = "SPLIT_STYLE_EXCHANGE";

    static constexpr char ENTER_ONE_STEP_MIDSCENE[] = "ENTER_ONE_STEP_MIDSCENE";

    static constexpr char ENTER_MIDSCENE[] = "ENTER_MIDSCENE";

    static constexpr char MIDSCENE_REPLACE[] = "MIDSCENE_REPLACE";

    static constexpr char MIDSCENE_TO_FULL[] = "MIDSCENE_TO_FULL";

    static constexpr char MIDSCENE_TO_SPLIT[] = "MIDSCENE_TO_SPLIT";

    static constexpr char MIDSCENE_FOCUS_TRANSFER[] = "MIDSCENE_FOCUS_TRANSFER";

    static constexpr char MIDSCENE_ROTATION[] = "MIDSCENE_ROTATION";

    static constexpr char MIDSCENE_TO_RECENT[] = "MIDSCENE_TO_RECENT";

    static constexpr char SAVE_COMBINATION[] = "SAVE_COMBINATION";

    static constexpr char START_COMBINATION[] = "START_COMBINATION";
};
} // namespace OHOS
} // namespace HiviewDFX
#endif // XPERF_SCENE_ID_H
