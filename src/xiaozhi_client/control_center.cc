// SPDX-License-Identifier: GPL-3.0-only
/*
 * Copyright (c) 2008-2023 100askTeam : Dongshan WEI <weidongshan@100ask.net>
 * Discourse:  https://forums.100ask.net
 * Copyright (c) 2025, Canaan Bright Sight Co., Ltd
 */

/*  Copyright (C) 2008-2023 深圳百问网科技有限公司
 *  All rights reserved
 *
 * 免责声明: 百问网编写的文档, 仅供学员学习使用, 可以转发或引用(请保留作者信息),禁止用于商业用途！
 * 免责声明: 百问网编写的程序, 用于商业用途请遵循GPL许可, 百问网不承担任何后果！
 *
 * 本程序遵循GPL V3协议, 请遵循协议
 * 百问网学习平台   : https://www.100ask.net
 * 百问网交流社区   : https://forums.100ask.net
 * 百问网官方B站    : https://space.bilibili.com/275908810
 * 本程序所用开发板 : Linux开发板
 * 百问网官方淘宝   : https://100ask.taobao.com
 * 联系我们(E-mail) : weidongshan@100ask.net
 *
 *
 *          版权所有，盗版必究。
 *
 * 修改历史     版本号           作者        修改内容
 *-----------------------------------------------------
 * 2025.03.20      v01         百问科技      创建文件
 * 2025.05.20      v01         canaan       支持多种语音交互模式：手动，自动，realtime,fix bug
 *-----------------------------------------------------
 */
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <random>
#include <sstream>
#include <iomanip>
#include "webttsctrl.h"

// Include nlohmann/json library
#include "json.hpp"
#include "websocket_client.h"
#include "http.h"
#include "ipc_udp.h"
#include "uuid.h"

#include "cfg.h"
#include "control_center.h"

extern SpeechInteractionMode g_speech_interaction_mode;
using json = nlohmann::json;
int g_audio_upload_enable = 1;
static std::string g_session_id;
int g_websocket_start = 0;

WebttsConfig g_webtts_config;
std::string g_ota_url;
std::string g_ws_addr;

static p_ipc_endpoint_t g_ipc_ep_ui;
static DeviceState g_device_state = kDeviceStateUnknown;
static std::string g_uuid;
static std::string g_mac;
static websocket_data_t g_ws_data;
static server_audio_data_callback g_callback = NULL;
static audio_tts_state_callback g_tts_callback = NULL;
static ws_work_state_callback g_ws_state_callback = NULL;

static void create_default_xiaozhi_config(std::string &ota_url, std::string &ws_addr)
{
    ota_url = "https://api.tenclass.net/xiaozhi/ota/";
    ws_addr = "wss://api.tenclass.net:443/xiaozhi/v1/";
}

static void set_device_state(DeviceState state)
{
    g_device_state = state;
}

static void send_device_state(void)
{
    std::string stateString = "{\"state\":" + std::to_string(g_device_state) + "}";
    g_ipc_ep_ui->send(g_ipc_ep_ui, stateString.data(), stateString.size());
}

static void send_stt(const std::string& text)
{
    if (!g_ipc_ep_ui) {
        std::cerr << "Error: g_ipc_ep_ui is nullptr" << std::endl;
        return;
    }

    try {
        json j;
        j["text"] = text;
        std::string textString = j.dump();
        g_ipc_ep_ui->send(g_ipc_ep_ui, textString.data(), textString.size());
    } catch (const std::exception& e) {
        std::cerr << "Error creating JSON string: " << e.what() << std::endl;
    }
}


static void process_opus_data_downloaded(const char *buffer, size_t size)
{
    //printf("========audio data downloaded===========:%d\n",size);
    if (g_callback != NULL) {
        g_callback((const unsigned char *)buffer, size);
    }
}

