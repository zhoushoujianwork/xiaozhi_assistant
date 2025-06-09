#!/bin/sh

# 关闭 ui_and_ai
echo "正在关闭进程：ui_and_ai"
PID=$(ps -e | grep "ui_and_ai" | grep -v "grep" | awk '{print $1}')

if [ -n "$PID" ]; then
    # 尝试优雅关闭
    kill "$PID"
    echo "已发送 SIGTERM 信号至 PID：$PID"

    # 等待进程关闭（最多 5 秒）
    TIMEOUT=5
    while [ $TIMEOUT -gt 0 ]; do
        if ! ps -p "$PID" > /dev/null 2>&1; then
            echo "进程 ui_and_ai 已关闭"
            break
        fi
        sleep 1
        TIMEOUT=$((TIMEOUT - 1))
    done

    # 若未关闭，强制终止
    if ps -p "$PID" > /dev/null 2>&1; then
        kill -9 "$PID"
        echo "已发送 SIGKILL 强制终止 PID：$PID"
    fi
else
    echo "未找到进程：ui_and_ai"
fi

# 关闭 xiaozhi_client
echo "正在关闭进程：xiaozhi_client"
PID=$(ps -e | grep "xiaozhi_client" | grep -v "grep" | awk '{print $1}')

if [ -n "$PID" ]; then
    # 尝试优雅关闭
    kill "$PID"
    echo "已发送 SIGTERM 信号至 PID：$PID"

    # 等待进程关闭（最多 5 秒）
    TIMEOUT=5
    while [ $TIMEOUT -gt 0 ]; do
        if ! ps -p "$PID" > /dev/null 2>&1; then
            echo "进程 xiaozhi_client 已关闭"
            break
        fi
        sleep 1
        TIMEOUT=$((TIMEOUT - 1))
    done

    # 若未关闭，强制终止
    if ps -p "$PID" > /dev/null 2>&1; then
        kill -9 "$PID"
        echo "已发送 SIGKILL 强制终止 PID：$PID"
    fi
else
    echo "未找到进程：xiaozhi_client"
fi