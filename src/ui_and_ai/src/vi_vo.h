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
#ifndef _VI_VO_H
#define _VI_VO_H

#include <cassert>
#include <iostream>
#include <linux/videodev2.h>
#include <mutex>
#include <opencv2/videoio.hpp>
#include <sys/select.h>
#include <atomic>
#include <unistd.h>
#include <stdlib.h>
#include "mmz.h"
#include <opencv2/opencv.hpp>
#include <display.h>
#include <v4l2-drm.h>
#include <unistd.h>
#include <fcntl.h>
#include <thead.h>
#include <vector>

#define SENSOR_CHANNEL (3)
#define SENSOR_HEIGHT (1080)
#define SENSOR_WIDTH (1920)
#endif