static void send_abort_req()
{
    json j;
    j["session_id"] = g_session_id;
    j["type"] = "abort";
    j["reason"] = "wake_word_detected";

    std::string abortString = j.dump();
    //std::string abortString = "{\"session_id\":\"" + g_session_id + "\",\"type\":\"abort\",\"reason\":\"wake_word_detected\"}";
    try {
        //c->send(hdl, abortString, websocketpp::frame::opcode::text);
        //std::cout<< "Send before: " << abortString << std::endl;
        websocket_send_text(abortString.data(), abortString.size());
        //std::cout << "Send: " << abortString << std::endl;
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Error sending message: " << e << " (" << e.message() << ")" << std::endl;
    }
}

static void send_start_listening_req(ListeningMode mode)
{
    std::string startString = "{\"session_id\":\"" + g_session_id + "\"";

    startString += ",\"type\":\"listen\",\"state\":\"start\"";

    if (mode == kListeningModeAutoStop) {
        startString += ",\"mode\":\"auto\"}";
    } else if (mode == kListeningModeManualStop) {
        startString += ",\"mode\":\"manual\"}";
    } else if (mode == kListeningModeAlwaysOn) {
        startString += ",\"mode\":\"realtime\"}";
    }

    try {
        //c->send(hdl, startString, websocketpp::frame::opcode::text);
        websocket_send_text(startString.data(), startString.size());
        std::cout << "Send: " << startString << std::endl;
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Error sending message: " << e << " (" << e.message() << ")" << std::endl;
    }
}

static void process_hello_json(const char *buffer, size_t size)
{
    json j = json::parse(buffer);
    int sample_rate = j["audio_params"]["sample_rate"];
    int channels = j["audio_params"]["channels"];
    std::cout << "Received valid 'hello' message with sample_rate: " << sample_rate << " and channels: " << channels << std::endl;

    g_session_id = j["session_id"];
#if 0
    std::string desc = R"(
    {"session_id":"","type":"iot","update":true,"descriptors":[{"name":"Speaker","description":"扬声器","properties":{"volume":{"description":"当前音量值","type":"number"}},"methods":{"SetVolume":{"description":"设置音量","parameters":{"volume":{"description":"0到100之间的整数","type":"number"}}}}}]}
    )";

    // Send the new message
    try {
        //c->send(hdl, desc, websocketpp::frame::opcode::text);
        websocket_send_text(desc.data(), desc.size());
        std::cout << "Send: " << desc << std::endl;
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Error sending message: " << e << " (" << e.message() << ")" << std::endl;
    }

    std::string desc2 = R"(
    {"session_id":"","type":"iot","update":true,"descriptors":[{"name":"Backlight","description":"屏幕背光","properties":{"brightness":{"description":"当前亮度百分比","type":"number"}},"methods":{"SetBrightness":{"description":"设置亮度","parameters":{"brightness":{"description":"0到100之间的整数","type":"number"}}}}}]}
)";

    // Send the new message

    try {
        //c->send(hdl, desc2, websocketpp::frame::opcode::text);
        websocket_send_text(desc2.data(), desc2.size());
        std::cout << "Send: " << desc2 << std::endl;
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Error sending message: " << e << " (" << e.message() << ")" << std::endl;
    }

    std::string desc3 = R"(
    {"session_id":"","type":"iot","update":true,"descriptors":[{"name":"Battery","description":"电池管理","properties":{"level":{"description":"当前电量百分比","type":"number"},"charging":{"description":"是否充电中","type":"boolean"}},"methods":{}}]}
)";

    // Send the new message

    try {
        //c->send(hdl, desc3, websocketpp::frame::opcode::text);
        websocket_send_text(desc3.data(), desc3.size());
        std::cout << "Send: " << desc3 << std::endl;
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Error sending message: " << e << " (" << e.message() << ")" << std::endl;
    }
