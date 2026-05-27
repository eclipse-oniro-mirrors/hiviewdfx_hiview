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

inline constexpr int32_t US_TO_MS = 1000;
inline constexpr int32_t NS_TO_MS = 1000000;
inline constexpr int32_t NS_TO_S = 1000000000;

inline constexpr uint32_t JANK_STATS_VERSION = 2;
inline constexpr char DEFAULT_SCENE_ID[] = "NONE_ANIMATION";

inline constexpr int64_t RESPONSE_TIMEOUT = 600000000;
inline constexpr int64_t ALL_RESPONSE_TIMEOUT = 10000000000LL;
inline constexpr int64_t STARTAPP_FRAME_TIMEOUT = 1000000000;
inline constexpr float SINGLE_FRAME_TIME = 16600000;
inline constexpr int64_t MIN_GC_INTERVAL = 1000000000;
inline constexpr int DEFAULT_THRESHOLD_JANK = 15;
inline constexpr int32_t JANK_SKIPPED_THRESHOLD = DEFAULT_THRESHOLD_JANK;
inline constexpr int32_t DEFAULT_JANK_REPORT_THRESHOLD = 3;
inline constexpr uint32_t DEFAULT_VSYNC = 16;
// Obtain the last three digits of the full path
inline constexpr uint32_t PATH_DEPTH = 3;

inline constexpr size_t MAX_PAGE_NAME_LEN = 128;

inline constexpr uint32_t JANK_FRAME_6_LIMIT = 0;
inline constexpr uint32_t JANK_FRAME_15_LIMIT = 1;
inline constexpr uint32_t JANK_FRAME_20_LIMIT = 2;
inline constexpr uint32_t JANK_FRAME_36_LIMIT = 3;
inline constexpr uint32_t JANK_FRAME_48_LIMIT = 4;
inline constexpr uint32_t JANK_FRAME_60_LIMIT = 5;
inline constexpr uint32_t JANK_FRAME_120_LIMIT = 6;
inline constexpr uint32_t JANK_FRAME_180_LIMIT = 7;
inline constexpr uint32_t JANK_STATS_SIZE = 8;

inline constexpr int32_t VAILD_JANK_SUB_HEALTH_INTERVAL = 100;

class  {
public:
    // start app from launcher icon sceneid
    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_ICON[] = "LAUNCHER_APP_LAUNCH_FROM_ICON";

    inline constexpr std::string_view LAUNCHER_APP_LAUNCH_FROM_ICON = "LAUNCHER_APP_LAUNCH_FROM_ICON";


    // start app from notificationbar
    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR[] = "LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR";

    // start app from lockscreen
    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR_IN_LOCKSCREEN[] =
        "LAUNCHER_APP_LAUNCH_FROM_NOTIFICATIONBAR_IN_LOCKSCREEN";

    // start app from recent
    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_RECENT[] = "LAUNCHER_APP_LAUNCH_FROM_RECENT";

    // start app from Card
    inline static constexpr char START_APP_ANI_FORM[] = "START_APP_ANI_FORM";

    // into home ani
    inline static constexpr char INTO_HOME_ANI[] = "INTO_HOME_ANI";

    // screenlock screen off ani
    inline static constexpr char SCREENLOCK_SCREEN_OFF_ANIM[] = "SCREENLOCK_SCREEN_OFF_ANIM";

    // password unlock ani
    inline static constexpr char PASSWORD_UNLOCK_ANI[] = "PASSWORD_UNLOCK_ANI";

    // facial fling unlock ani
    inline static constexpr char FACIAL_FLING_UNLOCK_ANI[] = "FACIAL_FLING_UNLOCK_ANI";

    // facial unlock ani
    inline static constexpr char FACIAL_UNLOCK_ANI[] = "FACIAL_UNLOCK_ANI";

    // fingerprint unlock ani
    inline static constexpr char FINGERPRINT_UNLOCK_ANI[] = "FINGERPRINT_UNLOCK_ANI";

    // charging dynamic ani
    inline static constexpr char META_BALLS_TURBO_CHARGING_ANIMATION[] = "META_BALLS_TURBO_CHARGING_ANIMATION";

    // app ablitity page switch
    inline static constexpr char ABILITY_OR_PAGE_SWITCH[] = "ABILITY_OR_PAGE_SWITCH";

    // app exit to home by geturing slide out
    inline static constexpr char LAUNCHER_APP_SWIPE_TO_HOME[] = "LAUNCHER_APP_SWIPE_TO_HOME";

