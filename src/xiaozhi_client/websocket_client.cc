// SPDX-License-Identifier: GPL-3.0-only
/*
 * Copyright (c) 2008-2023 100askTeam : Dongshan WEI <weidongshan@100ask.net>
 * Discourse:  https://forums.100ask.net
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
 *          版权所有，盗版必究。
 *
 * 修改历史     版本号           作者        修改内容
 *-----------------------------------------------------
 * 2025.03.20      v01         百问科技      创建文件
 *-----------------------------------------------------
 */

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <pthread.h>
#include <atomic>
#include <list>
using namespace std;

// Include nlohmann/json library
#include "json.hpp"
#include "websocket_client.h"
#include "control_center.h"

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_enable_debug = 0;
static int g_connection_retry_count = 0;
static const int g_max_retry_count = 5;
static const int g_retry_delay_ms = 2000;

extern int g_websocket_start;

static std::atomic<bool> g_websocket_send_enable(false);
static std::thread g_ws_send_thread;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::placeholders::_3;
using websocketpp::lib::placeholders::_4;
using websocketpp::lib::bind;

using json = nlohmann::json;

static client* g_p_ws_client = NULL;
static websocketpp::connection_hdl g_hdl;
static websocket_data_t *g_ws_data;
static ws_recv_callback_t g_ws_recv_bin_cb;
static ws_recv_callback_t g_ws_recv_txt_cb;
static ws_work_state_callback_t g_ws_work_state_cb;
static volatile int g_iHasShaked = 0;
static volatile int g_iHasConnected = 0;

extern SpeechInteractionMode g_speech_interaction_mode;
/**
 * 处理接收到的消息
 *
 * @param c 指向WebSocket客户端的指针
 * @param hdl 连接句柄
 * @param msg 接收到的消息
 */
static void on_message(client *c, websocketpp::connection_hdl hdl, client::message_ptr msg) {

    // 获取操作码
    auto opcode = msg->get_opcode();
    //std::cout << "Received opcode: " << opcode << std::endl;
    std::string payload = msg->get_payload();

    if (opcode == websocketpp::frame::opcode::binary)
    {
        // 处理opus数据:msg->get_payload();
        g_ws_recv_bin_cb(payload.data(), payload.size());
        return;
    }

    if (g_enable_debug)
    {
        std::cout << "Received: " << payload << std::endl;
    }

    try {
        // 处理json数据:msg->get_payload();
        g_ws_recv_txt_cb(payload.data(), payload.size());

    } catch (json::parse_error& e) {
        std::cout << "Failed to parse JSON message: " << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cout << "Error processing message: " << e.what() << std::endl;
    }
}

/**
 * 验证证书的主体备用名称是否匹配给定的主机名
 *
 * @param hostname 主机名
 * @param cert 证书
 * @return 如果匹配则返回true，否则返回false
 */
static bool verify_subject_alternative_name(const char * hostname, X509 * cert) {
    STACK_OF(GENERAL_NAME) * san_names = NULL;

    san_names = (STACK_OF(GENERAL_NAME) *) X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
    if (san_names == NULL) {
        return false;
    }

    int san_names_count = sk_GENERAL_NAME_num(san_names);

    bool result = false;

    for (int i = 0; i < san_names_count; i++) {
        const GENERAL_NAME * current_name = sk_GENERAL_NAME_value(san_names, i);

        if (current_name->type != GEN_DNS) {
            continue;
        }

        char const * dns_name = (char const *) ASN1_STRING_get0_data(current_name->d.dNSName);

        // 确保DNS名称中没有嵌入的NUL字符
        if (ASN1_STRING_length(current_name->d.dNSName) != strlen(dns_name)) {
            break;
        }
        // 比较预期的主机名与CN
        result = (strcasecmp(hostname, dns_name) == 0);
    }
    sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);

    return result;
}

/**
 * 验证证书的通用名称是否匹配给定的主机名
 *
 * @param hostname 主机名
 * @param cert 证书
 * @return 如果匹配则返回true，否则返回false
 */
static bool verify_common_name(char const * hostname, X509 * cert) {
    // 查找证书主题字段中的CN字段
    int common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name(cert), NID_commonName, -1);
    if (common_name_loc < 0) {
        return false;
    }

    // 提取CN字段
    X509_NAME_ENTRY * common_name_entry = X509_NAME_get_entry(X509_get_subject_name(cert), common_name_loc);
    if (common_name_entry == NULL) {
        return false;
    }

    // 将CN字段转换为C字符串
    ASN1_STRING * common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
    if (common_name_asn1 == NULL) {
        return false;
    }

    char const * common_name_str = (char const *) ASN1_STRING_get0_data(common_name_asn1);

    // 确保CN中没有嵌入的NUL字符
    if (ASN1_STRING_length(common_name_asn1) != strlen(common_name_str)) {
        return false;
    }

    // 比较预期的主机名与CN
    return (strcasecmp(hostname, common_name_str) == 0);
}

