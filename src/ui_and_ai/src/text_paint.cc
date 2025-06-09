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
#include "text_paint.h"
#include <iostream>
#include <fstream>

// 构造函数的具体实现
ChineseTextRenderer::ChineseTextRenderer(std::string fontFile, int fontSize) {
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "无法初始化FreeType库\n");
        return;
    }

    if (FT_New_Face(ft, fontFile.c_str(), 0, &face)) {
        fprintf(stderr, "无法加载字体文件\n");
        return;
    }

    // 设置字体大小
    FT_Set_Pixel_Sizes(face, 0, fontSize);
}

// 析构函数的具体实现
ChineseTextRenderer::~ChineseTextRenderer() {
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void ChineseTextRenderer::putChineseText(cv::Mat& img, std::string text, cv::Point position, cv::Scalar color) {
    int channels = img.channels();
    if (channels != 3 && channels != 4) {
        // 非法格式，尝试转换为 BGR
        cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
        channels = 3;
    }

    cv::Point pos(position.x, position.y);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring textw = converter.from_bytes(text);

    for (wchar_t c : textw) {
        FT_Load_Char(face, c, FT_LOAD_RENDER);
        FT_GlyphSlot glyph = face->glyph;

        for (int row = 0; row < glyph->bitmap.rows; ++row) {
            for (int col = 0; col < glyph->bitmap.width; ++col) {
                int x = pos.x + glyph->bitmap_left + col;
                int y = pos.y - glyph->bitmap_top + row;

                if (x >= 0 && x < img.cols && y >= 0 && y < img.rows) {
                    uchar gray = glyph->bitmap.buffer[row * glyph->bitmap.pitch + col];
                    if (gray > 0) {
                        if (channels == 3) {
                            // BGR 图像
                            cv::Vec3b& pixel = img.at<cv::Vec3b>(y, x);
                            for (int i = 0; i < 3; ++i) {
                                pixel[i] = static_cast<uchar>((pixel[i] * (255 - gray) + color[i] * gray) / 255);
                            }
                        } else if (channels == 4) {
                            // BGRA 图像
                            cv::Vec4b& pixel = img.at<cv::Vec4b>(y, x);
                            for (int i = 0; i < 3; ++i) { // 只混合 BGR，不改 A
                                pixel[i] = static_cast<uchar>((pixel[i] * (255 - gray) + color[i] * gray) / 255);
                            }
                            // 如果你希望也修改 alpha，可以加上：
                            pixel[3] = std::max(pixel[3], gray);
                        }
                    }
                }
            }
        }
        pos.x += glyph->advance.x >> 6;
    }
}