    // app list fling
    inline static constexpr char APP_LIST_FLING[] = "APP_LIST_FLING";

    // app swiper fling
    inline static constexpr char APP_SWIPER_FLING[] = "APP_SWIPER_FLING";

    // app swiper scroll
    inline static constexpr char APP_SWIPER_SCROLL[] = "APP_SWIPER_SCROLL";

    // app tab switch
    inline static constexpr char APP_TAB_SWITCH[] = "APP_TAB_SWITCH";

    // volume bar show
    inline static constexpr char VOLUME_BAR_SHOW[] = "VOLUME_BAR_SHOW";

    // PC split exit animate on drag
    inline static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_DRAG[] = "PC_SPLIT_EXIT_ANIMATE_ON_DRAG";

    // PC split exit animate on recover
    inline static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_RECOVER[] = "PC_SPLIT_EXIT_ANIMATE_ON_RECOVER";

    // PC split exit animate on minimize
    inline static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_MINIMIZE[] = "PC_SPLIT_EXIT_ANIMATE_ON_MINIMIZE";

    // PC split exit animate on maximize
    inline static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_MAXIMIZE[] = "PC_SPLIT_EXIT_ANIMATE_ON_MAXIMIZE";

    // PC split exit animate on close
    inline static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_CLOSE[] = "PC_SPLIT_EXIT_ANIMATE_ON_CLOSE";

    // PC split exit animate on split
    inline static constexpr char PC_SPLIT_EXIT_ANIMATE_ON_SPLIT[] = "PC_SPLIT_EXIT_ANIMATE_ON_SPLIT";

    // PC split exit animate default
    inline static constexpr char PC_SPLIT_EXIT_ANIMATE_DEFAULT[] = "PC_SPLIT_EXIT_ANIMATE_DEFAULT";

    // PC app center gesture operation
    inline static constexpr char PC_APP_CENTER_GESTURE_OPERATION[] = "PC_APP_CENTER_GESTURE_OPERATION";

    // PC gesture to recent
    inline static constexpr char PC_GESTURE_TO_RECENT[] = "PC_GESTURE_TO_RECENT";

    // PC shortcut show desktop
    inline static constexpr char PC_SHORTCUT_SHOW_DESKTOP[] = "PC_SHORTCUT_SHOW_DESKTOP";

    // PC shortcut restore desktop
    inline static constexpr char PC_SHORTCUT_RESTORE_DESKTOP[] = "PC_SHORTCUT_RESTORE_DESKTOP";

    // PC show desktop gesture operation
    inline static constexpr char PC_SHOW_DESKTOP_GESTURE_OPERATION[] = "PC_SHOW_DESKTOP_GESTURE_OPERATION";

    // PC alt + tab to recent
    inline static constexpr char PC_ALT_TAB_TO_RECENT[] = "PC_ALT_TAB_TO_RECENT";

    // PC shortcut to recent
    inline static constexpr char PC_SHORTCUT_TO_RECENT[] = "PC_SHORTCUT_TO_RECENT";

    // PC exit recent
    inline static constexpr char PC_EXIT_RECENT[] = "PC_EXIT_RECENT";

    // PC shoutcut to app center
    inline static constexpr char PC_SHORTCUT_TO_APP_CENTER[] = "PC_SHORTCUT_TO_APP_CENTER";

    // PC shoutcut to app center on recent
    inline static constexpr char PC_SHORTCUT_TO_APP_CENTER_ON_RECENT[] = "PC_SHORTCUT_TO_APP_CENTER_ON_RECENT";

    // PC shoutcut exit app center
    inline static constexpr char PC_SHORTCUT_EXIT_APP_CENTER[] = "PC_SHORTCUT_EXIT_APP_CENTER";

    // A app jump to another app
    inline static constexpr char APP_TRANSITION_TO_OTHER_APP[] = "APP_TRANSITION_TO_OTHER_APP";

    // another app jamps back to A app
    inline static constexpr char APP_TRANSITION_FROM_OTHER_APP[] = "APP_TRANSITION_FROM_OTHER_APP";

    // another service jamps to A app
    inline static constexpr char APP_TRANSITION_FROM_OTHER_SERVICE[] = "APP_TRANSITION_FROM_OTHER_SERVICE";

    // mutitask scroll
    inline static constexpr char SNAP_RECENT_ANI[] = "SNAP_RECENT_ANI";