/**
 * 验证证书是否有效
 *
 * @param hostname 主机名
 * @param preverified 预验证结果
 * @param ctx 验证上下文
 * @return 如果验证通过则返回true，否则返回false
 */
static bool verify_certificate(const char * hostname, bool preverified, boost::asio::ssl::verify_context& ctx) {
    // 验证回调用于检查是否证书有效
    // RFC 2818 描述了如何为HTTPS执行此操作
    // 参考OpenSSL文档以获取更多详细信息

    // 获取当前证书在证书链中的深度
    int depth = X509_STORE_CTX_get_error_depth(ctx.native_handle());

    // 如果是最终证书且预验证通过，则确保主机名匹配SAN或CN
    if (depth == 0 && preverified) {
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

        if (verify_subject_alternative_name(hostname, cert)) {
            return true;
        } else if (verify_common_name(hostname, cert)) {
            return true;
        } else {
            return false;
        }
    }

    return preverified;
}

/**
 * TLS初始化处理程序
 *
 * @param hostname 主机名
 * @param hdl 连接句柄
 * @return TLS上下文指针
 */
static context_ptr on_tls_init(const char * hostname, websocketpp::connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);

        // 设置验证模式为不验证（用于测试环境），避免证书验证问题
        ctx->set_verify_mode(boost::asio::ssl::verify_none);
        
        // 设置默认验证路径
        ctx->set_default_verify_paths();
        
        // 设置SSL选项以提高兼容性
        ctx->set_options(boost::asio::ssl::context::no_compression);
        
    } catch (std::exception& e) {
        std::cout << "TLS初始化错误: " << e.what() << std::endl;
    }
    return ctx;
}

/**
 * 处理连接打开事件
 *
 * @param c 指向WebSocket客户端的指针
 * @param hdl 连接句柄
 */