#endif

    std::string startString;
    if (g_speech_interaction_mode == kSpeechInteractionModeAuto) {
        startString = R"({"session_id":"","type":"listen","state":"start","mode":"auto"})";
    } else if (g_speech_interaction_mode == kSpeechInteractionModeManual) {
        startString = R"({"session_id":"","type":"listen","state":"start","mode":"manual"})";
    } else if (g_speech_interaction_mode == kSpeechInteractionModeRealtime) {
        startString = R"({"session_id":"","type":"listen","state":"start","mode":"realtime"})";
    } else if (g_speech_interaction_mode == kSpeechInteractionModeAutoWithWakeupWord) {
        startString = R"({"session_id":"","type":"listen","state":"start","mode":"auto"})";
    }

    try {
        //c->send(hdl, startString, websocketpp::frame::opcode::text);
        websocket_send_text(startString.data(), startString.size());
        std::cout << "Send: " << startString << std::endl;
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Error sending message: " << e << " (" << e.message() << ")" << std::endl;
    }

    g_audio_upload_enable = 1;

#if 0
    std::string state = R"(
        {"session_id":"","type":"iot","update":true,"states":[{"name":"Speaker","state":{"volume":80}},{"name":"Backlight","state":{"brightness":75}},{"name":"Battery","state":{"level":0,"charging":false}}]}
    )";

    try {
        //c->send(hdl, state, websocketpp::frame::opcode::text);
        websocket_send_text(state.data(), state.size());
        std::cout << "Send: " << state << std::endl;
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Error sending message: " << e << " (" << e.message() << ")" << std::endl;
    }
#endif

}

static void process_other_json(const char *buffer, size_t size)
{
    try {
        // Parse JSON data
        json j = json::parse(buffer);

        if (!j.contains("type"))
            return;

        if (j["type"] == "tts") {
            auto state = j["state"];
            if (state == "start") {
                // 下发语音, 可以关闭录音
                g_audio_upload_enable = 0;
                set_device_state(kDeviceStateListening);
                send_device_state();
                if (g_tts_callback != NULL) {
                    g_tts_callback(1);
                }
            } else if (state == "stop") {
                if (g_speech_interaction_mode == kSpeechInteractionModeAuto || g_speech_interaction_mode == kSpeechInteractionModeAutoWithWakeupWord) {
                    // 本次交互结束, 可以继续上传声音
                    // 等待一会以免她听到自己的话误以为再次对话
                    //sleep(2);
                    send_start_listening_req(kListeningModeAutoStop);
                } else if (g_speech_interaction_mode == kSpeechInteractionModeManual) {
                    // 本次交互结束, 可以继续上传声音
                    // 等待一会以免她听到自己的话误以为再次对话
                    //sleep(2);
                    send_start_listening_req(kListeningModeManualStop);
                } else if (g_speech_interaction_mode == kSpeechInteractionModeRealtime) {
                    ;// do nothing
                }

                set_device_state(kDeviceStateListening);
                send_device_state();

                if (g_tts_callback != NULL)
                    g_tts_callback(0);

                g_audio_upload_enable = 1;
            } else if (state == "sentence_start") {
                // 取出"text", 通知GUI
                // {"type":"tts","state":"sentence_start","text":"1加1等于2啦~","session_id":"eae53ada"}
                auto text = j["text"];
                send_stt(text.get<std::string>());
                //send_start_listening_req(kListeningModeAlwaysOn);
                set_device_state(kDeviceStateSpeaking);
                send_device_state();
            }
        } else if (j["type"] == "stt") {
            // 表示服务器端识别到了用户语音, 取出"text", 通知GUI
            auto text = j["text"];
            send_stt(text.get<std::string>());
        } else if (j["type"] == "llm") {
            // 有"happy"等取值
        /*
            "neutral",
            "happy",
            "laughing",
            "funny",
            "sad",
            "angry",
            "crying",
            "loving",
            "embarrassed",
            "surprised",
            "shocked",
            "thinking",
            "winking",
            "cool",
            "relaxed",
            "delicious",
            "kissy",
            "confident",
            "sleepy",
            "silly",
            "confused"
        */
            auto emotion = j["emotion"];
        } else if (j["type"] == "iot") {

        }
    } catch (json::parse_error& e) {
        std::cout << "Failed to parse JSON message: " << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cout << "Error processing message: " << e.what() << std::endl;
    }
}