    // start app from dock
    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_DOCK[] = "LAUNCHER_APP_LAUNCH_FROM_DOCK";

    // start app from misson
    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_MISSON[] = "LAUNCHER_APP_LAUNCH_FROM_MISSON";

    // start app from menu
    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_MENU[] = "LAUNCHER_APP_LAUNCH_FROM_MENU";

    // app exit from back to home
    inline static constexpr char LAUNCHER_APP_BACK_TO_HOME[] = "LAUNCHER_APP_BACK_TO_HOME";

    // app exit from multitasking
    inline static constexpr char EXIT_RECENT_2_HOME_ANI[] = "EXIT_RECENT_2_HOME_ANI";

    // PC window resize
    inline static constexpr char WINDOW_RECT_RESIZE[] = "WINDOW_RECT_RESIZE";

    // PC window move
    inline static constexpr char WINDOW_RECT_MOVE[] = "WINDOW_RECT_MOVE";

    // input method show
    inline static constexpr char SHOW_INPUT_METHOD_ANIMATION[] = "SHOW_INPUT_METHOD_ANIMATION";

    // input method hide
    inline static constexpr char HIDE_INPUT_METHOD_ANIMATION[] = "HIDE_INPUT_METHOD_ANIMATION";

    // screen rotation
    inline static constexpr char SCREEN_ROTATION_ANI[] = "SCREEN_ROTATION_ANI";

    // folder close
    inline static constexpr char CLOSE_FOLDER_ANI[] = "CLOSE_FOLDER_ANI";

    // launcher spring back scroll
    inline static constexpr char LAUNCHER_SPRINGBACK_SCROLL[] = "LAUNCHER_SPRINGBACK_SCROLL";

    // window title bar minimized
    inline static constexpr char WINDOW_TITLE_BAR_MINIMIZED[] = "WINDOW_TITLE_BAR_MINIMIZED";

    // window title bar closed
    inline static constexpr char APP_EXIT_FROM_WINDOW_TITLE_BAR_CLOSED[] = "APP_EXIT_FROM_WINDOW_TITLE_BAR_CLOSED";

    // PC start app from other
    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_OTHER[] = "LAUNCHER_APP_LAUNCH_FROM_OTHER";

    // scroller animation
    inline static constexpr char SCROLLER_ANIMATION[] = "SCROLLER_ANIMATION";

    // pc title bar maximized
    inline static constexpr char WINDOW_TITLE_BAR_MAXIMIZED[] = "WINDOW_TITLE_BAR_MAXIMIZED";

    // pc title bar recover
    inline static constexpr char WINDOW_TITLE_BAR_RECOVER[] = "WINDOW_TITLE_BAR_RECOVER";

    // PC start app from transition
    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_TRANSITION[] = "LAUNCHER_APP_LAUNCH_FROM_TRANSITION";

    // navigation interactive animation
    inline static constexpr char ABILITY_OR_PAGE_SWITCH_INTERACTIVE[] = "ABILITY_OR_PAGE_SWITCH_INTERACTIVE";

    //screenlock into pin
    inline static constexpr char SCREENLOCK_SCREEN_INTO_PIN[] = "SCREENLOCK_SCREEN_INTO_PIN";

    inline static constexpr char CLEAR_1_RECENT_ANI[] = "CLEAR_1_RECENT_ANI";

    inline static constexpr char CLEAR_All_RECENT_ANI[] = "CLEAR_All_RECENT_ANI";

    inline static constexpr char RECENT_REALIGN_ANI[] = "RECENT_REALIGN_ANI";

    inline static constexpr char INTO_CC_ANI[] = "INTO_CC_ANI";

    inline static constexpr char EXIT_CC_ANI[] = "EXIT_CC_ANI";

    inline static constexpr char INTO_CC_FROM_NC[] = "INTO_CC_FROM_NC";

    inline static constexpr char INTO_CC_SUB_BLUETOOTH_ANI[] = "INTO_CC_SUB_BLUETOOTH_ANI";

    inline static constexpr char EXIT_CC_SUB_BLUETOOTH_ANI[] = "EXIT_CC_SUB_BLUETOOTH_ANI";

    inline static constexpr char INTO_CC_SUB_WIFI_ANI[] = "INTO_CC_SUB_WIFI_ANI";