static void on_open(client* c, websocketpp::connection_hdl hdl) {
    g_hdl = hdl;
    g_iHasConnected = 1;
    websocket_data_t *ws_data = g_ws_data;

    if (g_enable_debug)
        std::cout << "=========on_open==========" << std::endl;

#if 0
     std::string hello = R"(
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
#endif
    std::string hello = ws_data->hello;

    try {
        c->send(hdl, hello, websocketpp::frame::opcode::text);
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Error sending message: " << e << " (" << e.message() << ")" << std::endl;
    }

    g_websocket_start = 1;

    if (g_enable_debug)
        printf("""========websocket connected=========\n");

}

/**
 * 处理连接关闭事件
 *
 * @param c 指向WebSocket客户端的指针
 * @param hdl 连接句柄
 * @param ec 错误代码
 * @param reason 关闭原因
 */
static void on_close(client *c, websocketpp::connection_hdl hdl) {
    //std::cout << "=========on_close==========" << std::endl;
    g_websocket_start = 0;
    g_iHasConnected = 0;
    g_iHasShaked = 0;
    client::connection_ptr con = c->get_con_from_hdl(hdl);

    if (g_enable_debug)
        std::cout << "Connection closed. Code: " << con->get_remote_close_code() << ", Reason: " << con->get_remote_close_reason() << "!!" << std::endl;

    // 重置重试计数，为下次连接做准备
    g_connection_retry_count = 0;
}

/**
 * 建立WebSocket连接
 *
 * @param c 指向WebSocket客户端的指针
 * @return 错误码
 */
static int websocket_connect(client *c) {
    websocket_data_t *ws_data = g_ws_data;

    std::string hostname = ws_data->hostname;
    std::string port = ws_data->port;
    std::string path = ws_data->path;
    std::string protocol = ws_data->protocol;

    std::string uri = protocol + hostname + ":" + port + path;
    printf("uri = %s\n", uri.c_str());

    if (g_enable_debug)
        std::cout << "Connecting to " << uri << " (attempt " << (g_connection_retry_count + 1) << ")" << std::endl;

    try {
        // 设置日志级别
        c->clear_access_channels(websocketpp::log::alevel::all); // 禁用所有访问日志
        c->set_access_channels(websocketpp::log::alevel::app); // 仅启用应用程序日志
        c->set_error_channels(websocketpp::log::elevel::all); // 启用所有错误日志

        // 初始化ASIO
        c->init_asio();

        // 设置连接超时
        c->set_open_handshake_timeout(15000); // 15秒握手超时
        c->set_close_handshake_timeout(5000);  // 5秒关闭超时
        c->set_ping_interval(30000);           // 30秒ping间隔
        c->set_ping_timeout(10000);            // 10秒ping超时

        // 注册消息处理程序
        c->set_message_handler(bind(&on_message, c, ::_1, ::_2));
        c->set_tls_init_handler(bind(&on_tls_init, hostname.c_str(), ::_1));

        // 注册连接打开处理程序
        c->set_open_handler(bind(&on_open, c, ::_1));

        // 注册连接关闭处理程序
        c->set_close_handler(bind(&on_close, c, ::_1));

        // 添加失败处理器
        c->set_fail_handler([&](websocketpp::connection_hdl hdl) {
            client::connection_ptr con = c->get_con_from_hdl(hdl);
            std::cout << "连接失败，原因: " << con->get_ec().message() 
                      << " (状态码: " << con->get_response_code() << ")" << std::endl;
        });

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c->get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return -1;
        }

        // 设置自定义HTTP头
        std::string headers = ws_data->headers;
        if (g_enable_debug)
            std::cout << "HTTP headers: " << headers << std::endl;
        try {
            // 解析headers字符串为JSON对象
            json headers_json = json::parse(headers);

            // 遍历JSON对象中的每个键值对
            for (json::iterator it = headers_json.begin(); it != headers_json.end(); ++it) {
                std::string key = it.key();
                std::string value = it.value();
                if (g_enable_debug)
                    std::cout << "Key: " << key << ", Value: " << value << std::endl;
                con->append_header(key, value);
            }
        } catch (json::parse_error& e) {
            std::cout << "Failed to parse headers JSON: " << e.what() << std::endl;
        }
#if 0
        con->append_header("Authorization", "Bearer test-token");
        con->append_header("Protocol-Version", "1");
        con->append_header("Device-Id", "00:0c:29:bd:43:05");
        con->append_header("Client-Id", "d560294c-01d9-47d0-b538-085f38744b05");
#endif

        // 请求连接
        c->connect(con);

        c->get_alog().write(websocketpp::log::alevel::app, "Connecting to " + uri);

    } catch (websocketpp::exception const & e) {
        std::cout << "WebSocket连接异常: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

/**
 * WebSocket线程函数
 *
 * @param arg 参数指针
 * @return 线程返回值
 */
static void *websocket_thread(void *arg) {
    client *c = (client *)arg;
    bool connection_successful = false;

    // 重试连接逻辑
    while (g_connection_retry_count < g_max_retry_count && !connection_successful) {
        try {
            if (websocket_connect(c) != 0) {
                g_connection_retry_count++;
                std::cout << "连接失败，重试 " << g_connection_retry_count << "/" << g_max_retry_count 
                          << "，等待 " << g_retry_delay_ms << "ms 后重试..." << std::endl;
                
                if (g_connection_retry_count < g_max_retry_count) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(g_retry_delay_ms));
                    continue;
                } else {
                    std::cout << "达到最大重试次数，连接失败" << std::endl;
                    break;
                }
            }

            // 连接成功，启动ASIO io_service运行循环
            c->run();
            c->stop();
            connection_successful = true;
            g_connection_retry_count = 0; // 重置重试计数

        } catch (websocketpp::exception const & e) {
            g_connection_retry_count++;
            std::cout << "WebSocket异常: " << e.what() << "，重试 " << g_connection_retry_count 
                      << "/" << g_max_retry_count << std::endl;
            
            if (g_connection_retry_count < g_max_retry_count) {
                std::this_thread::sleep_for(std::chrono::milliseconds(g_retry_delay_ms));
            } else {
                std::cout << "达到最大重试次数，WebSocket线程退出" << std::endl;
                break;
            }
        }
    }

    delete c;
    std::cout << "WebSocket线程退出" << std::endl;

    if (g_ws_work_state_cb != NULL) {
        g_ws_work_state_cb(false);
    }

    if (g_speech_interaction_mode != kSpeechInteractionModeAutoWithWakeupWord) {
        // 如果连接失败，等待一段时间后重新启动WebSocket
        if (!connection_successful) {
            std::cout << "等待30秒后重新启动WebSocket..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(30));
            websocket_start();
        }
    }

    return NULL;
}



static int websocket_send_binary_ex(const char *data, int size) {
    client* c = g_p_ws_client;
    websocketpp::connection_hdl hdl = g_hdl;

    static int cnt = 0;
    // 发送二进制数据
    if (g_iHasConnected && g_iHasShaked){
        //printf("send data cnt  = %d, size = %d\n", cnt++, size);
        try {
            c->send(hdl, data, size, websocketpp::frame::opcode::binary);
        } catch (websocketpp::exception const & e) {
            // std::cout << "exit in websocket_send_binary: " << e.what() << std::endl;
            // websocket_connect(c);
            ;
        }
    }

    return 0;
}


static int websocket_send_text_ex(const char *data, int size) {
    client* c = g_p_ws_client;
    websocketpp::connection_hdl hdl = g_hdl;

    // 发送文本数据
    if (g_iHasConnected){
        try {
            c->send(hdl, data, size, websocketpp::frame::opcode::text);
        } catch (websocketpp::exception const & e) {
            // std::cout << "exit in websocket_send_text: " << e.what() << std::endl;
            // websocket_connect(c);
            ;
        }
        g_iHasShaked = 1;
    }
    return 0;
}



typedef struct tag_client_data
{
    char data_buffer[16000];
    int data_size;
    int data_type;
} client_data_t;

typedef std::list<client_data_t> CLIENT_DATA_LIST;
static CLIENT_DATA_LIST g_client_data_list;
static void *websocket_send_thread(void *arg)
{
    client *c = (client *)arg;

    while(g_websocket_send_enable)
    {
        pthread_mutex_lock(&g_mutex);
        if (g_client_data_list.size() > 0)
        {
            client_data_t data = g_client_data_list.front();
            g_client_data_list.pop_front();
            pthread_mutex_unlock(&g_mutex);

            if (data.data_type == 1)
            {
                websocket_send_binary_ex(data.data_buffer, data.data_size);
            }
            else
            {
                websocket_send_text_ex(data.data_buffer, data.data_size);
            }
        }
        else
        {
            pthread_mutex_unlock(&g_mutex);
            usleep(1000 * 10); // 10ms
        }
    }
    return NULL;
}




/**
 * 发送二进制数据
 *
 * @param data 数据指针
 * @param size 数据大小
 * @return 错误码
 */
int websocket_send_binary(const char *data, int size) {

    client_data_t client_data;
    client_data.data_size = size;
    client_data.data_type = 1; // 二进制数据
    memcpy(client_data.data_buffer, data, size);

    pthread_mutex_lock(&g_mutex);
    g_client_data_list.push_back(client_data);
    pthread_mutex_unlock(&g_mutex);

    return 0;
}

/**
 * 发送文本数据
 *
 * @param data 数据指针
 * @param size 数据大小
 * @return 错误码
 */
int websocket_send_text(const char *data, int size) {

    client_data_t client_data;
    client_data.data_size = size;
    client_data.data_type = 0; // 文本数据
    memcpy(client_data.data_buffer, data, size);

    pthread_mutex_lock(&g_mutex);
    g_client_data_list.push_back(client_data);
    pthread_mutex_unlock(&g_mutex);

    return 0;
}

/**
 * 设置回调函数和数据
 *
 * @param bin_cb 二进制数据接收回调
 * @param txt_cb 文本数据接收回调
 * @param ws_data WebSocket数据
 * @return 返回值
 */
int websocket_set_callbacks(ws_recv_callback_t bin_cb, ws_recv_callback_t txt_cb, websocket_data_t *ws_data,ws_work_state_callback_t ws_state_cb) {
    g_ws_recv_bin_cb = bin_cb;
    g_ws_recv_txt_cb = txt_cb;
    g_ws_work_state_cb = ws_state_cb;
    g_ws_data = ws_data;
    return 0;
}

/**
 * 启动WebSocket线程
 *
 * @return 返回值
 */
int websocket_start() {
    g_websocket_send_enable = 0;
    if (g_ws_send_thread.joinable()) {
        g_ws_send_thread.join();
    }

    g_p_ws_client = new client();
    // 创建并启动线程 websocket_thread
    std::thread ws_thread(websocket_thread, g_p_ws_client);

    // 创建并启动线程 websocket_send_thread
    g_websocket_send_enable = 1;
    g_ws_send_thread = std::thread(websocket_send_thread, g_p_ws_client);


    // 将线程分离，使其在后台运行
    ws_thread.detach();

    return 0;
}