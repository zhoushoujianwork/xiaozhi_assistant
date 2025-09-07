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
#include "sensor_buf_manager.h"
#include "face_detection.h"
#include "face_recognition.h"
#include "text_paint.h"

#include "json.hpp"
#include "cfg.h"
#include "ipc_udp.h"
#include "mmz.h"
#include "kws.h"
#include "json.hpp"
#include "lvgl_ui.h"
#include "message_queue.h"

#if 0
#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#endif

using std::cerr;
using std::cout;
using std::endl;
using std::thread;

using json = nlohmann::json;

// 帧率统计相关
static bool fps_count=false;
static volatile unsigned kpu_frame_count = 0;
static struct timeval tv, tv2;
// 显示实例和OSD缓冲区
struct display* display = NULL;;
struct display_buffer* draw_buffer;

// 人脸识别结果互斥锁，防止后处理和结果绘制访问result冲突
static std::mutex result_mutex;
// kpu互斥锁，保证人脸识别和kws同时只有一个任务在使用kpu
static std::mutex kpu_mutex;
// 用于存储float类型的PCM数据给kws唤醒线程使用
std::deque<float> audio_buffer;
// 双端队列互斥锁，保证填入数据和获取数据的线程安全
std::mutex buffer_mutex;

// 人脸检测结果
static vector<FaceDetectionInfo> face_results;
// 人脸识别结果
static vector<FaceRecognitionInfo> face_rec_results;
// AI线程退出标志
std::atomic<bool> video_stop(false);

std::atomic<bool> audio_stop(false);
// 显示线程退出标志
std::atomic<bool> display_stop(false);
// 注册人名称
static std::string register_name;

// 字体文件配置
ChineseTextRenderer writepen("SourceHanSansSC-Normal-Min.ttf",20);

// 特征库已注册数量
static int reg_num=0;
// 注册截图相关
static std::vector<cv::Mat> sensor_bgr(3);
static cv::Mat dump_img(SENSOR_HEIGHT,SENSOR_WIDTH , CV_8UC3);

// 当前状态
int cur_state=1;
// 显示状态
int display_state=1;
// 语音唤醒状态，1为检测语音唤醒，0为不检测语音唤醒
int kws_state=1;
// 状态锁
static std::mutex cur_state_mutex;
static std::mutex display_state_mutex;

// 显示状态监控
static int last_display_state = -1;
static int display_state_change_count = 0;

// 帧计数结构体
struct FaceInfo {
    std::string name;
    int frame_count; // 帧计数
};
// 跟踪每个人脸的帧信息
std::vector<FaceInfo> face_track_info;
// 超过frame_count_thresh未出现的人脸从跟踪帧信息容器中移除
int frame_count_thresh=30;
// 当前帧人脸识别名称
std::vector<string> cur_rec_res;
// 和跟踪结构体中的差异人脸名称
std::vector<string> diff_rec_res;

// 与sound_app传输音频数据的端点
p_ipc_endpoint_t g_ipc_wakeup_detect_audio_ep;
p_ipc_endpoint_t g_ipc_wakeup_detect_control_ep;

// std::ofstream pcm_file("output.pcm", std::ios::binary);

/**
*@brief 使用方法打印
*/
void print_usage(const char *name)
{
    cout << "Usage: " << name << "<kmodel_det> <obj_thres> <nms_thres> <kmodel_recg> <recg_thres> <db_dir> <fps_count> <kmodel_kws> <num_keyword> <spot_thresh> <debug_mode>" << endl
         << "Options:" << endl
         << "  kmodel_det           人脸检测kmodel路径\n"
         << "  obj_thres            人脸检测kmodel阈值\n"
         << "  nms_thres            人脸检测kmodel nms阈值\n"
         << "  kmodel_recg          人脸识别kmodel路径\n"
         << "  recg_thres           人脸识别阈值\n"
         << "  db_dir               数据库目录\n"
         << "  fps_count            帧率统计开关\n"
         << "  kmodel_kws           语音唤醒 kmodel路径\n"
         << "  num_keyword          关键词数量\n"
         << "  spot_thresh          唤醒词激活阈值\n"
         << "  debug_mode           是否需要调试，0、1、2分别表示不调试、简单调试、详细调试\n"
         << "\n"
         << endl;
}

/**
 * @brief 打印终端操作帮助说明。
 *
 * 本函数用于向控制台输出当前程序支持的交互命令，便于用户了解操作流程。
 * 提供注册、识别、查询等功能入口。
 */
void print_help() {
    std::cout << "======== 操作说明 ========\n";
    std::cout << "请输入下列命令并按回车以执行对应操作：\n\n";

    std::cout << "  h/help    : 显示帮助说明（即本页内容）\n";
    std::cout << "  i         : 进入注册模式\n";
    std::cout << "              - 系统将自动截图用于人脸注册\n";
    std::cout << "              - 注册后继续输入该用户的姓名并回车完成绑定\n";
    std::cout << "  d         : 退出注册，返回识别模式\n";
    std::cout << "  n         : 显示当前已注册的人脸数量\n";
    std::cout << "  q         : 退出程序并清理资源\n\n";

    std::cout << "注意事项：\n";
    std::cout << "  - 注册截图时请确保画面中仅有一张清晰可见的人脸。\n";
    std::cout << "  - 姓名应使用可识别字符（如中文或英文），避免特殊符号。\n";

    std::cout << "==========================\n" << std::endl;
}

