/**
 ******************************************************************************
 * @file    lv_100ask_xz_ai_main.c
 * @author  百问科技
 * @version V1.0
 * @date    2025-3-17
 * @brief	100ask XiaoZhi AI base on LVGL
 ******************************************************************************
 * Change Logs:
 * Date           Author          Notes
 * 2025-3-17     zhouyuebiao     First version
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2008-2025 深圳百问网科技有限公司<https://www.100ask.net/>
 * All rights reserved
 *
 * 代码配套的视频教程：
 *      B站：   https://www.bilibili.com/video/BV1WE421K75k
 *      百问网：https://fnwcn.xetslk.com/s/39njGj
 *      淘宝：  https://detail.tmall.com/item.htm?id=779667445604
 *
 * 本程序遵循MIT协议, 请遵循协议！
 * 免责声明: 百问网编写的文档, 仅供学员学习使用, 可以转发或引用(请保留作者信息),禁止用于商业用途！
 * 免责声明: 百问网编写的程序, 仅供学习参考，假如被用于商业用途, 但百问网不承担任何后果！
 *
 * 百问网学习平台   : https://www.100ask.net
 * 百问网交流社区   : https://forums.100ask.net
 * 百问网LVGL文档   : https://lvgl.100ask.net
 * 百问网官方B站    : https://space.bilibili.com/275908810
 * 百问网官方淘宝   : https://100ask.taobao.com
 * 百问网微信公众号 ：百问科技 或 baiwenkeji
 * 联系我们(E-mail):  support@100ask.net 或 fae_100ask@163.com
 *
 *                             版权所有，盗版必究。
 ******************************************************************************
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <pthread.h>

#include "lv_100ask_xz_ai_main.h"
#include "lvgl.h"
#include "lang_config.h"
#include "ui_system.h"
//#include "lv_types.h"

// 定义设备状态枚举类型
typedef enum DeviceState {
    kDeviceStateUnknown,
    kDeviceStateStarting,
    kDeviceStateWifiConfiguring,
    kDeviceStateIdle,
    kDeviceStateConnecting,
    kDeviceStateListening,
    kDeviceStateSpeaking,
    kDeviceStateUpgrading,
    kDeviceStateActivating,
    kDeviceStateFatalError
} DeviceState;

// 将设备状态转换为本地字符串
static const char* ConvertToLocalString(DeviceState state)
{
    switch (state) {
        case kDeviceStateUnknown:
            return UNKNOWN_STATUS;
        case kDeviceStateStarting:
            return INITIALIZING;
        case kDeviceStateWifiConfiguring:
            return NETWORK_CFG;
        case kDeviceStateIdle:
            return STANDBY;
        case kDeviceStateConnecting:
            return CONNECTING;
        case kDeviceStateListening:
            return LISTENING;
        case kDeviceStateSpeaking:
            return SPEAKING;
        case kDeviceStateUpgrading:
            return UPGRADING;
        case kDeviceStateActivating:
            return ACTIVATION;
        case kDeviceStateFatalError:
            return ERROR_STR;
    }

    return "未知状态";
}

void *createMessageQueue();
int callSendFaceRecRunCmd(void* obj);
int callSendRegRegisterCmd(void* obj, const char* register_name, char** error_message);
int callSendRegRegisterNumCmd(void* obj);
int callSendRegImgDropCmd(void* obj);
int callSendGetRegImgCmd(void* obj);
int callSendClearDataBaseCmd(void* obj);
static void hide_register_ui(void);
static void show_normal_ui(void);
static void hide_register_img_ui(void);
static void show_register_ui(void);
static void create_register_ui(lv_obj_t *parent);
static void create_register_img_ui(void);

static void* ui_message_queue;

/*********************
 *      DEFINES
 *********************/


/**********************
 *      TYPEDEFS
 **********************/
