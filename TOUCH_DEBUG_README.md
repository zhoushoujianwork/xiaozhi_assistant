# 触摸触点打印Debug功能

## 功能概述

本功能为小智助手项目添加了完整的触摸触点debug打印功能，可以在触摸屏幕时实时打印触摸事件的详细信息，帮助开发者调试触摸相关问题。

## 功能特性

### 1. 多层次Debug输出
- **硬件层**: 打印原始触摸事件 (ABS_X, ABS_Y, BTN_TOUCH等)
- **驱动层**: 打印处理后的坐标和状态转换
- **UI层**: 打印LVGL触摸事件 (PRESSED, RELEASED, CLICKED等)

### 2. 条件编译控制
- 通过CMake选项 `ENABLE_TOUCH_DEBUG` 控制是否启用debug输出
- 默认启用，可以通过编译时参数关闭
- 关闭时不会影响程序性能

### 3. 详细的坐标信息
- 显示原始坐标和处理后坐标
- 显示触摸状态 (PRESSED/RELEASED)
- 显示触摸事件类型 (DOWN/UP/CLICKED等)

## 使用方法

### 启用触摸Debug
```bash
# 方法1: 使用测试脚本 (推荐)
./test_touch_debug.sh enable

# 方法2: 手动编译
cd src/ui_and_ai
cmake -DENABLE_TOUCH_DEBUG=ON -B build
make -C build
```

### 禁用触摸Debug
```bash
# 方法1: 使用测试脚本
./test_touch_debug.sh disable

# 方法2: 手动编译
cd src/ui_and_ai
cmake -DENABLE_TOUCH_DEBUG=OFF -B build
make -C build
```

## Debug输出示例

当触摸屏幕时，会看到类似以下的debug输出：

```
[TOUCH_DEBUG] Touch device initialized and event listener added
[TOUCH_DEBUG] BTN_TOUCH DOWN
[TOUCH_DEBUG] ABS_X: 240
[TOUCH_DEBUG] ABS_Y: 400
[TOUCH_DEBUG] TOUCH_DOWN - Tracking ID: 0
[TOUCH_DEBUG] POINTER EVENT - Raw: (240, 400) -> Processed: (240, 400) State: PRESSED
[TOUCH_DEBUG] UI_EVENT: PRESSED at (240, 400)
[TOUCH_DEBUG] UI_EVENT: CLICKED at (240, 400)
[TOUCH_DEBUG] UI_EVENT: RELEASED at (240, 400)
[TOUCH_DEBUG] TOUCH_UP - Tracking ID: -1
[TOUCH_DEBUG] BTN_TOUCH UP
```

## 技术实现

### 修改的文件
1. `src/lvgl/src/drivers/evdev/lv_evdev.c` - 在evdev驱动中添加debug打印
2. `src/ui_and_ai/src/lvgl_ui.cc` - 在UI层添加触摸事件监听
3. `src/ui_and_ai/CMakeLists.txt` - 添加编译选项控制

### 关键代码位置
- **硬件事件处理**: `_evdev_read()` 函数中的 `EV_ABS` 和 `EV_KEY` 事件处理
- **坐标转换**: `_evdev_process_pointer()` 函数中的坐标处理
- **UI事件监听**: `touch_event_cb()` 回调函数

## 调试技巧

1. **坐标问题**: 查看 "Raw" 和 "Processed" 坐标是否合理
2. **状态问题**: 检查 PRESSED/RELEASED 状态是否正确
3. **事件丢失**: 观察是否有事件序列不完整
4. **性能问题**: 如果不需要debug，记得禁用以减少输出

## 注意事项

- Debug输出会占用控制台，建议在开发调试时使用
- 生产环境建议禁用debug输出
- 触摸坐标会根据屏幕旋转角度进行调整
- 支持多点触控，但LVGL只处理第一个触点

## 故障排除

如果触摸debug功能不工作：

1. 检查编译时是否启用了 `ENABLE_TOUCH_DEBUG`
2. 确认触摸设备路径 `/dev/input/event0` 是否正确
3. 检查是否有权限访问触摸设备
4. 查看系统日志中是否有相关错误信息