/**
 * @brief 显示系统诊断函数
 * 
 * 检查显示系统的各个组件状态，帮助诊断摄像头画面不显示的问题
 */
void diagnose_display_system() {
    std::cout << "\n======== 显示系统诊断 ========" << std::endl;
    
    // 检查显示对象
    if (display) {
        std::cout << "✓ 显示对象已初始化" << std::endl;
        std::cout << "  - 分辨率: " << display->width << "x" << display->height << std::endl;
        std::cout << "  - 文件描述符: " << display->fd << std::endl;
        std::cout << "  - 连接器ID: " << display->conn_id << std::endl;
        std::cout << "  - CRTC ID: " << display->crtc_id << std::endl;
    } else {
        std::cout << "✗ 显示对象未初始化" << std::endl;
    }
    
    // 检查绘制缓冲区
    if (draw_buffer) {
        std::cout << "✓ 绘制缓冲区已分配" << std::endl;
        std::cout << "  - 尺寸: " << draw_buffer->width << "x" << draw_buffer->height << std::endl;
        std::cout << "  - 大小: " << draw_buffer->size << " 字节" << std::endl;
        std::cout << "  - 内存映射: " << (draw_buffer->map ? "有效" : "无效") << std::endl;
    } else {
        std::cout << "✗ 绘制缓冲区未分配" << std::endl;
    }
    
    // 检查状态
    std::cout << "当前状态:" << std::endl;
    std::cout << "  - AI状态: " << cur_state << std::endl;
    std::cout << "  - 显示状态: " << display_state << std::endl;
    std::cout << "  - 语音唤醒状态: " << kws_state << std::endl;
    std::cout << "  - 显示状态变化次数: " << display_state_change_count << std::endl;
    
    // 检查线程状态
    std::cout << "线程状态:" << std::endl;
    std::cout << "  - 视频停止标志: " << (video_stop ? "是" : "否") << std::endl;
    std::cout << "  - 音频停止标志: " << (audio_stop ? "是" : "否") << std::endl;
    std::cout << "  - 显示停止标志: " << (display_stop ? "是" : "否") << std::endl;
    
    std::cout << "============================\n" << std::endl;
}

/**
 * @brief AI 推理主循环：处理 DMA Buffer 流，并根据状态执行人脸检测、识别、注册等操作
 *
 * @param argv           命令行参数数组（包含模型路径、阈值、数据库目录等）
 * @param video_device   V4L2 视频设备编号
 */
