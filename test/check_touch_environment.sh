#!/bin/bash
# Touch环境检查脚本
# 快速检查OpenHaptics环境是否正确配置

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "========================================"
echo "    Touch环境检查"
echo "========================================"
echo

# 检查环境变量
echo -e "${BLUE}1. 检查环境变量:${NC}"
if [[ -n "$GTDD_HOME" ]]; then
    echo -e "   ${GREEN}✓${NC} GTDD_HOME = $GTDD_HOME"
else
    echo -e "   ${YELLOW}⚠${NC} GTDD_HOME 未设置"
    echo -e "     运行: export GTDD_HOME=/usr"
fi

if [[ -n "$LD_LIBRARY_PATH" ]]; then
    echo -e "   ${BLUE}ℹ${NC} LD_LIBRARY_PATH = $LD_LIBRARY_PATH"
else
    echo -e "   ${GREEN}✓${NC} LD_LIBRARY_PATH 未设置 (使用系统库)"
fi
echo

# 检查库文件
echo -e "${BLUE}2. 检查OpenHaptics库文件:${NC}"
libs_found=0
total_libs=4

for lib in "libHL.so.3.4.0" "libHD.so.3.4.0" "libHLU.a" "libHDU.a"; do
    if [[ -f "/usr/lib/$lib" ]]; then
        echo -e "   ${GREEN}✓${NC} /usr/lib/$lib"
        ((libs_found++))
    else
        echo -e "   ${RED}✗${NC} /usr/lib/$lib (缺失)"
    fi
done
echo

# 检查符号链接
echo -e "${BLUE}3. 检查符号链接:${NC}"
links_found=0
total_links=4

for link in "libHL.so" "libHL.so.3.4" "libHD.so" "libHD.so.3.4"; do
    if [[ -L "/usr/lib/$link" ]]; then
        echo -e "   ${GREEN}✓${NC} /usr/lib/$link -> $(readlink /usr/lib/$link)"
        ((links_found++))
    else
        echo -e "   ${RED}✗${NC} /usr/lib/$link (缺失)"
    fi
done
echo

# 检查头文件
echo -e "${BLUE}4. 检查头文件目录:${NC}"
headers_found=0
total_headers=4

for dir in "HD" "HDU" "HL" "HLU"; do
    if [[ -d "/usr/include/$dir" ]] && [[ -n "$(ls -A "/usr/include/$dir" 2>/dev/null)" ]]; then
        file_count=$(ls -1 "/usr/include/$dir" | wc -l)
        echo -e "   ${GREEN}✓${NC} /usr/include/$dir ($file_count 个文件)"
        ((headers_found++))
    else
        echo -e "   ${RED}✗${NC} /usr/include/$dir (缺失或为空)"
    fi
done
echo

# 检查系统缓存
echo -e "${BLUE}5. 检查系统库缓存:${NC}"
if ldconfig -p | grep -q "libHL\|libHD"; then
    echo -e "   ${GREEN}✓${NC} OpenHaptics库已加载到系统缓存"
    ldconfig -p | grep -E "(libHL|libHD)" | while read line; do
        echo -e "     $line"
    done
else
    echo -e "   ${RED}✗${NC} OpenHaptics库未在系统缓存中"
    echo -e "     运行: sudo ldconfig"
fi
echo

# 检查配置目录
echo -e "${BLUE}6. 检查配置目录:${NC}"
if [[ -d "/usr/config" ]]; then
    echo -e "   ${GREEN}✓${NC} /usr/config (权限: $(stat -c %a /usr/config))"
else
    echo -e "   ${YELLOW}⚠${NC} /usr/config 不存在"
    echo -e "     运行: sudo mkdir -p /usr/config && sudo chmod 755 /usr/config"
fi
echo

# 检查可执行文件
echo -e "${BLUE}7. 检查程序文件:${NC}"
if [[ -f "./Touch_Controller_Arm2" ]]; then
    echo -e "   ${GREEN}✓${NC} Touch_Controller_Arm2 可执行文件存在"
    
    # 检查程序链接的库
    if command -v ldd >/dev/null 2>&1; then
        hd_lib=$(ldd ./Touch_Controller_Arm2 2>/dev/null | grep "libHD" || echo "未找到")
        if [[ "$hd_lib" != "未找到" ]]; then
            echo -e "   ${GREEN}✓${NC} 程序已正确链接OpenHaptics库"
            echo -e "     $hd_lib"
        else
            echo -e "   ${YELLOW}⚠${NC} 程序可能未正确链接OpenHaptics库"
        fi
    fi
else
    echo -e "   ${YELLOW}⚠${NC} Touch_Controller_Arm2 可执行文件不存在"
    echo -e "     运行: make clean && make"