    inline static constexpr char EXIT_CC_SUB_WIFI_ANI[] = "EXIT_CC_SUB_WIFI_ANI";

    inline static constexpr char INTO_CC_MEDIA_ANI[] = "INTO_CC_MEDIA_ANI";

    inline static constexpr char EXIT_CC_MEDIA_ANI[] = "EXIT_CC_MEDIA_ANI";

    inline static constexpr char INTO_NC_ANI[] = "INTO_NC_ANI";

    inline static constexpr char INTO_NC_FROM_CC[] = "INTO_NC_FROM_CC";

    inline static constexpr char CLEAR_NT_ANI[] = "CLEAR_NT_ANI";

    inline static constexpr char SCROLL_NC_LIST_ANI[] = "SCROLL_NC_LIST_ANI";

    inline static constexpr char EXIT_NC_ANI[] = "EXIT_NC_ANI";

    inline static constexpr char VOLUME_BAR_CHANGE_ON[] = "VOLUME_BAR_CHANGE_ON";

    inline static constexpr char VOLUME_BAR_SLIDE[] = "VOLUME_BAR_SLIDE";

    inline static constexpr char VOLUME_BAR_EXPAND[] = "VOLUME_BAR_EXPAND";

    inline static constexpr char VOLUME_BAR_COLLAPSE[] = "VOLUME_BAR_COLLAPSE";

    inline static constexpr char VOLUME_BAR_TOUCHED[] = "VOLUME_BAR_TOUCHED";

    inline static constexpr char FOLD_EXPAND_SPLIT_VIEW[] = "FOLD_EXPAND_SPLIT_VIEW";

    inline static constexpr char FOLD_TO_EXPAND_AA[] = "FOLD_TO_EXPAND_AA";

    inline static constexpr char EXPAND_TO_FOLD_AA[] = "EXPAND_TO_FOLD_AA";

    inline static constexpr char FOLD_TO_EXPAND_DOCK_BACKGROUND_SCALE_TWO[] = "FOLD_TO_EXPAND_DOCK_BACKGROUND_SCALE_TWO";

    inline static constexpr char EXPAND_TO_FOLD_INDICATOR[] = "EXPAND_TO_FOLD_INDICATOR";

    inline static constexpr char FOLD_TO_EXPAND_WINDOWS[] = "FOLD_TO_EXPAND_WINDOWS";

    inline static constexpr char EXPAND_TO_FOLD_WINDOWS[] = "EXPAND_TO_FOLD_WINDOWS";

    inline static constexpr char LAUNCHER_BIGFOLDER_OPEN[] = "LAUNCHER_BIGFOLDER_OPEN";

    inline static constexpr char LAUNCHER_SMALLFOLDER_OPEN[] = "LAUNCHER_SMALLFOLDER_OPEN";

    inline static constexpr char LAUNCHER_FOLDER_OPEN[] = "LAUNCHER_FOLDER_OPEN";

    inline static constexpr char OPEN_ALBUM[] = "OPEN_ALBUM";

    inline static constexpr char OPEN_BROWSER[] = "OPEN_BROWSER";

    inline static constexpr char BROWSER_SWIPE[] = "BROWSER_SWIPE";

    inline static constexpr char CLOSE_BROWSER[] = "CLOSE_BROWSER";

    inline static constexpr char ANIMATE_TO_POSITION[] = "ANIMATE_TO_POSITION";

    inline static constexpr char EXPAND_SCREEN_ROTATION_ANI[] = "EXPAND_SCREEN_ROTATION_ANI";

    inline static constexpr char CANTACTS_DIALER_BUTTON_PRESS[] = "CANTACTS_DIALER_BUTTON_PRESS";

    inline static constexpr char CONTACTS_DIALER_HIDE[] = "CONTACTS_DIALER_HIDE";

    inline static constexpr char CONTACTS_DIALER_SHOW[] = "CONTACTS_DIALER_SHOW";

    inline static constexpr char SCREENSHOT_SCALE_ANIMATION[] = "SCREENSHOT_SCALE_ANIMATION";

    inline static constexpr char SCREENSHOT_DISMISS_ANIMATION[] = "SCREENSHOT_DISMISS_ANIMATION";

    inline static constexpr char SCREENSHOT_DISMISS_ANIMATION_BY_USER[] = "SCREENSHOT_DISMISS_ANIMATION_BY_USER";

    inline static constexpr char SCREENRECORD_ANIMATION[] = "SCREENRECORD_ANIMATION";

