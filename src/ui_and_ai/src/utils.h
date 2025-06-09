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
// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <iostream>
#include <fstream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <nncase/functional/ai2d/ai2d_builder.h>

using namespace nncase;
using namespace nncase::runtime;
using namespace nncase::runtime::k230;
using namespace nncase::F::k230;

using cv::Mat;
using std::cout;
using std::endl;
using std::ifstream;
using std::vector;


/**
 * @brief 检测框
 */
typedef struct Bbox
{
    float x; // 检测框的左顶点x坐标
    float y; // 检测框的左顶点x坐标
    float w;
    float h;
} Bbox;

/**
 * @brief 单张/帧图片大小
 */
typedef struct FrameSize
{
    size_t width;  // 宽
    size_t height; // 高
} FrameSize;

/**
 * @brief 单张/帧图片大小
 */
typedef struct FrameCHWSize
{
    size_t channel; // 通道
    size_t height;  // 高
    size_t width;   // 宽
} FrameCHWSize;

struct ai2d_import_dmabuf {
	int fd;         //整数类型的文件描述符（file descriptor）
	uintptr_t addr; //用于存储指针或内存地址
};
//创建一个用于ioctl（输入输出控制）系统的命令代码
#define AI2D_IMPORT_DMABUF _IOWR('A', 0, struct ai2d_import_dmabuf)

// 颜色盘 三通道
const std::vector<cv::Scalar> color_three = {cv::Scalar(220, 20, 60), cv::Scalar(119, 11, 32), cv::Scalar(0, 0, 142), cv::Scalar(0, 0, 230),
        cv::Scalar(106, 0, 228), cv::Scalar(0, 60, 100), cv::Scalar(0, 80, 100), cv::Scalar(0, 0, 70),
        cv::Scalar(0, 0, 192), cv::Scalar(250, 170, 30), cv::Scalar(100, 170, 30), cv::Scalar(220, 220, 0),
        cv::Scalar(175, 116, 175), cv::Scalar(250, 0, 30), cv::Scalar(165, 42, 42), cv::Scalar(255, 77, 255),
        cv::Scalar(0, 226, 252), cv::Scalar(182, 182, 255), cv::Scalar(0, 82, 0), cv::Scalar(120, 166, 157),
        cv::Scalar(110, 76, 0), cv::Scalar(174, 57, 255), cv::Scalar(199, 100, 0), cv::Scalar(72, 0, 118),
        cv::Scalar(255, 179, 240), cv::Scalar(0, 125, 92), cv::Scalar(209, 0, 151), cv::Scalar(188, 208, 182),
        cv::Scalar(0, 220, 176), cv::Scalar(255, 99, 164), cv::Scalar(92, 0, 73), cv::Scalar(133, 129, 255),
        cv::Scalar(78, 180, 255), cv::Scalar(0, 228, 0), cv::Scalar(174, 255, 243), cv::Scalar(45, 89, 255),
        cv::Scalar(134, 134, 103), cv::Scalar(145, 148, 174), cv::Scalar(255, 208, 186),
        cv::Scalar(197, 226, 255), cv::Scalar(171, 134, 1), cv::Scalar(109, 63, 54), cv::Scalar(207, 138, 255),
        cv::Scalar(151, 0, 95), cv::Scalar(9, 80, 61), cv::Scalar(84, 105, 51), cv::Scalar(74, 65, 105),
        cv::Scalar(166, 196, 102), cv::Scalar(208, 195, 210), cv::Scalar(255, 109, 65), cv::Scalar(0, 143, 149),
        cv::Scalar(179, 0, 194), cv::Scalar(209, 99, 106), cv::Scalar(5, 121, 0), cv::Scalar(227, 255, 205),
        cv::Scalar(147, 186, 208), cv::Scalar(153, 69, 1), cv::Scalar(3, 95, 161), cv::Scalar(163, 255, 0),
        cv::Scalar(119, 0, 170), cv::Scalar(0, 182, 199), cv::Scalar(0, 165, 120), cv::Scalar(183, 130, 88),
        cv::Scalar(95, 32, 0), cv::Scalar(130, 114, 135), cv::Scalar(110, 129, 133), cv::Scalar(166, 74, 118),
        cv::Scalar(219, 142, 185), cv::Scalar(79, 210, 114), cv::Scalar(178, 90, 62), cv::Scalar(65, 70, 15),
        cv::Scalar(127, 167, 115), cv::Scalar(59, 105, 106), cv::Scalar(142, 108, 45), cv::Scalar(196, 172, 0),
        cv::Scalar(95, 54, 80), cv::Scalar(128, 76, 255), cv::Scalar(201, 57, 1), cv::Scalar(246, 0, 122),
        cv::Scalar(191, 162, 208)};