typedef struct _lv_100ask_xz_ai {
	lv_obj_t  * state_bar_img_wifi;
	lv_obj_t  * state_bar_label_state;
	lv_obj_t  * state_bar_img_battery;
	lv_obj_t  * img_emoji;
	lv_obj_t  * label_chat;
    lv_obj_t  * register_button;
    lv_obj_t  * register_button_label;
    lv_obj_t  * clear_database_button;
    lv_obj_t  * clear_database_label;
    lv_obj_t  * register_num;
    lv_obj_t  * input_field;
    lv_obj_t  * keyboard;
    lv_obj_t  * done_btn;
    lv_obj_t  * done_label;
    lv_obj_t  * exit_btn;
    lv_obj_t  * exit_label;
    lv_obj_t  * next_img_button;
    lv_obj_t  * next_img_label;
    lv_obj_t  * sure_img_button;
    lv_obj_t  * sure_img_label;
    lv_obj_t  * exit_img_button;
    lv_obj_t  * exit_img_label;
    lv_obj_t  * popup_msgbox;
    int         status_value;
    bool      b_label_chat_update;
    char      label_chat_context[200];
} T_lv_100ask_xz_ai, *PT_lv_100ask_xz_ai;


/**********************
 *  STATIC PROTOTYPES
 **********************/
static void init_freetype(void);
static void deinit_freetype(void);

static void init_style(void);

static void screen_onclicked_event_cb(lv_event_t * e);
void show_error_popup(const char *error_msg);

/**********************
 *  STATIC VARIABLES
 **********************/
static PT_lv_100ask_xz_ai g_pt_lv_100ask_xz_ai;

static lv_style_t g_style_chat_font;
static lv_style_t g_style_register_num_font;
static lv_style_t g_style_state_font;
static lv_style_t g_style_back_ground;

static lv_font_t * gp_chat_font_freetype;
static lv_font_t * gp_state_font_freetype;
static pthread_mutex_t lvgl_mutex;

/**********************
 *      MACROS
 **********************/


/**********************
 *   GLOBAL FUNCTIONS
 **********************/

 void lvgl_lock(void)
{
    pthread_mutex_lock(&lvgl_mutex);
}

void lvgl_unlock(void)
{
    pthread_mutex_unlock(&lvgl_mutex);
}


// 清除人脸按钮点击事件
static void clear_database_event(lv_event_t * e) {
    int reg_num = 0;
    char str[20];
    LV_LOG_USER("[event_cb]clear_database_event begin");
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        callSendClearDataBaseCmd(ui_message_queue);
        reg_num = callSendRegRegisterNumCmd(ui_message_queue);
        sprintf(str, "注册人数：%d", reg_num);
        lvgl_lock();
        lv_label_set_text(g_pt_lv_100ask_xz_ai->register_num, str);
        lvgl_unlock();
    }
}

// 完成按钮点击事件
static void done_button_click(lv_event_t * e) {
    LV_LOG_USER("[event_cb]done_button_click begin");
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        int reg_num = 0;
        char str[20];
        char *error_message;
        // 隐藏键盘并移除焦点
        const char *text = lv_textarea_get_text(g_pt_lv_100ask_xz_ai->input_field);
        callSendRegRegisterCmd(ui_message_queue, text, &error_message);
        if(error_message == NULL){
            reg_num = callSendRegRegisterNumCmd(ui_message_queue);
            sprintf(str, "注册人数：%d", reg_num);
            lv_label_set_text(g_pt_lv_100ask_xz_ai->register_num, str);
            hide_register_ui();
            show_normal_ui();
        }
        else{
            hide_register_ui();
            show_error_popup(error_message);
        }
    }
}
//图像确定界面 退出按钮点击事件
static void exit_img_cb(lv_event_t * e) {
    LV_LOG_USER("[event_cb]exit_img_cb begin");
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        hide_register_img_ui();
        show_normal_ui();
        callSendRegImgDropCmd(ui_message_queue);
    }
}

//图像确定界面 确定按钮点击事件
static void sure_img_cb(lv_event_t * e) {
    LV_LOG_USER("[event_cb]sure_img_cb begin");
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        int reg_num = 0;
        char str[20];
        hide_register_img_ui();
        show_register_ui();
    }
}