    inline static constexpr char SCREENRECORD_DISMISS_ANIMATION[] = "SCREENRECORD_DISMISS_ANIMATION";

    inline static constexpr char SCREENRECORD_DISMISS_ANIMATION_BY_USER[] = "SCREENRECORD_DISMISS_ANIMATION_BY_USER";

    inline static constexpr char AOD_TO_LOCKSCREEN[] = "AOD_TO_LOCKSCREEN";

    inline static constexpr char AOD_TO_LAUNCHER[] = "AOD_TO_LAUNCHER";

    inline static constexpr char LOCKSCREEN_TO_LAUNCHER[] = "LOCKSCREEN_TO_LAUNCHER";

    inline static constexpr char LOCKSCREEN_TO_AOD[] = "LOCKSCREEN_TO_AOD";

    inline static constexpr char LAUNCHER_TO_AOD[] = "LAUNCHER_TO_AOD";

    inline static constexpr char SCENE_CAP_TO_CARD_ANIM[] = "SCENE_CAP_TO_CARD_ANIM";

    inline static constexpr char SCENE_CARD_TO_CAP_ANIM[] = "SCENE_CARD_TO_CAP_ANIM";

    inline static constexpr char SCENE_LIST_TO_CAP_ANIM[] = "SCENE_LIST_TO_CAP_ANIM";

    inline static constexpr char SCENE_CAP_TO_LIST_ANIM[] = "SCENE_CAP_TO_LIST_ANIM";

    inline static constexpr char SCENE_LIST_SWIPE_ANIM[] = "SCENE_LIST_SWIPE_ANIM";

    inline static constexpr char SCREENLOCK_INTO_EDITOR_ANIM[] = "SCREENLOCK_INTO_EDITOR_ANIM";

    inline static constexpr char SCREENLOCK_EXIT_EDITOR_ANIM[] = "SCREENLOCK_EXIT_EDITOR_ANIM";

    inline static constexpr char SCREEN_OFF_TO_SCREENLOCK_END[] = "SCREEN_OFF_TO_SCREENLOCK_END";

    inline static constexpr char SCROLL_2_AA[] = "SCROLL_2_AA";

    inline static constexpr char INTO_AA_ANI[] = "INTO_AA_ANI";

    inline static constexpr char EXIT_AA_ANI[] = "EXIT_AA_ANI";

    inline static constexpr char INTO_SEARCH_ANI[] = "INTO_SEARCH_ANI";

    inline static constexpr char EXIT_SEARCH_ANI[] = "EXIT_SEARCH_ANI";

    inline static constexpr char FORMSTACK_SLIDE_BACK[] = "FORMSTACK_SLIDE_BACK";

    inline static constexpr char FORMSTACK_SLIDE_DOWN[] = "FORMSTACK_SLIDE_DOWN";

    inline static constexpr char FORMSTACK_SLIDE_UP[] = "FORMSTACK_SLIDE_UP";

    inline static constexpr char FORMSTACK_SWITCH_CARD[] = "FORMSTACK_SWITCH_CARD";

    inline static constexpr char INTO_LV_ANIM[] = "INTO_LV_ANIM";

    inline static constexpr char EXIT_LV_ANIM[] = "EXIT_LV_ANIM";

    inline static constexpr char LV_INTO_APP_ANIM[] = "LV_INTO_APP_ANIM";

    inline static constexpr char CAMERA_UE_GO_GALLERY[] = "CAMERA_UE_GO_GALLERY";

    inline static constexpr char EDITMODE_ENTER[] = "EDITMODE_ENTER";

    inline static constexpr char EDITMODE_EXIT[] = "EDITMODE_EXIT";

    inline static constexpr char LAUNCHER_OVER_SCROLL[] = "LAUNCHER_OVER_SCROLL";

    inline static constexpr char DRAG_ITEM_ANI[] = "DRAG_ITEM_ANI";
    
    inline static constexpr char WEB_LIST_FLING[] = "WEB_LIST_FLING";

    inline static constexpr char START_APP_ANI_MENU[] = "START_APP_ANI_MENU";

    inline static constexpr char PC_CLICK_ARROW_RESTORE_DESKTOP[] = "PC_CLICK_ARROW_RESTORE_DESKTOP";

    inline static constexpr char PC_CLICK_ARROW_SHOW_DESKTOP[] = "PC_CLICK_ARROW_SHOW_DESKTOP";
    