// 颜色盘 四通道
const std::vector<cv::Scalar> color_four = {cv::Scalar(255, 220, 20, 60), cv::Scalar(255, 119, 11, 32), cv::Scalar(255, 0, 0, 142), cv::Scalar(255, 0, 0, 230),
        cv::Scalar(255, 106, 0, 228), cv::Scalar(255, 0, 60, 100), cv::Scalar(255, 0, 80, 100), cv::Scalar(255, 0, 0, 70),
        cv::Scalar(255, 0, 0, 192), cv::Scalar(255, 250, 170, 30), cv::Scalar(255, 100, 170, 30), cv::Scalar(255, 220, 220, 0),
        cv::Scalar(255, 175, 116, 175), cv::Scalar(255, 250, 0, 30), cv::Scalar(255, 165, 42, 42), cv::Scalar(255, 255, 77, 255),
        cv::Scalar(255, 0, 226, 252), cv::Scalar(255, 182, 182, 255), cv::Scalar(255, 0, 82, 0), cv::Scalar(255, 120, 166, 157),
        cv::Scalar(255, 110, 76, 0), cv::Scalar(255, 174, 57, 255), cv::Scalar(255, 199, 100, 0), cv::Scalar(255, 72, 0, 118),
        cv::Scalar(255, 255, 179, 240), cv::Scalar(255, 0, 125, 92), cv::Scalar(255, 209, 0, 151), cv::Scalar(255, 188, 208, 182),
        cv::Scalar(255, 0, 220, 176), cv::Scalar(255, 255, 99, 164), cv::Scalar(255, 92, 0, 73), cv::Scalar(255, 133, 129, 255),
        cv::Scalar(255, 78, 180, 255), cv::Scalar(255, 0, 228, 0), cv::Scalar(255, 174, 255, 243), cv::Scalar(255, 45, 89, 255),
        cv::Scalar(255, 134, 134, 103), cv::Scalar(255, 145, 148, 174), cv::Scalar(255, 255, 208, 186),
        cv::Scalar(255, 197, 226, 255), cv::Scalar(255, 171, 134, 1), cv::Scalar(255, 109, 63, 54), cv::Scalar(255, 207, 138, 255),
        cv::Scalar(255, 151, 0, 95), cv::Scalar(255, 9, 80, 61), cv::Scalar(255, 84, 105, 51), cv::Scalar(255, 74, 65, 105),
        cv::Scalar(255, 166, 196, 102), cv::Scalar(255, 208, 195, 210), cv::Scalar(255, 255, 109, 65), cv::Scalar(255, 0, 143, 149),
        cv::Scalar(255, 179, 0, 194), cv::Scalar(255, 209, 99, 106), cv::Scalar(255, 5, 121, 0), cv::Scalar(255, 227, 255, 205),
        cv::Scalar(255, 147, 186, 208), cv::Scalar(255, 153, 69, 1), cv::Scalar(255, 3, 95, 161), cv::Scalar(255, 163, 255, 0),
        cv::Scalar(255, 119, 0, 170), cv::Scalar(255, 0, 182, 199), cv::Scalar(255, 0, 165, 120), cv::Scalar(255, 183, 130, 88),
        cv::Scalar(255, 95, 32, 0), cv::Scalar(255, 130, 114, 135), cv::Scalar(255, 110, 129, 133), cv::Scalar(255, 166, 74, 118),
        cv::Scalar(255, 219, 142, 185), cv::Scalar(255, 79, 210, 114), cv::Scalar(255, 178, 90, 62), cv::Scalar(255, 65, 70, 15),
        cv::Scalar(255, 127, 167, 115), cv::Scalar(255, 59, 105, 106), cv::Scalar(255, 142, 108, 45), cv::Scalar(255, 196, 172, 0),
        cv::Scalar(255, 95, 54, 80), cv::Scalar(255, 128, 76, 255), cv::Scalar(255, 201, 57, 1), cv::Scalar(255, 246, 0, 122),
        cv::Scalar(255, 191, 162, 208)};

