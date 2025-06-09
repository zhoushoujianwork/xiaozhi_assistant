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

#ifndef _KWS_H
#define _KWS_H

#include "ai_base.h"
#include "feature_pipeline.h"


/**
 * @brief KWS—语音唤醒
 * 封装了利用麦克风实时语音唤醒的过程
 */
class KWS : public AIBase
{
    public:
        /**
        * @brief KWS构造函数，加载kmodel
        * @param kmodel_file kmodel文件路径
        * @param task_name  任务名称
        * @param num_keyword 关键词数量
        * @param debug_mode  0（不调试）、 1（只显示时间）、2（显示所有打印信息）
        * @return None
        */
        KWS(char *kmodel_file, int num_keyword, float spot_thresh, int debug_mode);

        ~KWS();

        bool pre_process(std::vector<float> wav);

        /**
        * @brief kmodel推理
        * @return None
        */
        void inference();

        /**
        * @brief kmodel推理结果后处理
        * @return None
        */
        std::string post_process();

        static wenet::FeaturePipelineConfig feature_config;
        static wenet::FeaturePipeline feature_pipeline;


    private:
        int num_bin = 40;                     // 音频特征维度
        const int chunk_size = 30;            // 推理块大小
        const std::string task_name;          // 音频位置
        const int batch_size = 1;             // 音频预处理默认参数，不可更改！
        const int hidden_dim = 256;           // 隐藏层维度
        const int cache_dim = 105;            // in_cache最后一维数量, 与模型参数绑定，不可更改！
        const int num_keyword;                // 唤醒词数量, 包含Deactivated！
        const float spot_thresh;              // 唤醒阈值
        std::vector<string> idx2res;          // ID与唤醒词的映射
        runtime_tensor wav_feature;           // Kmodel输入
        runtime_tensor in_cache;              // Kmodel输入
        std::vector<std::vector<float>> cache;// 用于保存模型每次推理输出的cache, cache用于流式推理
};
#endif