void face_proc(char *argv[], int video_device) {
    struct v4l2_drm_context context;
    struct v4l2_drm_video_buffer buffer;

    // 等待 display_proc 完成初始化
    result_mutex.lock();
    result_mutex.unlock();

    // 配置摄像头参数
    v4l2_drm_default_context(&context);
    context.device       = video_device;
    context.display      = false;  // AI线程不直接显示，只处理数据
    context.width        = SENSOR_WIDTH;
    context.height       = SENSOR_HEIGHT;
    context.video_format = v4l2_fourcc('B', 'G', '3', 'P'); // BGR planar
    context.buffer_num   = 3;

    std::cout << "[AI_THREAD] 初始化摄像头参数: device=" << video_device 
              << ", size=" << SENSOR_WIDTH << "x" << SENSOR_HEIGHT 
              << ", format=BGR_PLANAR, buffers=" << context.buffer_num << std::endl;

    if (v4l2_drm_setup(&context, 1, NULL)) {
        std::cerr << "[AI_THREAD] v4l2_drm_setup error" << std::endl;
        return;
    }
    std::cout << "[AI_THREAD] v4l2_drm_setup 成功" << std::endl;
    
    if (v4l2_drm_start(&context)) {
        std::cerr << "[AI_THREAD] v4l2_drm_start error" << std::endl;
        return;
    }
    std::cout << "[AI_THREAD] v4l2_drm_start 成功，开始摄像头数据流" << std::endl;

    // 初始化模型与数据库参数
    char* kmodel_det   = argv[1];
    float obj_thresh   = atof(argv[2]);
    float nms_thresh   = atof(argv[3]);
    char* kmodel_rec   = argv[4];
    float recg_thresh  = atof(argv[5]);
    char* db_rec       = argv[6];
    fps_count          = atoi(argv[7]);
    int debug_mode     = atoi(argv[11]);
    char empty_str[] = "";

    MessageQueue* mq = MessageQueue::getInstance();
    ai_cmd_message_t ai_message;

    FaceDetection fd(kmodel_det, obj_thresh, nms_thresh, {SENSOR_CHANNEL, SENSOR_HEIGHT, SENSOR_WIDTH}, debug_mode);
    FaceRecognition face_recg(kmodel_rec, recg_thresh, {SENSOR_CHANNEL, SENSOR_HEIGHT, SENSOR_WIDTH}, debug_mode);
    face_recg.database_init(db_rec);

    // 创建 DMA Tensor 缓冲区
    std::vector<std::tuple<int, void*>> tensors;
    for (unsigned i = 0; i < context.buffer_num; i++) {
        tensors.push_back({context.buffers[i].fd, context.buffers[i].mmap});
    }
    SensorBufManager sensor_buf({SENSOR_CHANNEL, SENSOR_HEIGHT, SENSOR_WIDTH}, tensors);

    // 主处理循环
    static int frame_count = 0;
    while (!video_stop) {
        int ret = v4l2_drm_dump(&context, 1000);
        if (ret) {
            std::cerr << "[AI_THREAD] v4l2_drm_dump error: " << ret << std::endl;
            continue;
        }
        
        frame_count++;
        if (frame_count % 30 == 0) {  // 每30帧打印一次
            std::cout << "[AI_THREAD] 处理第 " << frame_count << " 帧，buffer_index=" 
                      << context.vbuffer.index << std::endl;
        }
        
        // 检查摄像头数据是否有效
        if (frame_count % 60 == 0) {  // 每60帧检查一次数据有效性
            void* data = context.buffers[context.vbuffer.index].mmap;
            if (data) {
                // 检查数据是否全为0（可能表示摄像头无数据）
                uint8_t* data_ptr = (uint8_t*)data;
                bool all_zero = true;
                for (int i = 0; i < 1000 && all_zero; i++) {  // 只检查前1000字节
                    if (data_ptr[i] != 0) all_zero = false;
                }
                if (all_zero) {
                    std::cout << "[AI_THREAD] 警告：摄像头数据可能无效（全零数据）" << std::endl;
                } else {
                    std::cout << "[AI_THREAD] 摄像头数据正常" << std::endl;
                }
            } else {
                std::cout << "[AI_THREAD] 错误：摄像头缓冲区数据为空" << std::endl;
            }
        }
        //---------------------------------- 状态处理分支 ----------------------------------
        std::lock_guard<std::mutex> lock(cur_state_mutex);
        std::lock_guard<std::mutex> d_lock(display_state_mutex);
        if (cur_state == 1) { // 正常识别状态
            runtime_tensor input_tensor = sensor_buf.get_buf_for_index(context.vbuffer.index);
            fd.pre_process(input_tensor);
            kpu_mutex.lock();
            fd.inference();
            kpu_mutex.unlock();
            result_mutex.lock();
            face_results.clear();
            face_rec_results.clear();
            cur_rec_res.clear();
            fd.post_process({SENSOR_WIDTH, SENSOR_HEIGHT}, face_results);
            for (int i = 0; i < face_results.size(); ++i) {
                face_recg.pre_process(input_tensor, face_results[i].sparse_kps.points);
                kpu_mutex.lock();
                face_recg.inference();
                kpu_mutex.unlock();
                FaceRecognitionInfo recg_result;
                face_recg.database_search(recg_result);
                face_rec_results.push_back(recg_result);
                cur_rec_res.push_back(recg_result.name);
            }
            diff_rec_res.clear();
            // Step 1: 遍历当前帧所有人脸，更新跟踪状态
            for (const auto& cur_face : cur_rec_res) {
                bool found = false;

                for (auto& info : face_track_info) {
                    if (info.name == cur_face) {
                        found = true;
                        info.frame_count = 0; // 重置帧计数器
                        break;
                    }
                }

                if (!found) {
                    // 新人脸：添加到跟踪列表，并加入 diff_rec_res
                    face_track_info.push_back({cur_face, 0});
                    diff_rec_res.push_back(cur_face);
                }
            }
            // Step 2: 对于所有未出现在当前帧的人脸，frame_count +1
            for (auto& info : face_track_info) {
                if (std::find(cur_rec_res.begin(), cur_rec_res.end(), info.name) == cur_rec_res.end()) {
                    if (info.frame_count < frame_count_thresh) {
                        info.frame_count++;
                    }
                }
            }
            // Step 3: 移除 frame_count >= frame_count_thresh 的人脸（表示消失超过30帧）
            face_track_info.erase(
                std::remove_if(face_track_info.begin(), face_track_info.end(),
                            [](const FaceInfo& info) {
                                return info.frame_count >= frame_count_thresh;
                            }),
                face_track_info.end()
            );
            // Step 4: 发送唤醒消息（仅发送第一个非 unknown 的人脸）
            if (!diff_rec_res.empty() && diff_rec_res[0] != "unknown") {
                std::cout << "识别到人脸：" << diff_rec_res[0] << "，唤醒小智！" << std::endl;

                json j;
                j["type"] = "wake-up";
                j["status"] = "start";
                j["wake-up_method"] = "video";
                j["wake-up_text"] = diff_rec_res[0];

                std::string textString = j.dump();
                g_ipc_wakeup_detect_control_ep->send(g_ipc_wakeup_detect_control_ep, textString.data(), textString.size());
            }
            result_mutex.unlock();
            cur_state = 1;
        }
        else if (cur_state == 2) { // 显示注册人数
            reg_num = face_recg.database_count(db_rec);
            ai_message.data = reg_num;
            ai_message.error_message = empty_str;
            ai_message.success = 1;
            mq->SendAIMsg(&ai_message);

            cur_state = 1;
        }

        else if (cur_state == 3) { // 抓取注册图片
            dump_img.setTo(cv::Scalar(0, 0, 0));
            sensor_bgr.clear();
            void* data = context.buffers[0].mmap;
            cv::Mat ori_img_B(SENSOR_HEIGHT, SENSOR_WIDTH, CV_8UC1, data);
            cv::Mat ori_img_G(SENSOR_HEIGHT, SENSOR_WIDTH, CV_8UC1, data + SENSOR_HEIGHT * SENSOR_WIDTH);
            cv::Mat ori_img_R(SENSOR_HEIGHT, SENSOR_WIDTH, CV_8UC1, data + 2 * SENSOR_HEIGHT * SENSOR_WIDTH);
            if (ori_img_B.empty() || ori_img_G.empty() || ori_img_R.empty()) {
                std::cout << "One or more of the channel images is empty." << std::endl;
                continue;
            }
            sensor_bgr.push_back(ori_img_R);
            sensor_bgr.push_back(ori_img_G);
            sensor_bgr.push_back(ori_img_B);
            cv::merge(sensor_bgr, dump_img);
            ai_message.data = reg_num;
            ai_message.error_message = empty_str;
            ai_message.success = 1;
            mq->SendAIMsg(&ai_message);

            cur_state = 1;
            display_state = 3;
        }
        else if(cur_state==4){
            ai_message.data = reg_num;
            ai_message.error_message = empty_str;
            ai_message.success = 1;
            mq->SendAIMsg(&ai_message);
            cur_state = 1;
            display_state = 1;
        }
        else if (cur_state == 5) { // 注册模式
            int ori_w = dump_img.cols;
            int ori_h = dump_img.rows;
            std::vector<uint8_t> chw_vec;
            std::vector<cv::Mat> bgrChannels(3);
            cv::split(dump_img, bgrChannels);
            for (int i = 2; i >= 0; --i) {
                std::vector<uint8_t> data_vec(bgrChannels[i].reshape(1, 1));
                chw_vec.insert(chw_vec.end(), data_vec.begin(), data_vec.end());
            }
            dims_t ai2d_in_shape{1, 3, ori_h, ori_w};
            runtime_tensor ai2d_in_tensor = host_runtime_tensor::create(typecode_t::dt_uint8, ai2d_in_shape, hrt::pool_shared)
                .expect("cannot create input tensor");
            auto input_buf = ai2d_in_tensor.impl()->to_host().unwrap()->buffer().as_host().unwrap()
                                 .map(map_access_::map_write).unwrap().buffer();
            memcpy(reinterpret_cast<char*>(input_buf.data()), chw_vec.data(), chw_vec.size());
            hrt::sync(ai2d_in_tensor, sync_op_t::sync_write_back, true).expect("write back input failed");
            fd.pre_process(ai2d_in_tensor);
            kpu_mutex.lock();
            fd.inference();
            kpu_mutex.unlock();
            result_mutex.lock();
            face_results.clear();
            fd.post_process({SENSOR_WIDTH, SENSOR_HEIGHT}, face_results);
            int face_num = face_results.size();
            if (face_num == 1) {
                face_recg.pre_process(ai2d_in_tensor, face_results[0].sparse_kps.points);
                kpu_mutex.lock();
                face_recg.inference();
                kpu_mutex.unlock();
                std::cout<<register_name<<std::endl;
                face_recg.database_add(register_name, db_rec);
                reg_num = face_recg.database_count(db_rec);
                ai_message.data = reg_num;
                ai_message.error_message = empty_str;
                ai_message.success = 1;
                mq->SendAIMsg(&ai_message);
                cur_state = 1;
                display_state = 1;
            } else if(face_num > 1){
                char error_message[] = "该图像包含多张人脸，注册失败！";
                ai_message.data = reg_num;
                ai_message.error_message = error_message;
                ai_message.success = 0;
                mq->SendAIMsg(&ai_message);
                cur_state = 1;
                display_state = 1;
            }else{
                char error_message[] = "该图像无法检测到人脸，注册失败！";
                ai_message.data = reg_num;
                ai_message.error_message = error_message;
                ai_message.success = 0;
                mq->SendAIMsg(&ai_message);
                cur_state = 1;
                display_state = 1;
            }
            result_mutex.unlock();
        }
        else if (cur_state == 6) { //数据库清空
            face_recg.database_reset(db_rec);
            reg_num = face_recg.database_count(db_rec);
            ai_message.data = reg_num;
            ai_message.error_message = empty_str;
            ai_message.success = 1;
            mq->SendAIMsg(&ai_message);
            cur_state = 1;
        }
        //---------------------------------- 状态处理结束 ----------------------------------
        kpu_frame_count += 1;
        v4l2_drm_dump_release(&context);
    }

    v4l2_drm_stop(&context);
}

