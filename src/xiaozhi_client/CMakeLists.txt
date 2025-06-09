cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 17)
project(xiaozhi_client)

# 读取 .last_conf 文件中的 CONF 值
file(READ "../../../../../.last_conf" LAST_CONF_CONTENT)
string(REGEX MATCH "CONF=([^\n]+)" _ "${LAST_CONF_CONTENT}")
if (CMAKE_MATCH_1)
    set(CONF_VALUE "${CMAKE_MATCH_1}")
    message(STATUS "Current CONF: ${CONF_VALUE}")
else()
    message(FATAL_ERROR "not found CONF in .last_conf")
endif()

# 收集源文件
file(GLOB src_src CONFIGURE_DEPENDS *.cc)
file(GLOB src_common CONFIGURE_DEPENDS ../common/*.cc)
file(GLOB src_snd_core CONFIGURE_DEPENDS snd_core/*.cc)

set(bin xiaozhi_client)

set(sdk_root "${PROJECT_SOURCE_DIR}/../../../../../")
set(usr_root "${sdk_root}/output/${CONF_VALUE}/host/riscv64-buildroot-linux-gnu/sysroot/usr")

# 包含目录
include_directories(
    ${usr_root}/include
    ${usr_root}/include/webrtc-audio-processing-1
    ${usr_root}/include -I../common
    ${PROJECT_SOURCE_DIR}/snd_core
    ${PROJECT_SOURCE_DIR}/../common
    ${usr_root}/../include
    ${TARGET_DIR}/usr/include
)

# 设置编译器路径
set(CMAKE_C_COMPILER /opt/toolchain/Xuantie-900-gcc-linux-6.6.0-glibc-x86_64-V3.0.2/bin/riscv64-unknown-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /opt/toolchain/Xuantie-900-gcc-linux-6.6.0-glibc-x86_64-V3.0.2/bin/riscv64-unknown-linux-gnu-g++)

# 设置 sysroot 路径
set(CMAKE_SYSROOT ${sdk_root}/output/${CONF_VALUE}/host/riscv64-buildroot-linux-gnu/sysroot)

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

# 链接目录
link_directories(
    ${usr_root}/lib
    ${usr_root}/../lib
    ${TARGET_DIR}/usr/lib
)

# 添加可执行文件
add_executable(${bin} ${src_src} ${src_common} ${src_snd_core})

# 链接库
target_link_libraries(${bin}
    openal
    alure2
    asound
    pthread
    webrtc-audio-processing-1
    absl_raw_logging_internal
    absl_log_severity
    absl_bad_optional_access
    absl_string_view
    absl_throw_delegate
    opus
    boost_system
    ssl
    crypto
    curl
    z
)

# 安装目标
install(TARGETS ${bin} DESTINATION bin)