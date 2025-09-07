# 小智语音助手 (XiaoZhi Assistant)

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform](https://img.shields.io/badge/Platform-K230%20RISC--V-green.svg)](https://www.canaan-creative.com/)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()

## 项目概述

小智语音助手是一个深度优化的嵌入式语音交互系统，专为**嘉楠K230开发板**设计。项目基于[78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)进行功能迁移和适配，融合本地AI能力与云端服务，提供自然流畅的语音交互体验。

### 🚀 技术亮点

- 🎙️ **双重唤醒机制**：集成K230本地语音唤醒+人脸识别唤醒
- 🖥️ **高性能GUI**：采用LVGL构建低延迟触控界面  
- 🌐 **混合架构**：本地AI处理+云端服务协同
- 🔊 **专业级TTS**：集成讯飞星火语音合成服务
- 🔧 **模块化设计**：支持独立编译和部署
- 📱 **触控交互**：完整的人脸管理界面

### 📋 系统要求

- **硬件平台**：嘉楠K230开发板
- **操作系统**：Linux (RISC-V 64位)
- **交叉编译工具链**：Xuantie-900-gcc-linux-6.6.0
- **依赖库**：LVGL, OpenCV, ALSA, Boost, WebSocketPP

### 📄 开源协议

本项目遵循 **GPL V3 开源协议**，确保代码开放性与可复用性。

## 🎯 核心功能

### 🎙️ 语音交互模块
- **实时数据传输**：通过WebSocket协议与服务器进行二进制Opus音频数据及JSON文本消息的实时传输
- **语音指令处理**：支持语音指令解析、响应生成及多轮对话逻辑
- **本地语音唤醒**：利用K230开发板实现本地语音唤醒功能，支持自定义唤醒词
- **语音合成**：集成讯飞星火语音合成技术，提供自然流畅的语音输出

### 🔐 设备激活模块  
- **HTTP协议激活**：基于HTTP协议实现设备激活，支持JSON格式的激活状态管理
- **设备认证**：确保设备能够安全接入云端服务系统

### 👤 人脸识别唤醒模块
- **实时识别**：利用K230开发板的AI算力，实现实时人脸识别功能
- **智能唤醒**：当识别到已注册的人脸时，自动唤醒语音助手
- **人脸管理**：支持人脸注册、识别、删除等完整管理功能

### 🖥️ 用户界面模块
- **LVGL图形库**：集成LVGL图形库，提供流畅的触控交互界面
- **人脸管理界面**：提供注册/识别/删除一体化的人脸管理界面
- **状态显示**：实时显示系统状态和交互反馈

## 📁 项目结构

```
xiaozhi_assistant/
├── src/                          # 源代码目录
│   ├── ui_and_ai/               # UI和AI模块
│   │   ├── src/                 # 核心源代码
│   │   │   ├── main.cc          # 主程序入口
│   │   │   ├── lvgl_ui.cc       # LVGL界面实现
│   │   │   ├── face_*.cc        # 人脸识别相关
│   │   │   ├── kws.cc           # 关键词检测
│   │   │   └── ...
│   │   ├── utils/               # 工具和资源文件
│   │   │   ├── *.kmodel         # AI模型文件
│   │   │   ├── *.ttf            # 字体文件
│   │   │   └── run.sh           # 运行脚本
│   │   └── CMakeLists.txt       # 构建配置
│   ├── xiaozhi_client/          # 语音客户端模块
│   │   ├── main.cc              # 客户端主程序
│   │   ├── websocket_client.cc  # WebSocket客户端
│   │   ├── snd_core/            # 音频处理核心
│   │   └── CMakeLists.txt       # 构建配置
│   └── common/                  # 公共代码
│       ├── ipc_udp.cc           # IPC通信
│       └── json.hpp             # JSON处理
├── build.sh                     # 构建脚本
├── README.md                    # 项目文档
└── k230_bin/                    # 编译输出目录
    ├── ui_and_ai                # UI可执行文件
    ├── xiaozhi_client           # 客户端可执行文件
    └── run.sh                   # 运行脚本
```

## ⚖️ 版权声明

### 主要版权方
- **Canaan Bright Sight Co., Ltd** (2025)
- **深圳百问网科技有限公司 (100askTeam)** (2008-2023)

### 开源协议
- 本项目遵循 **GPL V3 开源协议**
- 第三方库遵循各自许可协议（LVGL、Boost等）
- 商业使用需遵循GPL协议要求

## 🛠️ 快速开始

### 环境准备

1. **安装交叉编译工具链**
   ```bash
   # 下载并安装 Xuantie-900 工具链
   wget https://github.com/T-head-Semi/xuantie-gnu-toolchain/releases/download/v3.0.2/Xuantie-900-gcc-linux-6.6.0-glibc-x86_64-V3.0.2.tar.gz
   tar -xzf Xuantie-900-gcc-linux-6.6.0-glibc-x86_64-V3.0.2.tar.gz
   sudo mv Xuantie-900-gcc-linux-6.6.0-glibc-x86_64-V3.0.2 /opt/toolchain/
   ```

2. **安装依赖库**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install cmake build-essential libasound2-dev libdrm-dev
   
   # CentOS/RHEL
   sudo yum install cmake gcc-c++ alsa-lib-devel libdrm-devel
   ```

### 编译项目

1. **克隆项目**
   ```bash
   git clone https://github.com/yourusername/xiaozhi_assistant.git
   cd xiaozhi_assistant
   ```

2. **编译项目**
   ```bash
   # 赋予执行权限
   chmod +x build.sh
   
   # 开始编译
   ./build.sh
   ```

3. **编译输出**
   编译完成后，在 `k230_bin/` 目录下会生成：
   - `ui_and_ai` - UI和AI模块可执行文件
   - `xiaozhi_client` - 语音客户端可执行文件
   - `run.sh` - 运行脚本
   - 相关资源文件（模型、字体等）

### 部署到开发板

1. **传输文件到开发板**
   ```bash
   # 使用 scp 传输整个 k230_bin 目录
   scp -r k230_bin/ root@<开发板IP>:/home/root/
   ```

2. **在开发板上运行**
   ```bash
   # SSH 登录开发板
   ssh root@<开发板IP>
   
   # 进入项目目录
   cd /home/root/k230_bin
   
   # 赋予执行权限
   chmod +x run.sh ui_and_ai xiaozhi_client
   
   # 运行项目
   ./run.sh
   ```

### 运行模式

项目支持多种运行模式：

```bash
# 自动语音交互模式
./xiaozhi_client auto

# 手动语音交互模式  
./xiaozhi_client manual

# 实时语音交互模式
./xiaozhi_client realtime

# 唤醒词模式
./xiaozhi_client wakeup
```

## 🤝 贡献指南

### 如何贡献
1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

### 开发规范
- 遵循现有代码风格
- 添加适当的注释和文档
- 确保代码通过编译测试
- 提交信息使用清晰的描述

## 📞 支持与联系

- **问题反馈**: [GitHub Issues](https://github.com/yourusername/xiaozhi_assistant/issues)
- **讨论交流**: [GitHub Discussions](https://github.com/yourusername/xiaozhi_assistant/discussions)
- **文档**: [项目Wiki](https://github.com/yourusername/xiaozhi_assistant/wiki)

## 🙏 致谢

### 特别感谢
- **100askTeam** - 提供优秀的嵌入式系统教学资源和开源代码
- **Canaan Technology** - 提供K230开发板硬件支持
- **LVGL Community** - 提供优秀的图形库支持
- **开源社区** - 所有贡献者和用户的支持

### 相关项目
- [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) - 原始ESP32版本
- [kendryte/k230_linux_sdk](https://github.com/kendryte/k230_linux_sdk) - K230 Linux SDK
- [LVGL/lvgl](https://github.com/lvgl/lvgl) - 轻量级图形库

---

<div align="center">

**⭐ 如果这个项目对您有帮助，请给我们一个 Star！⭐**

Made with ❤️ by the XiaoZhi Assistant Team

</div>
