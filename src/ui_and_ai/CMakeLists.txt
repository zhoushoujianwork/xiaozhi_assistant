cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 17)
project(ui_and_ai)

file(READ "../../../../../.last_conf" LAST_CONF_CONTENT)
string(REGEX MATCH "CONF=([^\n]+)" _ "${LAST_CONF_CONTENT}")
if (CMAKE_MATCH_1)
    set(CONF_VALUE "${CMAKE_MATCH_1}")
    message(STATUS "Current CONF: ${CONF_VALUE}")
else()
    message(FATAL_ERROR "not found CONF in .last_conf")
endif()

file(GLOB src_src CONFIGURE_DEPENDS src/*.c src/*.cc)
file(GLOB src_common CONFIGURE_DEPENDS ../common/*.c ../common/*.cc)

set(bin ui_and_ai)

set(sdk_root "${PROJECT_SOURCE_DIR}/../../../../../")
set(usr_root "${sdk_root}/output/${CONF_VALUE}/host/riscv64-buildroot-linux-gnu/sysroot/usr")

include_directories(
	${usr_root}/include
	${usr_root}/include/drm
	${usr_root}/include/libdrm
    )
include_directories(${usr_root}/../include)
include_directories(${PROJECT_SOURCE_DIR}/src/lvgl)
include_directories(${CMAKE_SOURCE_DIR}/src/lv_100ask_xz_ai)
include_directories(${sdk_root}/output/${CONF_VALUE}/build/freetype-2.13.3/include)

set(LV_CONF_PATH ${PROJECT_SOURCE_DIR}/src/lvgl/lv_conf.h)
add_subdirectory(src/lvgl)
add_subdirectory(src/lv_100ask_xz_ai)


# 触摸debug选项
option(ENABLE_TOUCH_DEBUG "Enable touch debug output" ON)
if(ENABLE_TOUCH_DEBUG)
    add_definitions(-DTOUCH_DEBUG_ENABLED)
    message(STATUS "Touch debug enabled")
else()
    message(STATUS "Touch debug disabled")
endif()

# 检查架构
if (BR2_RISCV_32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv32gcv_xtheadc")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=rv32gcv_xtheadc")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -march=rv32gcv_xtheadc")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=c908v -O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=c908v -O2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcpu=c908v -O2")
endif()

link_directories(
	${usr_root}/lib
	${usr_root}/../lib
)

link_directories(
	${usr_root}/lib
	${usr_root}/../lib
)

add_executable(ui_and_ai ${src_src} ${src_common})

# 设置编译器路径
set(CMAKE_C_COMPILER /opt/toolchain/Xuantie-900-gcc-linux-6.6.0-glibc-x86_64-V3.0.2/bin/riscv64-unknown-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /opt/toolchain/Xuantie-900-gcc-linux-6.6.0-glibc-x86_64-V3.0.2/bin/riscv64-unknown-linux-gnu-g++)

# 设置 sysroot 路径
set(CMAKE_SYSROOT ${sdk_root}/output/${CONF_VALUE}/host/riscv64-buildroot-linux-gnu/sysroot)

include_directories(../common)
include_directories(${usr_root}/buildroot-overlay/package/libmmz/)
include_directories(${usr_root}/include/nncase/include/)
include_directories(${usr_root}/include)
include_directories(${usr_root}/include/libdrm)
include_directories(${usr_root}/include/opencv4)
#include_directories(${sdk_root}/output/${CONF_VALUE}/build/freetype-2.13.2/include)
include_directories(${sdk_root}/output/${CONF_VALUE}/host/riscv64-buildroot-linux-gnu/sysroot/usr/include/alsa)


target_link_directories(${bin} PRIVATE
    ${usr_root}/lib
    /opt/toolchain/Xuantie-900-gcc-linux-6.6.0-glibc-x86_64-V3.0.2/sysroot/usr/lib64/lp64d
    /opt/toolchain/Xuantie-900-gcc-linux-6.6.0-glibc-x86_64-V3.0.2/sysroot/
)

target_link_libraries(${bin} -Wl,--start-group Nncase.Runtime.Native nncase.rt_modules.k230 functional_k230 lv_100ask_xz_ai lvgl opencv_imgcodecs opencv_imgproc opencv_core sharpyuv libjpeg.so.9 webp png z v4l2-drm drm display mmz pthread freetype asound bz2 lvgl::thorvg rt)

install(TARGETS ${bin} DESTINATION bin)