    inline static constexpr char PC_DOCK_EXIT_APP_CENTER[] = "PC_DOCK_EXIT_APP_CENTER";

    inline static constexpr char PC_DOCK_INTO_APP_CENTER[] = "PC_DOCK_INTO_APP_CENTER";

    inline static constexpr char PC_INTO_APP_CENTER_ON_RECENT[] = "PC_INTO_APP_CENTER_ON_RECENT";

    inline static constexpr char PC_INTO_RECENT[] = "PC_INTO_RECENT";

    inline static constexpr char PC_SPLIT_SCROLL_RECENT[] = "PC_SPLIT_SCROLL_RECENT";

    inline static constexpr char PC_SPLIT_START_ANIMATE[] = "PC_SPLIT_START_ANIMATE";

    inline static constexpr char SMARTDOCK_RECENTANIM_FIRSTOPEN[] = "SMARTDOCK_RECENTANIM_FIRSTOPEN";

    inline static constexpr char PC_SHORTCUT_GLOBAL_SEARCH[] = "PC_SHORTCUT_GLOBAL_SEARCH";

    inline static constexpr char SWITCH_DESKTOP[] = "SWITCH_DESKTOP";
    
    inline static constexpr char APP_ASSOCIATED_START[] = "APP_ASSOCIATED_START";

    inline static constexpr char CONTACTS_DIALER_BUTTON_PRESS[] = "CONTACTS_DIALER_BUTTON_PRESS";

    inline static constexpr char LAUNCHER_CARD_TEMP_SHOW[] = "LAUNCHER_CARD_TEMP_SHOW";

    inline static constexpr char GESTURE_TO_RECENTS[] = "GESTURE_TO_RECENTS";

    inline static constexpr char SWIPER_FROM_BOTTOM[] = "SWIPER_FROM_BOTTOM";

    inline static constexpr char START_APP_ANI_AG[] = "START_APP_ANI_AG";

    inline static constexpr char CORE_METHOD_DESKTOP_SHOW[] = "CORE_METHOD_DESKTOP_SHOW";

    inline static constexpr char APP_START[] = "APP_START";

    inline static constexpr char PC_SPLIT_DRAG_DIVIDER_ANIMATE[] = "PC_SPLIT_DRAG_DIVIDER_ANIMATE";

    inline static constexpr char PC_STARTUP_TIME[] = "PC_STARTUP_TIME";

    inline static constexpr char PC_WAKEUP_LATENCY[] = "PC_WAKEUP_LATENCY";

    inline static constexpr char APP_EXIT_FROM_RECENT[] = "APP_EXIT_FROM_RECENT";

    inline static constexpr char COLLABORATION_ANIMATION[] = "COLLABORATION_ANIMATION";

    inline static constexpr char CUSTOM_ANIMATOR_ROTATE90ACW[] = "CUSTOM_ANIMATOR rotate90Acw";

    inline static constexpr char CUSTOM_ANIMATOR_MOVINGIMAGEROTATE90ACW[] = "CUSTOM_ANIMATOR MovingImageRotate90Acw";

    inline static constexpr char EXIT_APP_CENTER[] = "EXIT_APP_CENTER";

    inline static constexpr char PC_RESTORE_DESKTOP_GESTURE_OPERATION[] = "PC_RESTORE_DESKTOP_GESTURE_OPERATION";

    inline static constexpr char PC_SHOW_DESKTOP_GESTURE[] = "PC_SHOW_DESKTOP_GESTURE";

    inline static constexpr char PC_TO_RECENT_GESTURE[] = "PC_TO_RECENT_GESTURE";

    inline static constexpr char PC_ONE_FIN_SHOW_DESKTOP_GESTURE[] = "PC_ONE_FIN_SHOW_DESKTOP_GESTURE";

    inline static constexpr char PC_ONE_FINGER_TO_RECENT_GESTURE[] = "PC_ONE_FINGER_TO_RECENT_GESTURE";

    inline static constexpr char WINDOW_DO_RESET_SCALE_ANIMATION[] = "WINDOW_DO_RESET_SCALE_ANIMATION";

    inline static constexpr char WINDOW_DO_SCALE_ANIMATION[] = "WINDOW_DO_SCALE_ANIMATION";

    inline static constexpr char AUTO_APP_SWIPER_FLING[] = "AUTO_APP_SWIPER_FLING";