static int process_ui_data(char* buffer,size_t size, void *user_data)
{

    return 0;
}

static int process_audio_data(char* buffer,size_t size, void *user_data)
{
    size_t sample_size=size/2;
    // if (pcm_file.is_open()) {
    //     pcm_file.write(buffer, size);
    //     // 如果你想每收到一次数据就flush，可选：
    //     // pcm_file.flush();
    // }
    // 加锁
    std::lock_guard<std::mutex> lock(buffer_mutex);
    // 每次接收1920字节 = 960个 int16_t
    for (int i = 0; i <sample_size ; ++i) {
        int16_t sample = (buffer[2 * i] & 0xFF) | ((buffer[2 * i + 1] & 0xFF) << 8);
        audio_buffer.push_back(static_cast<float>(sample));
    }
    // 控制缓冲区最大长度，避免无限增长，kws一次推理需要9600字节数据，4800采样值
    if (audio_buffer.size() > 19200) { // 最多缓存4次推理数据
        audio_buffer.erase(audio_buffer.begin(), audio_buffer.begin() + (audio_buffer.size() - 19200));
    }

    return 0;
}

static int process_wakeup_word_control_info(char *buffer, size_t size, void *user_data)
{
    printf("========process_wakeup_word_control_info===========:%s\n", buffer);

    json j = json::parse(buffer);
    if (!j.contains("type"))
        return 0;

    if (!j.contains("status"))
        return 0;

    if (j["type"] == "wake-up" && j["status"] == "stop") {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        audio_buffer.clear();
        kws_state=1;
        std::cout << "对话已结束，唤醒检测启动！" << std::endl;
    }

    return 0;
}

