/**
 * @file message_queue.h
 *
 */

 #ifndef MESSAGE_QUEUE_H
 #define MESSAGE_QUEUE_H
 
 #ifdef __cplusplus
 extern "C" {
 #endif

 typedef struct {
    int cmd;
    char* registered_name;
} ui_cmd_message_t;

typedef struct {
    int data;
    int success;
    char* error_message;
} ai_cmd_message_t;

 class MessageQueue
 {
 public: 
    //发送人脸识运行指令
    static MessageQueue* getInstance();
    int SendFaceRecRunCmd(void);
    //发送人脸注册指令
    int SendRegRegisterCmd(const char* register_name, char*& error_message);
    //获取人脸数量
    int SendRegRegisterNumCmd(void);
    //清空数据库
    int SendClearDataBaseCmd(void);
    //停止人脸识别
    int SendFaceRecStopCmd(void);
    int SendRegImgDropCmd(void);
    int SendGetRegImgCmd(void);
    bool AIQueryCmd(ui_cmd_message_t *message);
    bool UIQueryCmd(ai_cmd_message_t *message);
    void SendAIMsg(ai_cmd_message_t* message);

 private:
    static MessageQueue* instance;
    static std::mutex mutex_;
        /**
     * @brief MessageQueue构造函数
     * @return None
     */
    MessageQueue();
    
    /**
     * @brief MessageQueue析构函数
     * @return None
     */
    ~MessageQueue();

 };

#ifdef __cplusplus
}
#endif

#endif