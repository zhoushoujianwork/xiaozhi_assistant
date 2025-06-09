# 小智语音助手项目文档

## 一、项目概述
### 1. 项目简介
小智语音助手是深度优化的嵌入式语音交互系统，基于[78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)项目功能迁移适配至**嘉楠K230开发板**实现。核心创新在于融合本地AI能力与云端服务，提供自然流畅的语音交互体验。

**技术亮点：**
- 🎙️ **双重唤醒机制**：集成K230本地语音唤醒+人脸识别唤醒
- 🖥️ **高性能GUI**：采用LVGL构建低延迟触控界面
- 🌐 **混合架构**：本地AI处理+云端服务协同
- 🔊 **专业级TTS**：集成讯飞星火语音合成服务

遵循**GPL V3开源协议**，确保代码开放性与可复用性。

### 2. 核心功能
#### （1）语音交互模块
- **实时数据传输**：通过WebSocket协议与服务器进行二进制Opus音频数据及JSON文本消息的实时传输，确保语音数据的高效、稳定传输。
- **语音指令处理**：支持语音指令解析、响应生成及多轮对话逻辑，能够准确理解用户的语音指令，并提供相应的回复。
- **本地语音唤醒**：利用K230开发板实现本地语音唤醒功能，用户可以通过特定的唤醒词唤醒语音助手，无需时刻与服务器保持连接，提高了唤醒的及时性和便捷性。
- **语音合成**：采用讯飞星火的语音合成技术（https://console.xfyun.cn/services/tts），将文本信息转换为自然流畅的语音输出，提供更加友好的交互体验。

#### （2）设备激活模块
- **HTTP协议激活**：基于HTTP协议实现设备激活，解析服务器返回的JSON激活状态及激活码，确保设备能够正常接入系统。

#### （3）人脸识别唤醒模块
- **实时识别**：利用K230开发板的算力，实现实时人脸识别功能，当识别到特定的人脸时，自动唤醒语音助手。

#### （4）用户界面模块
- **LVGL图形库集成**：集成LVGL图形库，提供触控交互界面，人脸管理：注册/识别/删除一体化界面。

## 二、版权声明与开源协议
### 1. 版权信息
#### （1）Canaan Bright Sight Co., Ltd 版权声明
```c
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
```

#### （2）深圳百问网科技有限公司（100askTeam）版权声明
```c
// SPDX-License-Identifier: GPL-3.0-only
/*
 * Copyright (c) 2008-2023 100askTeam : Dongshan WEI <weidongshan@100ask.net>
 * Discourse:  https://forums.100ask.net
 *
 *  Copyright (C) 2008-2023 深圳百问网科技有限公司
 *  All rights reserved
 *
 * 免责声明: 百问网编写的文档, 仅供学员学习使用, 可以转发或引用(请保留作者信息),禁止用于商业用途！
 * 免责声明: 百问网编写的程序, 用于商业用途请遵循GPL许可, 百问网不承担任何后果！
 *
 * 本程序遵循GPL V3协议, 请遵循协议
 * 百问网学习平台   : https://www.100ask.net
 * 百问网交流社区   : https://forums.100ask.net
 * 百问网官方B站    : https://space.bilibili.com/275908810
 * 联系我们(E-mail) : weidongshan@100ask.net
 *
 *          版权所有，盗版必究。
 */
```

### 2. 开源协议说明
- 本项目整体遵循 **GPL V3开源协议**，允许修改和分发，但需保留原版权声明并公开修改后的代码。
- 第三方库（如LVGL、LZ4）遵循其各自许可协议，使用时需单独遵守对应条款。

## 三、编译指南

### 1.获取 SDK 源码
#### 步骤 1：克隆 `k230_linux_sdk` 仓库
首先，从 GitHub 上克隆 `k230_linux_sdk` 源码仓库。在终端中执行以下命令：
```bash
git clone git@github.com:kendryte/k230_linux_sdk.git
```

#### 步骤 2：修改开发板配置文件
修改开发板的配置文件，以启用所需的软件包。配置文件的位置为 `k230_linux_sdk/buildroot-overlay/configs`。
使用文本编辑器打开该配置文件，并确保以下配置项被启用：
```plaintext
BR2_PACKAGE_WEBSOCKETPP=y
BR2_PACKAGE_BOOST=y
BR2_PACKAGE_BOOST_JSON=y
BR2_PACKAGE_BOOST_LOG=y
BR2_PACKAGE_BOOST_SERIALIZATION=y
BR2_PACKAGE_BOOST_URL=y
```

#### 步骤 3：配置并编译固件
进入 `k230_linux_sdk` 目录，并执行以下命令来配置和编译固件：
```bash
make k230_canmv_01studio_defconfig
make
```

### 2.编译小智语音助手
#### 步骤 1：下载小智语音助手源码
进入 `k230_linux_sdk/buildroot-overlay/package` 目录，从本仓库克隆小智语音助手的源码。在终端中执行以下命令，将 `<xiaozhi_assistant_repository_url>` 替换为实际的仓库地址：
```bash
git clone <xiaozhi_assistant_repository_url>
```

#### 步骤 2：编译小智语音助手源码
在 `k230_linux_sdk/buildroot-overlay/package/xiaozhi_assistant` 目录下，执行 `build.sh` 脚本以编译源码：
```bash
./build.sh
```
编译完成后，在 `k230_linux_sdk/buildroot-overlay/package/xiaozhi_assistant/k230_bin` 目录下会生成目标文件。该目录包含两个可执行文件：
- `ui_and_ai`：负责用户界面和人工智能相关功能。
- `xiaozhi_client`：小智语音助手的客户端程序。

### 3.运行小智语音助手
#### 步骤 1：将软件拷贝到开发板
将 `k230_linux_sdk/buildroot-overlay/package/xiaozhi_assistant/k230_bin` 目录下的所有文件和子目录拷贝到开发板上。可以使用 `scp` 等工具完成文件传输。

#### 步骤 2：运行软件
在开发板上，进入 `k230_bin` 目录，并执行 `run.sh` 脚本以启动小智语音助手：
```bash
cd k230_bin
./run.sh
```

通过以上步骤，你可以成功编译并运行小智语音助手。

## 四、致谢
### 1. 对100askTeam的感谢
本项目部分功能基于深圳百问网科技有限公司（100askTeam）的开源代码开发，其在嵌入式系统教学与开源社区中的贡献为项目提供了重要支持。感谢100askTeam的技术积累与分享！