static void process_txt_data_downloaded(const char *buffer, size_t size)
{
    try {
        // Parse the JSON message
        json j = json::parse(buffer);

        // Check if the message matches the expected structure
        if (j.contains("type") && j["type"] == "hello") {
            process_hello_json(buffer, size);
        } else {
            process_other_json(buffer, size);
        }

    } catch (json::parse_error& e) {
        std::cout << "Failed to parse JSON message: " << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cout << "Error processing message: " << e.what() << std::endl;
    }
}

static void ws_work_state_cb(bool work_state)
{
    if (g_ws_state_callback != NULL) {
        g_ws_state_callback(work_state);
    }
}

int  send_audio(const unsigned char *buffer, int size, void *user_data)
{
    if (!g_websocket_start)
    {
        return 0;
    }

    if (g_speech_interaction_mode == kSpeechInteractionModeAuto || g_speech_interaction_mode == kSpeechInteractionModeManual || g_speech_interaction_mode == kSpeechInteractionModeAutoWithWakeupWord) {
        if (g_audio_upload_enable) {
            websocket_send_binary((const char *)buffer, size);
            // static int cnt = 0;
            // if ((cnt++ % 100) == 0)
            //     std::cout << "Send opus data to server: " << size <<" count: "<< cnt << std::endl;
        }
    }
    else if (g_speech_interaction_mode == kSpeechInteractionModeRealtime) {
        // 直接发送数据到服务器
        websocket_send_binary((const char *)buffer, size);
        // static int cnt = 0;
        // if ((cnt++ % 100) == 0)
        //     std::cout << "Send opus data to server: " << size <<" count: "<< cnt << std::endl;
    }

    return 0;
}

int process_ui_data(char *buffer, size_t size, void *user_data)
{
    return 0;
}

/**
 * 从配置文件中读取 UUID
 *
 * 该函数尝试从 /etc/xiaozhi.cfg 文件中读取 UUID。
 * 如果文件存在且包含有效的 UUID，则返回该 UUID。
 * 否则，返回空字符串。
 *
 * @return 从配置文件中读取的 UUID，如果未找到则返回空字符串
 */
std::string read_uuid_from_config() {
    std::ifstream config_file(CFG_FILE);
    if (!config_file.is_open()) {
        std::cerr << "Failed to open " CFG_FILE " for reading" << std::endl;
        return "";
    }

    try {
        json config_json;
        config_file >> config_json;
        config_file.close();

        if (config_json.contains("uuid")) {
            return config_json["uuid"].get<std::string>();
        }
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse " CFG_FILE ": " << e.what() << std::endl;
    }

    return "";
}

/**
 * 将 UUID 写入配置文件
 *
 * 该函数将给定的 UUID 写入 /etc/xiaozhi.cfg 文件。
 * 如果文件不存在，则创建新文件。
 *
 * @param uuid 要写入配置文件的 UUID
 * @return 成功写入文件返回 true，否则返回 false
 */
bool write_uuid_to_config(const std::string& uuid) {
    std::ofstream config_file(CFG_FILE);
    if (!config_file.is_open()) {
        std::cerr << "Failed to open " CFG_FILE " for writing" << std::endl;
        return false;
    }

    try {
        json config_json;
        config_json["uuid"] = uuid;
        config_file << config_json.dump(4); // 使用 4 个空格进行缩进
        config_file.close();
        return true;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to write to " CFG_FILE ": " << e.what() << std::endl;
    }

    return false;
}

bool is_webtts_config_exists() {
    std::ifstream config_file(CFG_FILE);
    if (!config_file.is_open()) {
        return false;
    }

    try {
        json config_json;
        config_file >> config_json;
        return config_json.contains("webtts");
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse " CFG_FILE ": " << e.what() << std::endl;
        return false;
    }
}


