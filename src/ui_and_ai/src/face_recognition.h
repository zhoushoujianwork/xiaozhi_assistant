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
#ifndef _FACE_RECOGNITION_H
#define _FACE_RECOGNITION_H

#include <vector>
#include "utils.h"
#include "ai_base.h"

using std::vector;

typedef struct FaceRecognitionInfo
{
    int id;                     //人脸识别结果对应ID
    float score;                //人脸识别结果对应得分
    string name;                //人脸识别结果对应人名
} FaceRecognitionInfo;

/**
 * @brief 基于Retinaface的人脸检测
 * 主要封装了对于每一帧图片，从预处理、运行到后处理给出结果的过程
 */
class FaceRecognition : public AIBase
{
public:
    /**
     * @brief FaceRecognition构造函数，加载kmodel,并初始化kmodel输入、输出和人脸检测阈值(for isp)
     * @param kmodel_file       kmodel文件路径
     * @param max_register_face 数据库最多可以存放的人脸特征数
     * @param thresh            人脸识别阈值
     * @param isp_shape         isp输入大小（chw）
     * @param vaddr             isp对应虚拟地址
     * @param paddr             isp对应物理地址
     * @param debug_mode        0（不调试）、 1（只显示时间）、2（显示所有打印信息）
     * @return None
     */
    FaceRecognition(const char *kmodel_file,float thresh, FrameCHWSize isp_shape, const int debug_mode);

    /**
     * @brief FaceRecognition析构函数
     * @return None
     */
    ~FaceRecognition();

    void pre_process(runtime_tensor &input_tensor , float* sparse_points);

    /**
     * @brief kmodel推理
     * @return None
     */
    void inference();

    //for database
    /**
     * @brief 人脸数据库加载接口
     * @param db_pth 数据库目录
     * @return None
     */
    void database_init(char *db_pth);

    /**
    * @brief 人脸数据库在线注册，运行过程中，增加新的人脸特征
    */
    void database_add(std::string& name, char* db_path);


    void database_reset(char *db_pth);

    //for database
    /**
     * @brief 查询注册人数
     * @param db_pth 数据库目录
     * @return None
     */
    int database_count(char* db_pth);

    /**
     * @brief 人脸数据库查询接口
     * @param result 人脸识别结果
     * @return None
     */
    void database_search(FaceRecognitionInfo& result);

    /**
     * @brief 将处理好的轮廓画到原图
     * @param src_img     原图
     * @param bbox        识别人脸的检测框位置
     * @param result      人脸识别结果
     * @param pic_mode    ture(原图片)，false(osd)
     * @return None
     */
    static void draw_result(cv::Mat& src_img,Bbox& bbox,FaceRecognitionInfo& result);

private:
    /**
     * @brief svd
     * @param a     原始矩阵
     * @param u     左奇异向量
     * @param s     对角阵
     * @param v     右奇异向量
     * @return None
     */
    void svd22(const float a[4], float u[4], float s[2], float v[4]);

    /**
    * @brief 使用Umeyama算法计算仿射变换矩阵
    * @param src  原图像点位置
    * @param dst  目标图像（112*112）点位置。
    * @return None
    */
    void image_umeyama_112(float* src, float* dst);

    /**
    * @brief 获取affine变换矩阵
    * @param sparse_points  原图像人脸五官点位置
    * @return None
    */
    void get_affine_matrix(float* sparse_points);

    /**
    * @brief 使用L2范数对数据进行归一化
    * @param src  原始数据
    * @param dst  L2归一化后的数据
    * @param len  原始数据长度
    * @return None
    */
    void l2_normalize(float* src, float* dst, int len);

    /**
    * @brief 计算两特征的余弦距离
    * @param feature_0    第一个特征
    * @param feature_1    第二个特征
    * @param feature_len  特征长度
    * @return 余弦距离
    */
    float cal_cosine_distance(float* feature_0, float* feature_1, int feature_len);

    /**
    * @brief 获取目录下的文件个数
    * @param path     目标目录
    * @return 文件个数
    */
    int get_dir_files(const char *path);

    std::unique_ptr<ai2d_builder> ai2d_builder_; // ai2d构建器
    runtime_tensor ai2d_out_tensor_;             // ai2d输出tensor
    float matrix_dst_[10];                       // 人脸affine的变换矩阵
    int feature_num_;                             // 人脸识别提取特征长度
public:
    FrameCHWSize isp_shape_;                     // isp对应的地址大小
    float obj_thresh_;                            // 人脸识别阈值
    vector<string> names_;                        // 人脸数据库名字
    vector<vector<float>> features_;              // 人脸数据库特征
    int valid_register_face_;                     // 数据库中实际人脸个数
};
#endif