fi
echo

# 总结
echo "========================================"
echo -e "${BLUE}环境状态总结:${NC}"

total_score=0
max_score=0

# 库文件检查 (20分)
max_score=$((max_score + 20))
if [[ $libs_found -eq $total_libs ]]; then
    total_score=$((total_score + 20))
    echo -e "   ${GREEN}✓${NC} 库文件: $libs_found/$total_libs (20/20分)"
else
    score=$((libs_found * 20 / total_libs))
    total_score=$((total_score + score))
    echo -e "   ${YELLOW}⚠${NC} 库文件: $libs_found/$total_libs ($score/20分)"
fi

# 符号链接检查 (15分)
max_score=$((max_score + 15))
if [[ $links_found -eq $total_links ]]; then
    total_score=$((total_score + 15))
    echo -e "   ${GREEN}✓${NC} 符号链接: $links_found/$total_links (15/15分)"
else
    score=$((links_found * 15 / total_links))
    total_score=$((total_score + score))
    echo -e "   ${YELLOW}⚠${NC} 符号链接: $links_found/$total_links ($score/15分)"
fi

# 头文件检查 (15分)
max_score=$((max_score + 15))
if [[ $headers_found -eq $total_headers ]]; then
    total_score=$((total_score + 15))
    echo -e "   ${GREEN}✓${NC} 头文件: $headers_found/$total_headers (15/15分)"
else
    score=$((headers_found * 15 / total_headers))
    total_score=$((total_score + score))
    echo -e "   ${YELLOW}⚠${NC} 头文件: $headers_found/$total_headers ($score/15分)"
fi

# 环境变量检查 (10分)
max_score=$((max_score + 10))
if [[ -n "$GTDD_HOME" ]]; then
    total_score=$((total_score + 10))
    echo -e "   ${GREEN}✓${NC} 环境变量: 已设置 (10/10分)"
else
    echo -e "   ${YELLOW}⚠${NC} 环境变量: 未设置 (0/10分)"
fi

# 系统缓存检查 (20分)
max_score=$((max_score + 20))
if ldconfig -p | grep -q "libHL\|libHD"; then
    total_score=$((total_score + 20))
    echo -e "   ${GREEN}✓${NC} 系统缓存: 已加载 (20/20分)"
else
    echo -e "   ${RED}✗${NC} 系统缓存: 未加载 (0/20分)"
fi

# 配置目录检查 (10分)
max_score=$((max_score + 10))
if [[ -d "/usr/config" ]]; then
    total_score=$((total_score + 10))
    echo -e "   ${GREEN}✓${NC} 配置目录: 存在 (10/10分)"
else
    echo -e "   ${YELLOW}⚠${NC} 配置目录: 不存在 (0/10分)"
fi

# 可执行文件检查 (10分)
max_score=$((max_score + 10))
if [[ -f "./Touch_Controller_Arm2" ]]; then
    total_score=$((total_score + 10))
    echo -e "   ${GREEN}✓${NC} 可执行文件: 存在 (10/10分)"
else
    echo -e "   ${YELLOW}⚠${NC} 可执行文件: 不存在 (0/10分)"
fi

echo "----------------------------------------"
percentage=$((total_score * 100 / max_score))

if [[ $percentage -ge 90 ]]; then
    echo -e "${GREEN}总分: $total_score/$max_score ($percentage%) - 环境配置完美！${NC}"
elif [[ $percentage -ge 70 ]]; then
    echo -e "${YELLOW}总分: $total_score/$max_score ($percentage%) - 环境基本正常，建议优化${NC}"
else
    echo -e "${RED}总分: $total_score/$max_score ($percentage%) - 环境需要修复${NC}"
fi

echo "========================================"

# 提供修复建议
if [[ $percentage -lt 90 ]]; then
    echo
    echo -e "${BLUE}修复建议:${NC}"
    
    if [[ $libs_found -lt $total_libs ]] || [[ $links_found -lt $total_links ]] || [[ $headers_found -lt $total_headers ]]; then
        echo "1. 运行完整环境配置脚本:"
        echo "   ./setup_touch_environment.sh"
    fi
    
    if [[ -z "$GTDD_HOME" ]]; then
        echo "2. 设置环境变量:"
        echo "   export GTDD_HOME=/usr"
    fi
    
    if ! ldconfig -p | grep -q "libHL\|libHD"; then
        echo "3. 更新系统库缓存:"
        echo "   sudo ldconfig"
    fi
    
    if [[ ! -f "./Touch_Controller_Arm2" ]]; then
        echo "4. 重新编译程序:"
        echo "   make clean && make"
    fi
    
    echo
fi