bool write_webtts_to_config(const WebttsConfig& config) {
    // 先读取现有配置
    json config_json;
    {
        std::ifstream existing_file(CFG_FILE);
        if (existing_file.is_open()) {
            try {
                existing_file >> config_json;
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "Failed to parse existing " CFG_FILE ": " << e.what() << std::endl;
                // 继续执行，将创建新配置
            }
        }
    }

    // 确保webtts节点存在
    if (!config_json.contains("webtts")) {
        config_json["webtts"] = json::object();
    }

    // 更新现有参数（保留其他现有参数）
    auto& webtts = config_json["webtts"];
    webtts["app_id"] = config.app_id;
    webtts["api_key"] = config.api_key;
    webtts["api_secret"] = config.api_secret;
    webtts["sample_rate"] = config.sample_rate;
    webtts["voice_name"] = config.voice_name;
    webtts["speed"] = config.speed;

    // 写入更新后的配置
    std::ofstream config_file(CFG_FILE);
    if (!config_file.is_open()) {
        std::cerr << "Failed to open " CFG_FILE " for writing" << std::endl;
        return false;
    }

    try {
        config_file << config_json.dump(4);
        config_file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to write to " CFG_FILE ": " << e.what() << std::endl;
    }

    return false;
}

bool read_webtts_from_config(WebttsConfig& config) {
    std::ifstream config_file(CFG_FILE);
    if (!config_file.is_open()) {
        std::cerr << "Failed to open " CFG_FILE " for reading" << std::endl;
        return false;
    }

    try {
        json config_json;
        config_file >> config_json;
        config_file.close();

        // 检查配置文件中是否存在 webtts 字段
        if (config_json.contains("webtts") && config_json["webtts"].is_object()) {
            const auto& webtts = config_json["webtts"];

            // 使用默认值处理可能缺失的字段
            config.app_id = webtts.value("app_id", "");
            config.api_key = webtts.value("api_key", "");
            config.api_secret = webtts.value("api_secret", "");
            config.sample_rate = webtts.value("sample_rate", 16000);
            config.voice_name = webtts.value("voice_name", "xiaoyan");
            config.speed = webtts.value("speed", 50);

            return true;
        } else {
            std::cerr << "Missing 'webtts' object in " CFG_FILE << std::endl;
        }
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse " CFG_FILE ": " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error reading " CFG_FILE ": " << e.what() << std::endl;
    }

    return false;
}

bool is_xiaozhi_config_exists() {
    std::ifstream config_file(CFG_FILE);
    if (!config_file.is_open()) {
        return false;
    }

    try {
        json config_json;
        config_file >> config_json;
        return config_json.contains("xiaozhi");
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse " CFG_FILE ": " << e.what() << std::endl;
        return false;
    }
}

bool write_xiaozhi_to_config(std::string ota_url, std::string ws_addr) {
    // 先读取现有配置
    json config_json;
    {
        std::ifstream existing_file(CFG_FILE);
        if (existing_file.is_open()) {
            try {
                existing_file >> config_json;
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "Failed to parse existing " CFG_FILE ": " << e.what() << std::endl;
                // 继续执行，将创建新配置
            }
        }
    }

    // 确保xiaozhi节点存在
    if (!config_json.contains("xiaozhi")) {
        config_json["xiaozhi"] = json::object();
    }

    // 更新现有参数（保留其他现有参数）
    auto& webtts = config_json["xiaozhi"];
    webtts["xiaozhi_ota_url"] = ota_url.c_str();
    webtts["xiaozhi_ws_addr"] = ws_addr.c_str();

    // 写入更新后的配置
    std::ofstream config_file(CFG_FILE);
    if (!config_file.is_open()) {
        std::cerr << "Failed to open " CFG_FILE " for writing" << std::endl;
        return false;
    }

    try {
        config_file << config_json.dump(4);
        config_file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to write to " CFG_FILE ": " << e.what() << std::endl;
    }

    return false;
}