void kws_proc(char *argv[]){
    char* kmodel_kws    = argv[8];
    int num_keyword     = atoi(argv[9]);
    float spot_thresh   = atof(argv[10]);
    int debug_mode      = atoi(argv[11]);

    unsigned int sample_rate = 16000;
    std::vector<float> wav;

    // 初始化KWS模型
    KWS kws(kmodel_kws, num_keyword, spot_thresh, debug_mode);
    std::string last_res="Deactivated!";
    std::string cur_res="Deactivated!";
    while (!audio_stop) {
        {
            //加锁
            std::lock_guard<std::mutex> lock(buffer_mutex);
            wav.clear();
            if (audio_buffer.size() < 4800) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            // 拷贝数据到本地缓冲，锁内快速完成
            wav.assign(audio_buffer.begin(), audio_buffer.begin() + 4800);
            audio_buffer.erase(audio_buffer.begin(), audio_buffer.begin() + 4800);
        }
        ScopedTiming st("kws total time", debug_mode);
        if(kws_state==1){
            bool ok = kws.pre_process(wav);
            kpu_mutex.lock();
            kws.inference();
            kpu_mutex.unlock();
            std::string results = kws.post_process();
            last_res=cur_res;
            cur_res=results;
            if ( last_res!= "Deactivated!" && cur_res=="Deactivated!") {
                kws_state=0;
                std::cout << "已唤醒！" << std::endl;
                json j;
                j["type"] = "wake-up";
                j["status"] = "start";
                j["wake-up_method"] = "voice";
                j["wake-up_text"] = "小智小智";
                std::string textString = j.dump();
                g_ipc_wakeup_detect_control_ep->send(g_ipc_wakeup_detect_control_ep, textString.data(), textString.size());
            }
        }
        else{
            last_res="Deactivated!";
            cur_res="Deactivated!";
        }
    }
}

/**
 * @brief V4L2-DRM 显示帧处理函数（每帧触发一次）
 *
 * 该函数由 v4l2_drm_run 驱动循环回调，在每一帧显示数据时被调用。主要功能如下：
 * - 初始阶段：通知 AI 线程开始运行（解锁互斥锁）。
 * - 检查当前帧是否来自新缓冲区，如是，则进行显示绘制；
 * - 根据不同状态（如识别、注册、调试），在显示缓冲区上绘制对应图像或文字；
 * - 将绘制图像复制到实际 DRM 显示缓冲区；
 * - 支持人脸框绘制、文字信息显示、原图缩略图嵌入等；
 * - 可选择启用 FPS 打印功能（已注释）；
 *
 * @param context V4L2-DRM 上下文结构体指针
 * @param displayed 表示该帧是否已经被实际显示
 * @return 返回 0 表示正常，返回 'q' 表示请求退出主循环（受控于 display_stop 标志）
 */