//图像确定界面 换一张按钮点击事件
static void next_img_cb(lv_event_t * e) {
    LV_LOG_USER("[event_cb]next_img_cb begin");
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        callSendRegImgDropCmd(ui_message_queue);
        callSendGetRegImgCmd(ui_message_queue);
    }
}

// 人脸注册界面返回按钮点击事件
static void exit_button_click(lv_event_t * e) {
    LV_LOG_USER("[event_cb]exit_button_click begin");
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        hide_register_ui();
        show_normal_ui();
        callSendRegImgDropCmd(ui_message_queue);
    }
}

void lv_100ask_xz_ai_main(void)
{
    int reg_num = 0;
    char str[20];
    ui_message_queue = createMessageQueue();

    //lv_display_t *disp = lv_display_get_default();

    //lv_display_set_physical_resolution(disp, 800, 480);

    //lv_display_set_rotation(disp, LV_DISP_ROTATION_90);
    /* init */
    ui_system_init();
    pthread_mutex_init(&lvgl_mutex, NULL);

    g_pt_lv_100ask_xz_ai = (T_lv_100ask_xz_ai *)lv_malloc(sizeof(T_lv_100ask_xz_ai));
    memset(g_pt_lv_100ask_xz_ai, 0, sizeof(T_lv_100ask_xz_ai));
    g_pt_lv_100ask_xz_ai->status_value = -1;
    g_pt_lv_100ask_xz_ai->b_label_chat_update = false;
    
    lv_fs_stdio_init();	

    init_freetype();
    init_style();

    /*back ground img*/
    lv_obj_add_style(lv_screen_active(), &g_style_back_ground, 0);

    /* state bar */
    lv_obj_t * cont_state_bar = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(cont_state_bar);
    lv_obj_set_size(cont_state_bar, LV_PCT(100), 40);
    lv_obj_set_align(cont_state_bar, LV_ALIGN_TOP_MID);
    lv_obj_set_style_radius(cont_state_bar, 0, 0);
    lv_obj_set_style_bg_opa(cont_state_bar, LV_OPA_60, 0);
    lv_obj_set_style_pad_hor(cont_state_bar, 10, 0);
    lv_obj_set_layout(cont_state_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont_state_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_state_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // wifi
    g_pt_lv_100ask_xz_ai->state_bar_img_wifi = lv_image_create(cont_state_bar);
    lv_image_set_src(g_pt_lv_100ask_xz_ai->state_bar_img_wifi, LV_SYMBOL_WIFI);
    
    // state
    g_pt_lv_100ask_xz_ai->state_bar_label_state = lv_label_create(cont_state_bar);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->state_bar_label_state, &g_style_state_font, 0);
    lv_obj_set_width(g_pt_lv_100ask_xz_ai->state_bar_label_state, LV_PCT(70));
    lv_label_set_text(g_pt_lv_100ask_xz_ai->state_bar_label_state, "待命");

    // battery
    g_pt_lv_100ask_xz_ai->state_bar_img_battery = lv_image_create(cont_state_bar);
    lv_image_set_src(g_pt_lv_100ask_xz_ai->state_bar_img_battery, LV_SYMBOL_BATTERY_FULL);


    /* emoji */ 
    // https://www.iconfont.cn/search/index?searchType=icon&q=%E5%9C%86%E8%84%B8%E8%A1%A8%E6%83%85
    g_pt_lv_100ask_xz_ai->img_emoji = lv_image_create(lv_screen_active());
    lv_image_set_src(g_pt_lv_100ask_xz_ai->img_emoji, ASSETS_PATH"img_naughty.png");
    lv_obj_align(g_pt_lv_100ask_xz_ai->img_emoji, LV_ALIGN_BOTTOM_MID, 0, -80);

    
    /* chat */
    g_pt_lv_100ask_xz_ai->label_chat = lv_label_create(lv_screen_active());
    lv_obj_set_width(g_pt_lv_100ask_xz_ai->label_chat, LV_PCT(90));
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->label_chat, &g_style_chat_font, 0);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->label_chat, "Hi！有什么可以帮到你呢？");
    lv_obj_align_to(g_pt_lv_100ask_xz_ai->label_chat, g_pt_lv_100ask_xz_ai->img_emoji, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_label_set_long_mode(g_pt_lv_100ask_xz_ai->label_chat, LV_LABEL_LONG_DOT);
    
    //button for register
    g_pt_lv_100ask_xz_ai->register_button = lv_button_create(lv_screen_active());
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->register_button, 120, 40);
    lv_obj_align_to(g_pt_lv_100ask_xz_ai->register_button, cont_state_bar, LV_ALIGN_OUT_BOTTOM_RIGHT, -5, 20);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->register_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->register_button, event_cb, LV_EVENT_CLICKED, "ButtonRegister");

    g_pt_lv_100ask_xz_ai->register_button_label = lv_label_create(g_pt_lv_100ask_xz_ai->register_button);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->register_button_label, &g_style_chat_font, 0);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->register_button_label, "注册");
    lv_obj_center(g_pt_lv_100ask_xz_ai->register_button_label);

    reg_num = callSendRegRegisterNumCmd(ui_message_queue);
    sprintf(str, "注册人数：%d", reg_num);
    g_pt_lv_100ask_xz_ai->register_num = lv_label_create(lv_screen_active());
    lv_obj_set_width(g_pt_lv_100ask_xz_ai->register_num, LV_PCT(50));
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->register_num, &g_style_register_num_font, 0);
    //lv_obj_set_style_bg_color(g_pt_lv_100ask_xz_ai->register_num, lv_color_white(), 0);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->register_num, str);
    lv_obj_align_to(g_pt_lv_100ask_xz_ai->register_num, g_pt_lv_100ask_xz_ai->register_button, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

    //button for register
    g_pt_lv_100ask_xz_ai->clear_database_button = lv_button_create(lv_screen_active());
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->clear_database_button, 120, 40);
    lv_obj_align_to(g_pt_lv_100ask_xz_ai->clear_database_button, cont_state_bar, LV_ALIGN_OUT_BOTTOM_LEFT, 5, 20);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->clear_database_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->clear_database_button, clear_database_event, LV_EVENT_CLICKED, "ButtonClearDataBase");

    g_pt_lv_100ask_xz_ai->clear_database_label = lv_label_create(g_pt_lv_100ask_xz_ai->clear_database_button);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->clear_database_label, &g_style_chat_font, 0);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->clear_database_label, "清除人脸");
    lv_obj_center(g_pt_lv_100ask_xz_ai->clear_database_label);
    // screen touch
    //lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_event_cb(lv_layer_top(), screen_onclicked_event_cb, LV_EVENT_CLICKED, NULL);
    
    //lv_obj_add_flag(g_pt_lv_100ask_xz_ai->register_button_label, LV_OBJ_FLAG_CLICKABLE);
    //LV_LOG_USER("[init]lv_obj_add_event_cb register label");
    //lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->register_button_label, event_cb, LV_EVENT_CLICKED, NULL);
    //LV_LOG_USER("[init]lv_obj_add_event_cb register label end");
    //lv_obj_move_foreground(g_pt_lv_100ask_xz_ai->register_button_label);

    create_register_ui(lv_screen_active());
    create_register_img_ui();
    hide_register_ui();
    hide_register_img_ui();
    show_normal_ui();
    callSendFaceRecRunCmd(ui_message_queue);
}