    inline static constexpr char GESTURE_NETER_TO_SWITCH[] = "GESTURE_NETER_TO_SWITCH";
  
    inline static constexpr char FORM_MANAGER_CREATE_FORM[] = "FORM_MANAGER_CREATE_FORM";
  
    inline static constexpr char FORM_MANAGER_DELETE_FORM[] = "FORM_MANAGER_DELETE_FORM";
  
    inline static constexpr char FORM_STACK_DELETE_FORM[] = "FORM_STACK_DELETE_FORM";
  
    inline static constexpr char LOCKSCREEN_WIDGET_LAUNCH[] = "LOCKSCREEN_WIDGET_LAUNCH";
  
    inline static constexpr char LOCKSCREEN_WIDGET_EXIT[] = "LOCKSCREEN_WIDGET_EXIT";
  
    inline static constexpr char LOCKSCREEN_CAMERA_LAUNCH[] = "LOCKSCREEN_CAMERA_LAUNCH";
  
    inline static constexpr char LOCKSCREEN_CAMERA_EXIT[] = "LOCKSCREEN_CAMERA_EXIT";
  
    inline static constexpr char LOCKSCREEN_WIDGET_EXPAND[] = "LOCKSCREEN_WIDGET_EXPAND";
  
    inline static constexpr char LOCKSCREEN_WIDGET_COLLAPSE[] = "LOCKSCREEN_WIDGET_COLLAPSE";

    inline static constexpr char STATIC_ENTER_CROP[] = "STATIC_ENTER_CROP";

    inline static constexpr char MOVING_ENTER_CROP[] = "MOVING_ENTER_CROP";

    inline static constexpr char STATIC_ADD_TEXT[] = "STATIC_ADD_TEXT";

    inline static constexpr char STATIC_ADD_STYLUS[] = "STATIC_ADD_STYLUS";

    inline static constexpr char STATIC_CROP_DRAG[] = "STATIC_CROP_DRAG";

    inline static constexpr char MOVING_CROP_DRAG[] = "MOVING_CROP_DRAG";

    inline static constexpr char STATIC_SET_MIRROR[] = "STATIC_SET_MIRROR";

    inline static constexpr char MOVING_SET_MIRROR[] = "MOVING_SET_MIRROR";

    inline static constexpr char SEAL_SHEET_OPEN[] = "SEAL_SHEET_OPEN";

    inline static constexpr char SEAL_SHEET_CLOSE[] = "SEAL_SHEET_CLOSE";

    inline static constexpr char STATIC_SEAL_SWITCH[] = "STATIC_SEAL_SWITCH";

    inline static constexpr char MOVING_SEAL_SWITCH[] = "MOVING_SEAL_SWITCH";

    inline static constexpr char XMAGE_OPEN[] = "XMAGE_OPEN";

    inline static constexpr char XMAGE_CLOSE[] = "XMAGE_CLOSE";

    inline static constexpr char ADJUST_XMAGE[] = "ADJUST_XMAGE";

    inline static constexpr char STATIC_ADJUST_APERTURE[] = "STATIC_ADJUST_APERTURE";

    inline static constexpr char MOVING_ADJUST_APERTURE[] = "MOVING_ADJUST_APERTURE";
    
    inline static constexpr char CELIA_EXPAND_TO_FULL_SCREEN[] = "CELIA_EXPAND_TO_FULL_SCREEN";

    // only for watch
    inline static constexpr char WATCH_SCROLL_CARD_LIST_ANI[] = "WATCH_SCROLL_CARD_LIST_ANI";

    inline static constexpr char WATCH_WATCHFACE_LONGPRESS_TO_LIST[] = "WATCH_WATCHFACE_LONGPRESS_TO_LIST";

    inline static constexpr char WATCH_WATCHFACELIST_CLICK_TO_EDIT[] = "WATCH_WATCHFACELIST_CLICK_TO_EDIT";

    inline static constexpr char WATCH_WATCHFACESELECT_TO_WATCHFACE[] = "WATCH_WATCHFACESELECT_TO_WATCHFACE";

    inline static constexpr char WATCH_WATCHFACE_STYLE_SWIPE[] = "WATCH_WATCHFACE_STYLE_SWIPE";

    inline static constexpr char WATCH_POWER_DOUBLE_CLICK_TO_RECENTS[] = "WATCH_POWER_DOUBLE_CLICK_TO_RECENTS";