bool read_xiaozhi_from_config(std::string& ota_url, std::string& ws_addr) {
    std::ifstream config_file(CFG_FILE);
    if (!config_file.is_open()) {
        std::cerr << "Failed to open " CFG_FILE " for reading" << std::endl;
        return false;
    }

    try {
        json config_json;
        config_file >> config_json;
        config_file.close();

        // 检查配置文件中是否存在 xiaozhi 字段
        if (config_json.contains("xiaozhi") && config_json["xiaozhi"].is_object()) {
            const auto& xiaozhi = config_json["xiaozhi"];

            // 使用默认值处理可能缺失的字段
            ota_url = xiaozhi.value("xiaozhi_ota_url", "");
            ws_addr = xiaozhi.value("xiaozhi_ws_addr", "");

            return true;
        } else {
            std::cerr << "Missing 'xiaozhi' object in " CFG_FILE << std::endl;
        }
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse " CFG_FILE ": " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error reading " CFG_FILE ": " << e.what() << std::endl;
    }

    return false;
}

int  active_device()
{
    char active_code[20] = "";
    static http_data_t http_data;
    // http_data.url = "https://api.tenclass.net/xiaozhi/ota/";
    http_data.url = g_ota_url;

    // 替换 http_data.post 中的 uuid
    std::ostringstream post_stream;
    post_stream << R"(
        {
            "uuid":")" << g_uuid << R"(",
            "application": {
                "name": "xiaozhi_linux_k230",
                "version": "1.0.0"
            },
            "ota": {
            },
            "board": {
                "type": "k230_linux_board",
                "name": "k230_linux_board"
            }
        }
    )";
    http_data.post = post_stream.str();

    // 替换 http_data.headers 中的 Device-Id
    std::ostringstream headers_stream;
    headers_stream << R"(
        {
            "Content-Type": "application/json",
            "Device-Id": ")" << g_mac << R"(",
            "User-Agent": "canaan",
            "Accept-Language": "zh-CN"
        }
    )";
    http_data.headers = headers_stream.str();

    while (0 != active_device(&http_data, active_code)) {
        std::cout << "active_device failed" << std::endl;
        if (active_code[0]) {
            std::string auth_code = "Active-Code: " + std::string(active_code);
            set_device_state(kDeviceStateActivating);
            send_device_state();
            send_stt(auth_code);
        }
        sleep(5);
    }

    set_device_state(kDeviceStateIdle);
    send_device_state();
    send_stt("设备已经激活");

    return 0;
}

static void parseWebSocketUrl(const std::string& url, websocket_data_t& data) {
    // 解析协议部分
    size_t protocolPos = url.find("://");
    if (protocolPos != std::string::npos) {
        data.protocol = url.substr(0, protocolPos + 3);
        size_t remainingPos = protocolPos + 3;

        // 解析主机名
        size_t hostEndPos = url.find(':', remainingPos);
        size_t pathStartPos = url.find('/', remainingPos);

        if (hostEndPos != std::string::npos && hostEndPos < pathStartPos) {
            // 有明确的端口号
            data.hostname = url.substr(remainingPos, hostEndPos - remainingPos);

            // 解析端口
            size_t portEndPos = url.find('/', hostEndPos);
            if (portEndPos != std::string::npos) {
                data.port = url.substr(hostEndPos + 1, portEndPos - hostEndPos - 1);
                data.path = url.substr(portEndPos);
            } else {
                data.port = url.substr(hostEndPos + 1);
                data.path = "/";
            }
        } else {
            // 使用默认端口
            data.hostname = url.substr(remainingPos, pathStartPos - remainingPos);
            data.port = (data.protocol == "wss://") ? "443" : "80";

            // 解析路径
            if (pathStartPos != std::string::npos) {
                data.path = url.substr(pathStartPos);
            } else {
                data.path = "/";
            }
        }
    }
}