// 清除人脸按钮点击事件
static void pop_button_event_cb(lv_event_t * e) {
    LV_LOG_USER("[event_cb]pop_button_event_cb begin");
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        lvgl_lock();
        lv_obj_delete(g_pt_lv_100ask_xz_ai->popup_msgbox);
        g_pt_lv_100ask_xz_ai->popup_msgbox = NULL;
        lv_obj_delete(btn);
        lvgl_unlock();
    }
    show_normal_ui();
}

void show_error_popup(const char *error_msg)
{
    // 创建一个弹出框
    LV_LOG_USER("[show_error_popup] %s",error_msg);
    lvgl_lock();
    if(g_pt_lv_100ask_xz_ai->popup_msgbox != NULL){
        lv_msgbox_close(g_pt_lv_100ask_xz_ai->popup_msgbox);
        g_pt_lv_100ask_xz_ai->popup_msgbox = NULL;
    }
    g_pt_lv_100ask_xz_ai->popup_msgbox = lv_msgbox_create(lv_screen_active());
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->popup_msgbox, 400, 300);
    lv_obj_center(g_pt_lv_100ask_xz_ai->popup_msgbox);
    lv_obj_t *popup_label = lv_msgbox_add_text(g_pt_lv_100ask_xz_ai->popup_msgbox, error_msg);
    lv_obj_add_style(popup_label, &g_style_chat_font, 0);

    lv_obj_t* close_btn = lv_btn_create(lv_screen_active());
    lv_obj_set_size(close_btn, 160, 40);
    lv_obj_set_style_bg_color(close_btn, lv_palette_main(LV_PALETTE_BLUE), 0);  // 按钮背景色
    lv_obj_set_style_radius(close_btn, 6, 0); 
    lv_obj_add_event_cb(close_btn, pop_button_event_cb, LV_EVENT_CLICKED, "ButtonSure");
    lv_obj_align_to(close_btn, g_pt_lv_100ask_xz_ai->popup_msgbox, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_obj_add_style(close_label, &g_style_chat_font, 0);
    lv_obj_set_style_text_color(close_label, lv_color_white(), 0);  // 按钮文本颜色
    lv_label_set_text(close_label, "确定");
    lv_obj_center(close_label);
    lvgl_unlock();

}

