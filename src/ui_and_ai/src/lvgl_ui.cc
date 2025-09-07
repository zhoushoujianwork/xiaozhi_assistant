// SPDX-License-Identifier: GPL-3.0-only
/*
 * Copyright (c) 2025, Canaan Bright Sight Co., Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <thread>
#include "utils.h"
#include "vi_vo.h"
#include "text_paint.h"

#include "lvgl/lvgl.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>


#include "lv_100ask_xz_ai/lv_100ask_xz_ai.h"
#include "lvgl_ui.h"
#include "message_queue.h"

using std::cerr;
using std::cout;
using std::endl;
using std::thread;

extern struct display* display;

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 800
#define SCREEN_ROTATE 270 //90

// 触摸事件回调函数
static void touch_event_cb(lv_event_t * e)
{
#ifdef TOUCH_DEBUG_ENABLED
    lv_obj_t * obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    switch(code) {
        case LV_EVENT_PRESSED:
            printf("[TOUCH_DEBUG] UI_EVENT: PRESSED at (%d, %d)\n", 
                   lv_indev_get_act()->proc.types.pointer.act_point.x,
                   lv_indev_get_act()->proc.types.pointer.act_point.y);
            break;
        case LV_EVENT_RELEASED:
            printf("[TOUCH_DEBUG] UI_EVENT: RELEASED at (%d, %d)\n", 
                   lv_indev_get_act()->proc.types.pointer.act_point.x,
                   lv_indev_get_act()->proc.types.pointer.act_point.y);
            break;
        case LV_EVENT_PRESS_LOST:
            printf("[TOUCH_DEBUG] UI_EVENT: PRESS_LOST\n");
            break;
        case LV_EVENT_CLICKED:
            printf("[TOUCH_DEBUG] UI_EVENT: CLICKED at (%d, %d)\n", 
                   lv_indev_get_act()->proc.types.pointer.act_point.x,
                   lv_indev_get_act()->proc.types.pointer.act_point.y);
            break;
        default:
            break;
    }
#endif
}

static const char * getenv_default(const char *name, const char *dflt)
{
    return getenv(name) ? : dflt;
}

static void lv_linux_disp_init(struct display* display)
{
    const char *device = getenv_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    lv_display_t * disp = lv_linux_drm_create(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ROTATE);

    //lv_linux_drm_init(disp, device, display);
    struct display_plane* plane = display_get_plane(display, DRM_FORMAT_ARGB8888);
    //std::cout << "lv_linux_disp_init plane: " << plane->plane_id << std::endl;
    lv_linux_drm_init(disp, display->fd, display->conn_id, display->crtc_id, plane->plane_id, display->blob_id);

    lv_indev_t * touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event0", SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ROTATE);
    lv_evdev_set_swap_axes(touch, true);
    lv_indev_set_display(touch, disp);
    
    // 添加全局触摸事件监听
    lv_obj_add_event_cb(lv_screen_active(), touch_event_cb, LV_EVENT_ALL, NULL);
#ifdef TOUCH_DEBUG_ENABLED
    printf("[TOUCH_DEBUG] Touch device initialized and event listener added\n");
#endif
}

static void ui_proc(int device)
{
    if(display == NULL){
        std::cout << "display not init error !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        return;
    }

    sleep(5);
    std::cout << "ui_proc lvgl_init begin ======================================================" << std::endl;
    lv_init();

    /*Linux display device init*/
    lv_linux_disp_init(display);

    lv_100ask_xz_ai_main();

    /*Handle LVGL tasks*/
    while(1) {
        lv_timer_handler();
        UpdateXiaoZhiInfo();
        usleep(5000);
    }
}

LvglUi::LvglUi(){
    ui_thread = thread(ui_proc, 3);
}

LvglUi::~LvglUi(){
    ui_thread.join();
}