int frame_handler(struct v4l2_drm_context *context, bool displayed)
{
    static bool first_frame = true;
    static unsigned response = 0, display_frame_count = 0;
    static int debug_frame_count = 0;
    
    if (first_frame) {
        // 第一帧解锁互斥量，允许 AI 处理线程继续执行
        std::cout << "[FRAME_HANDLER] 第一帧到达，解锁AI线程" << std::endl;
        result_mutex.unlock();
        first_frame = false;
    }
    
    response += 1;
    debug_frame_count++;
    
    if (displayed)
    {
        if (debug_frame_count % 30 == 0) {  // 每30帧打印一次
            std::cout << "[FRAME_HANDLER] 显示第 " << debug_frame_count << " 帧" << std::endl;
        }
        
        // 判断该帧的 buffer 是否被标记为持有可绘制数据
        if (context[0].buffer_hold[context[0].wp] >= 0)
        {
            static struct display_buffer* last_drawed_buffer = nullptr;
            auto buffer = context[0].display_buffers[context[0].buffer_hold[context[0].wp]];

            // 如果是新的一帧（不是重复帧），才进行绘制
            if (buffer != last_drawed_buffer) {
                if (debug_frame_count % 30 == 0) {
                    std::cout << "[FRAME_HANDLER] 新帧数据，开始绘制，buffer_hold=" 
                              << context[0].buffer_hold[context[0].wp] << std::endl;
                }
                // 创建临时 ARGB 显示缓冲（用于画图）
                cv::Mat temp_img(draw_buffer->height, draw_buffer->width, CV_8UC4);

                //---------------------- 显示状态机 ----------------------
                // 横屏：显示人脸检测框
                if (draw_buffer->width > draw_buffer->height)
                {
                    std::lock_guard<std::mutex> lock(display_state_mutex);
                    
                    // 监控显示状态变化
                    if (display_state != last_display_state) {
                        std::cout << "[FRAME_HANDLER] 显示状态变化: " << last_display_state 
                                  << " -> " << display_state << " (第" << ++display_state_change_count << "次变化)" << std::endl;
                        last_display_state = display_state;
                    }
                    
                    if (debug_frame_count % 30 == 0) {
                        std::cout << "[FRAME_HANDLER] 横屏模式，显示状态=" << display_state << std::endl;
                    }
                    // 正常显示人脸识别
                    if (display_state == 1) {
                        temp_img.setTo(cv::Scalar(0, 0, 0, 0));
                        result_mutex.lock();
                        if (debug_frame_count % 30 == 0) {
                            std::cout << "[FRAME_HANDLER] 绘制人脸识别结果，检测到 " << face_results.size() << " 个人脸" << std::endl;
                        }
                        for (size_t i = 0; i < face_results.size(); i++) {
                            FaceRecognition::draw_result(temp_img, face_results[i].bbox, face_rec_results[i]);
                        }
                        result_mutex.unlock();
                    }
                    // dump一帧图
                    else if (display_state == 3) {
                        temp_img.setTo(cv::Scalar(0, 0, 0, 0));
                        cv::cvtColor(dump_img, dump_img, cv::COLOR_BGR2BGRA);
                        cv::Mat resized_dump;
                        cv::resize(dump_img, resized_dump, cv::Size(temp_img.cols / 2, temp_img.rows / 2));
                        cv::Rect roi(0, 0, resized_dump.cols, resized_dump.rows);
                        resized_dump.copyTo(temp_img(roi));
                    }
                    else {
                        // 默认状态：识别状态
                        temp_img.setTo(cv::Scalar(0, 0, 0, 0));
                        result_mutex.lock();
                        for (size_t i = 0; i < face_results.size(); i++) {
                            FaceRecognition::draw_result(temp_img, face_results[i].bbox, face_rec_results[i]);
                        }
                        result_mutex.unlock();
                    }
                }
                else
                {
                    // 竖屏：绘制逻辑更复杂，需旋转和处理多种状态
                    cv::rotate(temp_img, temp_img, cv::ROTATE_90_COUNTERCLOCKWISE);
                    std::lock_guard<std::mutex> lock(display_state_mutex);
                    // 正常显示人脸识别
                    if (display_state == 1) {
                        temp_img.setTo(cv::Scalar(0, 0, 0, 0));
                        result_mutex.lock();
                        for (size_t i = 0; i < face_results.size(); i++) {
                            FaceRecognition::draw_result(temp_img, face_results[i].bbox, face_rec_results[i]);
                        }
                        result_mutex.unlock();
                    }
                    // dump一帧图
                    else if (display_state == 3) {
                        temp_img.setTo(cv::Scalar(0, 0, 0, 0));
                        cv::cvtColor(dump_img, dump_img, cv::COLOR_BGR2BGRA);
                        cv::Mat resized_dump;
                        cv::resize(dump_img, resized_dump, cv::Size(temp_img.cols , temp_img.rows));
                        cv::Rect roi(0, 0, resized_dump.cols, resized_dump.rows);
                        resized_dump.copyTo(temp_img(roi));
                    }
                    else {
                        // 默认状态：识别状态
                        temp_img.setTo(cv::Scalar(0, 0, 0, 0));
                        result_mutex.lock();
                        for (size_t i = 0; i < face_results.size(); i++) {
                            FaceRecognition::draw_result(temp_img, face_results[i].bbox, face_rec_results[i]);
                        }
                        result_mutex.unlock();
                    }

                    // 旋转回屏幕方向
                    cv::rotate(temp_img, temp_img, cv::ROTATE_90_CLOCKWISE);
                }
                //---------------------- 显示缓冲同步 ----------------------
                // 将绘图图像复制到实际显示缓冲区
                if (debug_frame_count % 30 == 0) {
                    std::cout << "[FRAME_HANDLER] 复制图像到显示缓冲区，size=" << draw_buffer->size << std::endl;
                }
                memcpy(draw_buffer->map, temp_img.data, draw_buffer->size);
                last_drawed_buffer = buffer;
                // 刷新缓存，通知显示设备更新
                thead_csi_dcache_clean_invalid_range(buffer->map, buffer->size);
                display_update_buffer(draw_buffer, 0, 0);
                if (debug_frame_count % 30 == 0) {
                    std::cout << "[FRAME_HANDLER] 显示缓冲区更新完成" << std::endl;
                }
            } else {
                if (debug_frame_count % 30 == 0) {
                    std::cout << "[FRAME_HANDLER] 跳过重复帧绘制" << std::endl;
                }
            }
        } else {
            if (debug_frame_count % 30 == 0) {
                std::cout << "[FRAME_HANDLER] 无可用显示缓冲区，buffer_hold=" 
                          << context[0].buffer_hold[context[0].wp] << std::endl;
            }
        }

        display_frame_count += 1;
    }

    // FPS counter
    if(fps_count){
        gettimeofday(&tv2, NULL);
        uint64_t duration = 1000000 * (tv2.tv_sec - tv.tv_sec) + tv2.tv_usec - tv.tv_usec;
        if (duration >= 1000000) {
            fprintf(stderr, " poll: %.2f, ", response * 1000000. / duration);
            response = 0;
            if (display) {
                fprintf(stderr, "display: %.2f, ", display_frame_count * 1000000. / duration);
                display_frame_count = 0;
            }
            fprintf(stderr, "camera: %.2f, ", context[0].frame_count * 1000000. / duration);
            context[0].frame_count = 0;
            fprintf(stderr, "KPU: %.2f", kpu_frame_count * 1000000. / duration);
            kpu_frame_count = 0;
            fprintf(stderr, "          \r");
            fflush(stderr);
            gettimeofday(&tv, NULL);
        }
    }

    // 若收到退出信号，返回 'q' 表示主循环退出
    if (display_stop) {
        return 'q';
    }

    return 0;
}