void create_register_img_ui(void){
    //button for register
    g_pt_lv_100ask_xz_ai->next_img_button = lv_button_create(lv_screen_active());
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->next_img_button, 100, 40);
    lv_obj_align(g_pt_lv_100ask_xz_ai->next_img_button, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->next_img_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->next_img_button, next_img_cb, LV_EVENT_CLICKED, "ButtonNextImg");

    g_pt_lv_100ask_xz_ai->next_img_label = lv_label_create(g_pt_lv_100ask_xz_ai->next_img_button);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->next_img_label, &g_style_chat_font, 0);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->next_img_label, "换一张");
    lv_obj_center(g_pt_lv_100ask_xz_ai->next_img_label);

        //button for register
    g_pt_lv_100ask_xz_ai->sure_img_button = lv_button_create(lv_screen_active());
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->sure_img_button, 100, 40);
    lv_obj_align(g_pt_lv_100ask_xz_ai->sure_img_button, LV_ALIGN_BOTTOM_RIGHT, -5, -10);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->sure_img_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->sure_img_button, sure_img_cb, LV_EVENT_CLICKED, "ButtonRegister");

    g_pt_lv_100ask_xz_ai->sure_img_label = lv_label_create(g_pt_lv_100ask_xz_ai->sure_img_button);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->sure_img_label, &g_style_chat_font, 0);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->sure_img_label, "确定");
    lv_obj_center(g_pt_lv_100ask_xz_ai->sure_img_label);

    g_pt_lv_100ask_xz_ai->exit_img_button = lv_button_create(lv_screen_active());
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->exit_img_button, 100, 40);
    lv_obj_align(g_pt_lv_100ask_xz_ai->exit_img_button, LV_ALIGN_BOTTOM_LEFT, 5, -10);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->exit_img_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->exit_img_button, exit_img_cb, LV_EVENT_CLICKED, "ButtonRegister");

    g_pt_lv_100ask_xz_ai->exit_img_label = lv_label_create(g_pt_lv_100ask_xz_ai->exit_img_button);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->exit_img_label, &g_style_chat_font, 0);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->exit_img_label, "退出");
    lv_obj_center(g_pt_lv_100ask_xz_ai->exit_img_label);
}

void show_register_img_ui(void){
    lvgl_lock();
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->sure_img_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->sure_img_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->next_img_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->next_img_label, LV_OBJ_FLAG_HIDDEN); 
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->exit_img_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->exit_img_label, LV_OBJ_FLAG_HIDDEN); 
    lvgl_unlock();
}

