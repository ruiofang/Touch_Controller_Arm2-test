#!/bin/bash
# Touch设备环境配置脚本
# 用于自动配置OpenHaptics库到系统环境
# 版本: 1.0

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查是否以root权限运行
check_root() {
    if [[ $EUID -eq 0 ]]; then
        log_error "请不要以root用户运行此脚本，脚本会在需要时自动使用sudo"
        exit 1
    fi
}

# 检查OpenHaptics源文件
check_source_files() {
    local openhaptics_dir="./OpenHaptics/openhaptics_3.4-0-developer-edition-amd64"
    
    if [[ ! -d "$openhaptics_dir" ]]; then
        log_error "OpenHaptics源目录不存在: $openhaptics_dir"
        exit 1
    fi
    
    if [[ ! -d "$openhaptics_dir/usr/lib" ]]; then
        log_error "OpenHaptics库文件目录不存在: $openhaptics_dir/usr/lib"
        exit 1
    fi
    
    if [[ ! -d "$openhaptics_dir/usr/include" ]]; then
        log_error "OpenHaptics头文件目录不存在: $openhaptics_dir/usr/include"
        exit 1
    fi
    
    log_success "OpenHaptics源文件检查通过"
}

# 备份现有库文件
backup_existing_files() {
    local backup_dir="/usr/lib/openhaptics_backup_$(date +%Y%m%d_%H%M%S)"
    
    if ls /usr/lib/libH*.* >/dev/null 2>&1; then
        log_info "发现现有OpenHaptics库文件，创建备份..."
        sudo mkdir -p "$backup_dir"
        sudo cp /usr/lib/libH*.* "$backup_dir/" 2>/dev/null || true
        log_success "备份已创建: $backup_dir"
    fi
}

# 安装库文件到系统
install_libraries() {
    log_info "开始安装OpenHaptics库文件到 /usr/lib..."
    
    # 复制库文件
    sudo cp ./OpenHaptics/openhaptics_3.4-0-developer-edition-amd64/usr/lib/libH*.* /usr/lib/
    
    # 设置权限
    sudo chmod 755 /usr/lib/libHL.so.3.4.0
    sudo chmod 755 /usr/lib/libHD.so.3.4.0
    sudo chmod 644 /usr/lib/libHLU.a
    sudo chmod 644 /usr/lib/libHDU.a
    
    # 创建符号链接
    sudo ln -sf /usr/lib/libHL.so.3.4.0 /usr/lib/libHL.so.3.4
    sudo ln -sf /usr/lib/libHL.so.3.4.0 /usr/lib/libHL.so
    sudo ln -sf /usr/lib/libHD.so.3.4.0 /usr/lib/libHD.so.3.4
    sudo ln -sf /usr/lib/libHD.so.3.4.0 /usr/lib/libHD.so
    
    log_success "OpenHaptics库文件安装完成"
}

# 安装头文件到系统
install_headers() {
    log_info "开始安装OpenHaptics头文件到 /usr/include..."
    
    # 创建目录并复制头文件
    sudo mkdir -p /usr/include/HD /usr/include/HDU /usr/include/HL /usr/include/HLU
    
    sudo cp ./OpenHaptics/openhaptics_3.4-0-developer-edition-amd64/usr/include/HD/* /usr/include/HD/
    sudo cp ./OpenHaptics/openhaptics_3.4-0-developer-edition-amd64/usr/include/HDU/* /usr/include/HDU/
    sudo cp ./OpenHaptics/openhaptics_3.4-0-developer-edition-amd64/usr/include/HL/* /usr/include/HL/
    sudo cp ./OpenHaptics/openhaptics_3.4-0-developer-edition-amd64/usr/include/HLU/* /usr/include/HLU/
    
    # 设置权限
    sudo chmod -R 644 /usr/include/H*/
    sudo chmod 755 /usr/include/HD /usr/include/HDU /usr/include/HL /usr/include/HLU
    
    log_success "OpenHaptics头文件安装完成"
}

# 配置系统目录
setup_system_directories() {
    log_info "配置系统目录..."
    
    # 创建配置目录
    if [[ ! -d "/usr/config" ]]; then
        sudo mkdir -p /usr/config
        sudo chmod 755 /usr/config
        log_success "创建配置目录: /usr/config"
    else
        log_info "配置目录已存在: /usr/config"
    fi
    
    # 更新动态链接库缓存
    sudo ldconfig
    log_success "动态链接库缓存已更新"
}

# 验证安装
verify_installation() {
    log_info "验证安装结果..."
    
    # 检查库文件
    local lib_files=("libHL.so.3.4.0" "libHD.so.3.4.0" "libHLU.a" "libHDU.a")
    for lib in "${lib_files[@]}"; do
        if [[ -f "/usr/lib/$lib" ]]; then
            log_success "库文件存在: /usr/lib/$lib"
        else
            log_error "库文件缺失: /usr/lib/$lib"
            return 1
        fi
    done
    
    # 检查符号链接
    local symlinks=("libHL.so" "libHL.so.3.4" "libHD.so" "libHD.so.3.4")
    for link in "${symlinks[@]}"; do
        if [[ -L "/usr/lib/$link" ]]; then
            log_success "符号链接存在: /usr/lib/$link"
        else
            log_error "符号链接缺失: /usr/lib/$link"
            return 1
        fi
    done
    
    # 检查头文件目录
    local header_dirs=("HD" "HDU" "HL" "HLU")
    for dir in "${header_dirs[@]}"; do
        if [[ -d "/usr/include/$dir" ]] && [[ -n "$(ls -A "/usr/include/$dir")" ]]; then
            log_success "头文件目录存在: /usr/include/$dir"
        else
            log_error "头文件目录缺失或为空: /usr/include/$dir"
            return 1
        fi
    done
    
    # 检查ldconfig缓存
    if ldconfig -p | grep -q "libHL\|libHD"; then
        log_success "库文件已加载到系统缓存"
    else
        log_warning "库文件未在系统缓存中找到，可能需要重新运行ldconfig"
    fi
    
    log_success "安装验证完成"
}

# 创建环境变量设置脚本
create_env_script() {
    local env_script="/usr/local/bin/setup-touch-env"
    
    log_info "创建环境变量设置脚本..."
    
    sudo tee "$env_script" > /dev/null << 'EOF'
#!/bin/bash
# OpenHaptics环境变量设置脚本
export GTDD_HOME=/usr
echo "OpenHaptics环境变量已设置:"
echo "  GTDD_HOME=$GTDD_HOME"
EOF
    
    sudo chmod +x "$env_script"
    log_success "环境变量脚本已创建: $env_script"
    log_info "使用方法: source $env_script"
}

# 显示使用说明
show_usage_info() {
    echo
    log_info "=== Touch环境配置完成 ==="
    echo
    echo "现在您可以："
    echo "1. 直接运行程序："
    echo "   ./start_touch_controller.sh"
    echo
    echo "2. 手动设置环境变量："
    echo "   export GTDD_HOME=/usr"
    echo
    echo "3. 使用环境变量脚本："
    echo "   source /usr/local/bin/setup-touch-env"
    echo
    echo "4. 编译程序（如需要）："
    echo "   make clean && make"
    echo
    log_success "Touch环境配置完成！"
}

# 主函数
main() {
    echo "========================================"
    echo "    Touch设备环境配置脚本"
    echo "    OpenHaptics系统级安装"
    echo "========================================"
    echo
    
    check_root
    check_source_files
    backup_existing_files
    install_libraries
    install_headers
    setup_system_directories
    verify_installation
    create_env_script
    show_usage_info
}

# 运行主函数
main "$@"