    inline static constexpr char WATCH_INTO_CARD_ANI[] = "WATCH_INTO_CARD_ANI";

    inline static constexpr char WATCH_EXIT_CARD_ANI[] = "WATCH_EXIT_CARD_ANI";

    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_WATCHFUNCKEY[] = "LAUNCHER_APP_LAUNCH_FROM_WATCHFUNCKEY";

    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_CONTROLCENTER[] = "LAUNCHER_APP_LAUNCH_FROM_CONTROLCENTER";

    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_WATCHFACE[] = "LAUNCHER_APP_LAUNCH_FROM_WATCHFACE";

    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_NEGATIVESCREEN[] = "LAUNCHER_APP_LAUNCH_FROM_NEGATIVESCREEN";

    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_CARD[] = "LAUNCHER_APP_LAUNCH_FROM_CARD";

    inline static constexpr char LAUNCHER_APP_LAUNCH_FROM_APPCENTER[] = "LAUNCHER_APP_LAUNCH_FROM_APPCENTER";
    
    inline static constexpr char ENTER_ONE_STEP_SPLIT[] = "ENTER_ONE_STEP_SPLIT";

    inline static constexpr char ENTER_SPLIT[] = "ENTER_SPLIT";

    inline static constexpr char SPLIT_TO_FLOAT[] = "SPLIT_TO_FLOAT";

    inline static constexpr char ADJUST_SPLIT_RATIO[] = "ADJUST_SPLIT_RATIO";

    inline static constexpr char SPLIT_SWAP_ANIMATION[] = "SPLIT_SWAP_ANIMATION";

    inline static constexpr char SPLIT_BACK_SEC[] = "SPLIT_BACK_SEC";

    inline static constexpr char SPLIT_DRAG_EXIT[] = "SPLIT_DRAG_EXIT";
    
    inline static constexpr char ENTER_ONE_STEP_FLOAT[] = "ENTER_ONE_STEP_FLOAT";

    inline static constexpr char FLOAT_START_FROM_DOCK[] = "FLOAT_START_FROM_DOCK";

    inline static constexpr char FLOAT_START_FROM_SIDEBAR[] = "FLOAT_START_FROM_SIDEBAR";

    inline static constexpr char EXIT_FLOAT_BY_CLICK[] = "EXIT_FLOAT_BY_CLICK";

    inline static constexpr char FLOAT_TO_FULL[] = "FLOAT_TO_FULL";

    inline static constexpr char MINIMIZE_FLOAT_BY_CLICK[] = "MINIMIZE_FLOAT_BY_CLICK";

    inline static constexpr char DRAG_FLOAT[] = "DRAG_FLOAT";

    inline static constexpr char MINI_FLOAT_TO_DEFAULT[] = "MINI_FLOAT_TO_DEFAULT";

    inline static constexpr char SPLIT_STYLE_EXCHANGE[] = "SPLIT_STYLE_EXCHANGE";

    inline static constexpr char ENTER_ONE_STEP_MIDSCENE[] = "ENTER_ONE_STEP_MIDSCENE";

    inline static constexpr char ENTER_MIDSCENE[] = "ENTER_MIDSCENE";

    inline static constexpr char MIDSCENE_REPLACE[] = "MIDSCENE_REPLACE";

    inline static constexpr char MIDSCENE_TO_FULL[] = "MIDSCENE_TO_FULL";

    inline static constexpr char MIDSCENE_TO_SPLIT[] = "MIDSCENE_TO_SPLIT";

    inline static constexpr char MIDSCENE_FOCUS_TRANSFER[] = "MIDSCENE_FOCUS_TRANSFER";

    inline static constexpr char MIDSCENE_ROTATION[] = "MIDSCENE_ROTATION";

    inline static constexpr char MIDSCENE_TO_RECENT[] = "MIDSCENE_TO_RECENT";

    inline static constexpr char SAVE_COMBINATION[] = "SAVE_COMBINATION";

    inline static constexpr char START_COMBINATION[] = "START_COMBINATION";

    inline static constexpr char APP_NAME_UGC_AWEME[] = "com.ss.hm.ugc.aweme";

    inline static constexpr char APP_NAME_KUAISHOU[] = "com.kuaishou.hmapp";

    inline static constexpr char APP_NAME_KUAISHOU_HMNEBULA[] = "com.kuaishou.hmnebula";
};
} // namespace OHOS
} // namespace HiviewDFX
#endif // XPERF_SCENE_ID_H