void hide_register_img_ui(void){
    lvgl_lock();
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->sure_img_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->sure_img_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->next_img_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->next_img_label, LV_OBJ_FLAG_HIDDEN); 
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->exit_img_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->exit_img_label, LV_OBJ_FLAG_HIDDEN); 
    lvgl_unlock();
}

void hide_normal_ui(void){
    lvgl_lock();
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->img_emoji, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->label_chat, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->register_num, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->register_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->register_button_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->clear_database_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->clear_database_label, LV_OBJ_FLAG_HIDDEN);
    lvgl_unlock();
    //lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->register_button, LV_OBJ_FLAG_CLICKABLE);
}

void show_normal_ui(void){
    lvgl_lock();
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->img_emoji, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->label_chat, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->register_num, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->register_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->register_button_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->clear_database_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->clear_database_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->register_button, LV_OBJ_FLAG_CLICKABLE);
    lvgl_unlock();
}

void show_register_ui(void){
    lvgl_lock();
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->input_field, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->done_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->done_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->exit_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->exit_label, LV_OBJ_FLAG_HIDDEN);
    lv_textarea_set_cursor_pos(g_pt_lv_100ask_xz_ai->input_field, 0);
    lv_textarea_set_text(g_pt_lv_100ask_xz_ai->input_field, "");
    lvgl_unlock();
    //lv_obj_add_flag(g_pt_lv_100ask_xz_ai->done_btn, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_flag(g_pt_lv_100ask_xz_ai->input_field, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    //lv_obj_add_flag(g_pt_lv_100ask_xz_ai->keyboard, LV_OBJ_FLAG_CLICKABLE);
}

void hide_register_ui(void){
    lvgl_lock();
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->input_field, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->done_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->done_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->exit_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->exit_label, LV_OBJ_FLAG_HIDDEN);
    lvgl_unlock();
    //lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->done_btn, LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->input_field, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    //lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->keyboard, LV_OBJ_FLAG_CLICKABLE);
}

static void input_field_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    LV_LOG_USER("[event_cb]input_field_event_cb %d", code);
    if(code == LV_EVENT_FOCUSED) {
        lvgl_lock();
        lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->keyboard, LV_OBJ_FLAG_HIDDEN);// 显示键盘
        lvgl_unlock();
    } else if(code == LV_EVENT_DEFOCUSED) {
        lvgl_lock();
        lv_obj_add_flag(g_pt_lv_100ask_xz_ai->keyboard, LV_OBJ_FLAG_HIDDEN);// 隐藏键盘
        lvgl_unlock();
    }
}