static void message_proc(int video_device){
    bool have_message = false;
    ui_cmd_message_t ui_message;

    std::cout << "message_proc init..." << std::endl;
    MessageQueue* mq = MessageQueue::getInstance();

    while(true){
        usleep(10000);
        have_message = mq->AIQueryCmd(&ui_message);
        if(have_message){
            std::cout << "get cmd: " << ui_message.cmd << std::endl;
            std::lock_guard<std::mutex> lock(cur_state_mutex);
            if(ui_message.cmd==1){
                cur_state = 1;
            }
            else if(ui_message.cmd==2){
                // 查询已注册人数（状态切换由主线程处理）
                cur_state = 2;
            }
            else if(ui_message.cmd==3){
                cur_state = 3;
            }
            else if(ui_message.cmd==4){
                cur_state=4;
            }
            else if(ui_message.cmd==5){
                cur_state=5;
                std::string name_str(ui_message.registered_name);
                register_name=name_str;
            }
            else if(ui_message.cmd==6){
                cur_state=6;
            }
            else if(ui_message.cmd==7){
                cur_state=7;
            }
            else if(ui_message.cmd==99){
                // 诊断命令
                diagnose_display_system();
            }else{
                cur_state=-1;
            }

        }
    }
}

/**
 * @brief 显示线程主函数，初始化 V4L2-DRM 并绑定绘制回调
 *
 * 根据屏幕方向（横屏 / 竖屏）配置对应的宽高、格式和旋转角度，
 * 然后调用 `v4l2_drm_run()` 启动帧处理主循环，由 `frame_handler()` 每帧触发绘制。
 *
 * @param video_device 视频设备编号（如 /dev/video0 中的 1）
 */
