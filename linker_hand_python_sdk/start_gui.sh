#!/bin/bash

# LinkerHand GUI 启动脚本
# 作者: AI Assistant
# 日期: 2025-07-03

echo "=== LinkerHand GUI 启动脚本 ==="
echo "正在启动机械手GUI控制程序..."

# 设置项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GUI_DIR="$PROJECT_ROOT/example/gui_control"
VENV_PATH="$PROJECT_ROOT/.venv"

# 检查项目目录是否存在
if [ ! -d "$PROJECT_ROOT" ]; then
    echo "错误: 项目目录不存在: $PROJECT_ROOT"
    exit 1
fi

# 检查虚拟环境是否存在
if [ ! -d "$VENV_PATH" ]; then
    echo "错误: 虚拟环境不存在: $VENV_PATH"
    echo "请先创建虚拟环境: python3 -m venv .venv"
    exit 1
fi

# 切换到GUI目录
cd "$GUI_DIR" || {
    echo "错误: 无法切换到GUI目录: $GUI_DIR"
    exit 1
}

echo "当前工作目录: $(pwd)"

# 设置Qt插件路径
export QT_QPA_PLATFORM_PLUGIN_PATH="$VENV_PATH/lib/python3.10/site-packages/PyQt5/Qt/plugins/platforms"

# 检查Qt插件路径是否存在
if [ ! -d "$QT_QPA_PLATFORM_PLUGIN_PATH" ]; then
    echo "警告: Qt插件路径不存在: $QT_QPA_PLATFORM_PLUGIN_PATH"
    echo "请确保PyQt5已正确安装"
fi

echo "Qt插件路径: $QT_QPA_PLATFORM_PLUGIN_PATH"

# 激活虚拟环境并运行程序
echo "正在激活虚拟环境..."
source "$VENV_PATH/bin/activate"

echo "正在启动GUI程序..."
echo "=================================="

# 使用虚拟环境的Python运行程序
"$VENV_PATH/bin/python" gui_control.py

# 检查程序退出状态
if [ $? -eq 0 ]; then
    echo "程序正常退出"
else
    echo "程序异常退出 (退出码: $?)"
fi 