void create_register_ui(lv_obj_t *parent){

    lv_style_t input_style;
    lv_style_t keyboard_bg_style;
    lv_style_t key_normal_style;

    lv_style_init(&input_style);
    lv_style_set_text_font(&input_style, gp_state_font_freetype);
    //lv_style_set_bg_color(&input_style, lv_color_white());
    //lv_style_set_border_width(&input_style, 2);
    //lv_style_set_border_color(&input_style, lv_color_hex(0x4A90E2));
    //lv_style_set_radius(&input_style, 12);
    //lv_style_set_pad_all(&input_style, 10);
    
    // 键盘背景样式
    lv_style_init(&keyboard_bg_style);
    lv_style_set_bg_color(&keyboard_bg_style, lv_color_hex(0x333333));
    lv_style_set_text_font(&keyboard_bg_style, gp_state_font_freetype);
    
    // 按键正常样式
    lv_style_init(&key_normal_style);
    lv_style_set_bg_color(&key_normal_style, lv_color_hex(0x555555));
    lv_style_set_text_color(&key_normal_style, lv_color_white());
    lv_style_set_pad_all(&key_normal_style, 10);
    lv_style_set_radius(&key_normal_style, 8);

    // 创建输入框
    g_pt_lv_100ask_xz_ai->input_field = lv_textarea_create(parent);
    lv_textarea_set_one_line(g_pt_lv_100ask_xz_ai->input_field, true);
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->input_field, 300, 50);
    lv_obj_align(g_pt_lv_100ask_xz_ai->input_field, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->input_field, &input_style, 0);
    lv_textarea_set_cursor_pos(g_pt_lv_100ask_xz_ai->input_field, 0);
    lv_textarea_set_placeholder_text(g_pt_lv_100ask_xz_ai->input_field, "please input...");

    // 创建键盘
    g_pt_lv_100ask_xz_ai->keyboard = lv_keyboard_create(parent);
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->keyboard, 600, 300);
    lv_obj_align(g_pt_lv_100ask_xz_ai->keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->keyboard, &keyboard_bg_style, 0);

    
    // 关联输入框和键盘
    lv_keyboard_set_textarea(g_pt_lv_100ask_xz_ai->keyboard, g_pt_lv_100ask_xz_ai->input_field);
    
    // 添加完成按钮
    g_pt_lv_100ask_xz_ai->done_btn = lv_btn_create(parent);
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->done_btn, 100, 40);
    lv_obj_align(g_pt_lv_100ask_xz_ai->done_btn, LV_ALIGN_TOP_MID, 80, 120);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->done_btn, &g_style_chat_font, 0);
    lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->done_btn, done_button_click, LV_EVENT_CLICKED, NULL);
    
    g_pt_lv_100ask_xz_ai->done_label = lv_label_create(g_pt_lv_100ask_xz_ai->done_btn);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->done_label, "确定");
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->done_label, &key_normal_style, 0);
    lv_obj_center(g_pt_lv_100ask_xz_ai->done_label);

        // 添加完成按钮
    g_pt_lv_100ask_xz_ai->exit_btn = lv_btn_create(parent);
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->exit_btn, 100, 40);
    lv_obj_align(g_pt_lv_100ask_xz_ai->exit_btn, LV_ALIGN_TOP_MID, -90, 120);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->exit_btn, &g_style_chat_font, 0);
    lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->exit_btn, exit_button_click, LV_EVENT_CLICKED, NULL);
    
    g_pt_lv_100ask_xz_ai->exit_label = lv_label_create(g_pt_lv_100ask_xz_ai->exit_btn);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->exit_label, "退出");
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->exit_label, &key_normal_style, 0);
    lv_obj_center(g_pt_lv_100ask_xz_ai->exit_label);

    // 焦点变化事件处理
    lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->input_field, input_field_event_cb, LV_EVENT_FOCUSED|LV_EVENT_DEFOCUSED, NULL);
}

void event_cb(lv_event_t * e)
{

    lv_event_code_t code = lv_event_get_code(e);
    //lv_obj_clear_flag(g_pt_lv_100ask_xz_ai->register_button, LV_OBJ_FLAG_CLICKABLE);
    hide_normal_ui();
    //create_register_ui(lv_screen_active());
    show_register_img_ui();
    callSendGetRegImgCmd(ui_message_queue);
}

void SetXiaoZhiState(int state){
    lvgl_lock();
    g_pt_lv_100ask_xz_ai->status_value = state;
    lvgl_unlock();
}

void UpdateXiaoZhiInfo(void){
    lvgl_lock();
    if(g_pt_lv_100ask_xz_ai->status_value != -1 && !lv_obj_has_flag(g_pt_lv_100ask_xz_ai->img_emoji, LV_OBJ_FLAG_HIDDEN) && !lv_obj_has_flag(g_pt_lv_100ask_xz_ai->state_bar_label_state, LV_OBJ_FLAG_HIDDEN)){
        lv_label_set_text(g_pt_lv_100ask_xz_ai->state_bar_label_state, ConvertToLocalString((DeviceState)g_pt_lv_100ask_xz_ai->status_value));
        if(g_pt_lv_100ask_xz_ai->status_value == kDeviceStateSpeaking)
            lv_image_set_src(g_pt_lv_100ask_xz_ai->img_emoji,  ASSETS_PATH "img_joke.png");
        if(g_pt_lv_100ask_xz_ai->status_value == kDeviceStateListening)
            lv_image_set_src(g_pt_lv_100ask_xz_ai->img_emoji, ASSETS_PATH "img_joke.png");
        g_pt_lv_100ask_xz_ai->status_value = -1;
    }

    if(g_pt_lv_100ask_xz_ai->b_label_chat_update && (!lv_obj_has_flag(g_pt_lv_100ask_xz_ai->label_chat, LV_OBJ_FLAG_HIDDEN))){
        lv_label_set_text(g_pt_lv_100ask_xz_ai->label_chat, g_pt_lv_100ask_xz_ai->label_chat_context);
        g_pt_lv_100ask_xz_ai->b_label_chat_update = false;
    }  
    lvgl_unlock();
}