void display_proc(int video_device)
{
    struct v4l2_drm_context context;
    v4l2_drm_default_context(&context);
    context.device = video_device;

    std::cout << "[DISPLAY_THREAD] 开始初始化显示线程，device=" << video_device << std::endl;
    std::cout << "[DISPLAY_THREAD] 显示分辨率: " << display->width << "x" << display->height << std::endl;

    // 根据屏幕方向设置 width/height/rotation
    if (display->width > display->height)
    {
        // 横屏
        context.width = display->width;
        context.height = (display->width * SENSOR_HEIGHT / SENSOR_WIDTH) & 0xfff8;
        context.video_format = V4L2_PIX_FMT_NV12;
        context.display_format = 0;
        context.drm_rotation = rotation_0;
        std::cout << "[DISPLAY_THREAD] 横屏模式: " << context.width << "x" << context.height 
                  << ", rotation=0" << std::endl;
    }
    else
    {
        // 竖屏
        context.width = display->height;
        context.height = display->width;
        context.video_format = V4L2_PIX_FMT_NV12;
        context.display_format = 0;
        context.drm_rotation = rotation_90;
        std::cout << "[DISPLAY_THREAD] 竖屏模式: " << context.width << "x" << context.height 
                  << ", rotation=90" << std::endl;
    }

    // 初始化 V4L2 + DRM 流
    std::cout << "[DISPLAY_THREAD] 开始v4l2_drm_setup..." << std::endl;
    if (v4l2_drm_setup(&context, 1, &display)) {
        std::cerr << "[DISPLAY_THREAD] v4l2_drm_setup error" << std::endl;
        return;
    }
    std::cout << "[DISPLAY_THREAD] v4l2_drm_setup 成功" << std::endl;

    // 分配OSD显示 plane 和 buffer
    std::cout << "[DISPLAY_THREAD] 分配OSD显示缓冲区..." << std::endl;
    struct display_plane* plane = display_get_plane(display, DRM_FORMAT_ARGB8888);
    if (!plane) {
        std::cerr << "[DISPLAY_THREAD] display_get_plane 失败" << std::endl;
        return;
    }
    std::cout << "[DISPLAY_THREAD] 获取到plane: " << plane->plane_id << std::endl;
    
    draw_buffer = display_allocate_buffer(plane, display->width, display->height);
    if (!draw_buffer) {
        std::cerr << "[DISPLAY_THREAD] display_allocate_buffer 失败" << std::endl;
        return;
    }
    std::cout << "[DISPLAY_THREAD] 分配显示缓冲区成功: " << draw_buffer->width 
              << "x" << draw_buffer->height << ", size=" << draw_buffer->size << std::endl;
    
    display_commit_buffer(draw_buffer, 0, 0);
    std::cout << "[DISPLAY_THREAD] 提交初始显示缓冲区" << std::endl;

    if(fps_count){
        // 记录起始时间（用于 FPS 测试）
        gettimeofday(&tv, NULL);
        std::cout << "[DISPLAY_THREAD] FPS计数已启用" << std::endl;
    }

    // 启动显示主循环，绑定回调 frame_handler
    std::cout << "[DISPLAY_THREAD] 启动v4l2_drm_run主循环..." << std::endl;
    v4l2_drm_run(&context, 1, frame_handler);

    // 清理资源
    if (display) {
        display_free_plane(plane);
        display_exit(display);
    }

    return;
}

void __attribute__((destructor)) cleanup() {
    std::cout << "Cleaning up memory..." << std::endl;
    shrink_memory_pool();
    kd_mpi_mmz_deinit();
}

/**
 * @brief 主程序入口
 *
 * 初始化显示、启动 AI 和显示线程，处理用户终端输入命令，实现人脸识别与注册系统交互。
 */
int main(int argc, char *argv[])
{
    // 输出编译信息
    std::cout << "程序名：" << argv[0] << " | 构建时间：" << __DATE__ << " " << __TIME__ << std::endl;

    // 检查参数数量是否合法（此处要求 9 个参数）
    if (argc != 12)
    {
        print_usage(argv[0]); // 打印用法说明
        return -1;
    }

    // 初始化显示模块
    display = display_init(0);
    if (!display) {
        std::cerr << "显示初始化失败，程序退出！" << std::endl;
        return -1;
    }

    // 锁住结果互斥量，等待首次帧到来后解锁
    result_mutex.lock();

    //语音，视频唤醒词检测
    g_ipc_wakeup_detect_audio_ep = ipc_endpoint_create_udp(WAKEUP_WORD_DETECTION_AUDIO_PORT_UP, WAKEUP_WORD_DETECTION_AUDIO_PORT_DOWN, process_audio_data, NULL);

    g_ipc_wakeup_detect_control_ep = ipc_endpoint_create_udp(WAKEUP_WORD_DETECTION_CONTROL_PORT_UP, WAKEUP_WORD_DETECTION_CONTROL_PORT_DOWN, process_wakeup_word_control_info, NULL);
    // 启动人脸识别推理线程
    std::thread face_thread(face_proc, argv, 1);
    // 启动关键词唤醒推理线程
    std::thread kws_thread(kws_proc, argv);

    // 启动显示线程（处理显示内容绘制）
    std::thread display_thread(display_proc, 2);

    auto message_query_thread = thread(message_proc, 4);
    auto lvglui = LvglUi();

    // 输入提示信息
    std::cout << "输入 'h' 或 'help' 并回车 查看命令说明" << std::endl;

    // 定期诊断计数器
    static int diagnostic_counter = 0;
    
    while (true) {
        usleep(100000);
        
        // 每10秒进行一次诊断
        diagnostic_counter++;
        if (diagnostic_counter % 100 == 0) {  // 100 * 100ms = 10秒
            std::cout << "\n[MAIN] 定期系统诊断 (运行时间: " << diagnostic_counter/10 << "秒)" << std::endl;
            diagnose_display_system();
        }
    }
    // 等待线程完成后退出程序
    display_thread.join();
    face_thread.join();
    kws_thread.join();

    return 0;
}