/**
 * @brief AI Demo工具类
 * 封装了AI Demo常用的函数，包括二进制文件读取、文件保存、图片预处理等操作
 */
class Utils
{
public:
    /**
     * @brief 读取2进制文件
     * @param file_name 文件路径
     * @return 文件对应类型的数据
     */
    template <class T>
    static vector<T> read_binary_file(const char *file_name)
    {
        ifstream ifs(file_name, std::ios::binary);
        ifs.seekg(0, ifs.end);
        size_t len = ifs.tellg();
        vector<T> vec(len / sizeof(T), 0);
        ifs.seekg(0, ifs.beg);
        ifs.read(reinterpret_cast<char *>(vec.data()), len);
        ifs.close();
        return vec;
    }

    /**
     * @brief 打印数据
     * @param data 需打印数据对应指针
     * @param size 需打印数据大小
     * @return None
     */
    template <class T>
    static void dump(const T *data, size_t size)
    {
        for (size_t i = 0; i < size; i++)
        {
            cout << data[i] << " ";
        }
        cout << endl;
    }

    // 静态成员函数不依赖于类的实例，可以直接通过类名调用
    /**
     * @brief 将数据以2进制方式保存为文件
     * @param file_name 保存文件路径+文件名
     * @param data      需要保存的数据
     * @param size      需要保存的长度
     * @return None
     */
    static void dump_binary_file(const char *file_name, char *data, const size_t size);

    /**
     * @brief 将数据保存为灰度图片
     * @param file_name  保存图片路径+文件名
     * @param frame_size 保存图片的宽、高
     * @param data       需要保存的数据
     * @return None
     */
    static void dump_gray_image(const char *file_name, const FrameSize &frame_size, unsigned char *data);

    /**
     * @brief 将数据保存为彩色图片
     * @param file_name  保存图片路径+文件名
     * @param frame_size 保存图片的宽、高
     * @param data       需要保存的数据
     * @return None
     */
    static void dump_color_image(const char *file_name, const FrameSize &frame_size, unsigned char *data);

    /**
     * @brief padding_resize函数（右或下padding），对chw数据进行padding & resize
     * @param ori_shape        原始数据chw
     * @param resize_shape     resize之后的大小
     * @param builder          ai2d构建器，用于运行ai2d
     * @param ai2d_in_tensor   ai2d输入
     * @param ai2d_out_tensor  ai2d输出
     * @param padding          填充值，用于resize时的等比例变换
     * @param ai2d_in_tensor_flag      是否使用预定好的ai2d_in_tensor
     * @return None
     */
    static void padding_resize_one_side(FrameCHWSize ori_shape, FrameSize resize_shape, std::unique_ptr<ai2d_builder> &builder, runtime_tensor &ai2d_in_tensor, runtime_tensor &ai2d_out_tensor, const cv::Scalar padding);

    /**
     * @brief 仿射变换函数，对chw数据进行仿射变换(for video)
     * @param affine_matrix    仿射变换矩阵
     * @param builder          ai2d构建器，用于运行ai2d
     * @param ai2d_in_tensor   ai2d输入
     * @param ai2d_out_tensor  ai2d输出
     * @return None
     */
    static void affine(float *affine_matrix, std::unique_ptr<ai2d_builder> &builder, runtime_tensor &ai2d_in_tensor, runtime_tensor &ai2d_out_tensor);
};

#endif