void SetText(const char *str)
{
    lvgl_lock();
    strcpy(g_pt_lv_100ask_xz_ai->label_chat_context, str);
    g_pt_lv_100ask_xz_ai->b_label_chat_update = true;
    lvgl_unlock();
}


void OnClicked(void)
{
    static uint16_t index = 0;
    static char *str[][3] = {
        {"待命", "聆听", "回答"},
        {"现在是待命状态哦。", "现在是聆听状态哦。", "现在是回答状态哦。"},
        {ASSETS_PATH "img_joke.png", ASSETS_PATH "img_naughty.png", ASSETS_PATH "img_think.png"},
    };
#if 0
    lvgl_lock();
    lv_label_set_text(g_pt_lv_100ask_xz_ai->state_bar_label_state, str[0][index]);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->label_chat, str[1][index]);

    lv_image_set_src(g_pt_lv_100ask_xz_ai->img_emoji, str[2][index]);
    lvgl_unlock();
#endif
    if(index >= 2) index = 0;
    else index++;

    LV_LOG_USER("Clicked, index: %d", index);
}


/**********************
 *   STATIC FUNCTIONS
 **********************/
static void lv_100ask_xz_ai_main_deinit(void)
{
    deinit_freetype();
    lv_free(g_pt_lv_100ask_xz_ai);

    lv_deinit();
}



static void init_freetype(void)
{
    /*Create a font*/
    gp_chat_font_freetype = lv_freetype_font_create(PATH_PREFIX "HarmonyOS_Sans_SC_Regular.ttf",
                                                    LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                                    26,
                                                    LV_FREETYPE_FONT_STYLE_NORMAL);
                                                    
    gp_state_font_freetype = lv_freetype_font_create(PATH_PREFIX "HarmonyOS_Sans_SC_Regular.ttf",
                                                    LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                                    20,
                                                    LV_FREETYPE_FONT_STYLE_NORMAL);

    if((!gp_chat_font_freetype) || (!gp_state_font_freetype)) {
        LV_LOG_ERROR("freetype font create failed.");
        exit(-1);
    }
}

static void deinit_freetype(void)
{
    lv_freetype_font_delete(gp_chat_font_freetype);
    lv_freetype_font_delete(gp_state_font_freetype);
    lv_freetype_font_delete(gp_state_font_freetype);
}


static void init_style(void)
{
    /*Create style with the new font*/;
    lv_style_init(&g_style_chat_font);
    lv_style_set_text_font(&g_style_chat_font, gp_chat_font_freetype);
    lv_style_set_text_align(&g_style_chat_font, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&g_style_state_font);
    lv_style_set_text_font(&g_style_state_font, gp_state_font_freetype);
    lv_style_set_text_align(&g_style_state_font, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&g_style_register_num_font);
    lv_style_set_text_font(&g_style_register_num_font, gp_state_font_freetype);
    lv_style_set_text_align(&g_style_register_num_font, LV_TEXT_ALIGN_LEFT);

    lv_style_init(&g_style_back_ground);
    lv_style_set_bg_opa(&g_style_back_ground, LV_OPA_TRANSP);
}


static void screen_onclicked_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    lv_obj_t * btn = lv_event_get_current_target(e);
    lv_obj_t * btn1 = lv_event_get_target(e);

    LV_LOG_USER("[screen_onclicked_event_cb]Clicked anywhere %p, %p, %p", btn, btn1, g_pt_lv_100ask_xz_ai->register_button);

    if(btn == g_pt_lv_100ask_xz_ai->register_button)
    {
        LV_LOG_USER("[screen_onclicked_event_cb]Clicked button");
    }

}