int  init_device(server_audio_data_callback callback,audio_tts_state_callback tts_callback,ws_work_state_callback ws_state_callback)
{
    g_callback = callback;
    g_tts_callback = tts_callback;
    g_ws_state_callback = ws_state_callback;

    // 获取无线网卡的 MAC 地址
    std::string mac = get_wireless_mac_address();
    if (mac.empty()) {
        std::cerr << "Failed to get wireless MAC address" << std::endl;
        mac = "00:00:00:00:00:00"; // 默认值，可以根据需要修改
    }

    // 读取配置文件中的 UUID
    std::string uuid = read_uuid_from_config();
    if (uuid.empty()) {
        std::cerr << "UUID not found in " CFG_FILE << std::endl;
        // 生成新的 UUID
        uuid = generate_uuid();
        std::cout << "Generated new UUID: " << uuid << std::endl;

        // 将新的 UUID 写入配置文件
        if (!write_uuid_to_config(uuid)) {
            std::cerr << "Failed to write UUID to " CFG_FILE << std::endl;
        } else {
            std::cout << "UUID written to " CFG_FILE << std::endl;
        }
    } else {
        std::cout << "UUID from " CFG_FILE ": " << uuid << std::endl;
    }

    // 读取配置文件中的 WebTTS 配置
    if (!is_webtts_config_exists()) {
        std::cerr << "WebTTS config not found in " CFG_FILE << std::endl;
        // 创建默认配置
        WebttsConfig default_config = create_default_webtts_config();
        // 将默认配置写入配置文件
        if (!write_webtts_to_config(default_config)) {
            std::cerr << "Failed to write WebTTS config to " CFG_FILE << std::endl;
        } else {
            std::cout << "WebTTS config written to " CFG_FILE << std::endl;
        }
    }

    if (!read_webtts_from_config(g_webtts_config)) {
        std::cerr << "Failed to read WebTTS config from " CFG_FILE << std::endl;
        return -1;
    }

    // 读取配置文件中的 xiaozhi配置
    if (!is_xiaozhi_config_exists()) {
        std::cerr << "xiaozhi config not found in " CFG_FILE << std::endl;
        // 创建默认配置
        create_default_xiaozhi_config(g_ota_url, g_ws_addr);
        // 将默认配置写入配置文件
        if (!write_xiaozhi_to_config(g_ota_url, g_ws_addr)) {
            std::cerr << "Failed to write xiaozhi config to " CFG_FILE << std::endl;
        } else {
            std::cout << "xiaozhi config written to " CFG_FILE << std::endl;
        }
    }

    if (!read_xiaozhi_from_config(g_ota_url, g_ws_addr)) {
        std::cerr << "Failed to read xiaozhi config from " CFG_FILE << std::endl;
        return -1;
    }

    g_uuid = uuid;
    g_mac = mac;

    g_ipc_ep_ui = ipc_endpoint_create_udp(UI_PORT_UP, UI_PORT_DOWN, process_ui_data, NULL);

    // 替换 g_ws_data.headers 中的 Device-Id 和 Client-Id
    std::ostringstream ws_headers_stream;
    ws_headers_stream << R"(
        {
            "Authorization": "Bearer test-token",
            "Protocol-Version": "1",
            "Device-Id": ")" << g_mac << R"(",
            "Client-Id": ")" << g_uuid << R"("
        }
    )";
    g_ws_data.headers = ws_headers_stream.str();

    g_ws_data.hello = R"(
        {
            "type": "hello",
            "version": 1,
            "transport": "websocket",
            "audio_params": {
                "format": "opus",
                "sample_rate": 16000,
                "channels": 1,
                "frame_duration": 60
            }
        })";


    // g_ws_data.hostname = "api.tenclass.net";
    // g_ws_data.port = "443";
    // g_ws_data.path = "/xiaozhi/v1/";

    parseWebSocketUrl(g_ws_addr, g_ws_data);
    printf("ws_addr:%s\n", g_ws_addr.c_str());
    printf("ws_hostname:%s\n", g_ws_data.hostname.c_str());
    printf("ws_port:%s\n", g_ws_data.port.c_str());
    printf("ws_path:%s\n", g_ws_data.path.c_str());



    websocket_set_callbacks(process_opus_data_downloaded, process_txt_data_downloaded, &g_ws_data,ws_work_state_cb);
    return 0;
}

int  connect_to_xiaozhi_server()
{
    if (g_websocket_start == 0) {
        websocket_start();
        //g_websocket_start = 1;
    }

    return 0;
}

int  abort_cur_session()
{
    if (!g_websocket_start)
    {
        return -1;
    }

    send_abort_req();
    return 0;
}



