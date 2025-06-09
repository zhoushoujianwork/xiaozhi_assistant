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
#include "sensor_buf_manager.h"

SensorBufManager::SensorBufManager(FrameCHWSize isp_shape,std::vector<std::tuple<int, void*>> sensor_bufs)
{
    dims_t in_shape{1, isp_shape.channel, isp_shape.height, isp_shape.width};
    if (sensor_bufs.size())
    {
        for(int i=0;i<sensor_bufs.size();++i)
        {
            this->sensor_bufs.push_back(sensor_bufs[i]);
        }
    }
    ai2d_in_tensor = hrt::create(typecode_t::dt_uint8, in_shape, hrt::pool_shared).expect("create ai2d input tensor failed");
    isp_size = isp_shape.channel * isp_shape.height * isp_shape.width;
}

runtime_tensor& SensorBufManager::get_buf_for_index(unsigned index)
{
    if(index<this->sensor_bufs.size())
    {
        auto buf = ai2d_in_tensor.impl()->to_host().unwrap()->buffer().as_host().unwrap().map(map_access_::map_write).unwrap().buffer();
        memcpy(reinterpret_cast<char *>(buf.data()), (void *)(std::get<1>(this->sensor_bufs[index])), isp_size);
        hrt::sync(ai2d_in_tensor, sync_op_t::sync_write_back, true).expect("sync write_back failed");
        return ai2d_in_tensor;
    }
    else
    {
        printf("index : %d , buf_size : %d",index,this->sensor_bufs.size());
        assert(("Invalid index", 0));
    }
}

SensorBufManager::~SensorBufManager()
{

}