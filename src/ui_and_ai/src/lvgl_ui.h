/**
 * @file lvgl_ui.h
 *
 */

 #ifndef LVGL_UI_H
 #define LVGL_UI_H
 
 #ifdef __cplusplus
 extern "C" {
 #endif

 class LvglUi
 {
 public:
     /**
      * @brief LvglUi构造函数
      * @return None
      */
     LvglUi();
     
     /**
      * @brief LvglUi析构函数
      * @return None
      */
     ~LvglUi();
     
      
     void create_ui_thread(void);
     void stop_ui_thread(void);
 private:
     std::thread ui_thread;
 };

#ifdef __cplusplus
}
#endif

#endif