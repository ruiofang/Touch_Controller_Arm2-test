#!/bin/bash

echo "=== 灵巧手系统启动脚本 ==="

# 检查是否以root权限运行CAN配置
if [ "$EUID" -eq 0 ]; then
    echo "⚠️  不建议以root权限运行整个程序"
    echo "建议：先用sudo配置CAN接口，然后以普通用户身份运行程序"
fi

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}📡 检查CAN总线配置...${NC}"

# 检查CAN模块
if ! lsmod | grep -q "^can "; then
    echo -e "${YELLOW}🔧 加载CAN模块...${NC}"
    sudo modprobe can 2>/dev/null || echo -e "${RED}❌ 无法加载CAN模块${NC}"
fi

if ! lsmod | grep -q "^can_raw "; then
    sudo modprobe can_raw 2>/dev/null || echo -e "${RED}❌ 无法加载CAN_RAW模块${NC}"
fi

if ! lsmod | grep -q "^vcan "; then
    sudo modprobe vcan 2>/dev/null || echo -e "${YELLOW}⚠️  VCAN模块加载失败（虚拟CAN，可选）${NC}"
fi

# 检查can0接口
if ip link show can0 >/dev/null 2>&1; then
    CAN0_STATE=$(ip link show can0 | grep -o "state [A-Z]*" | cut -d' ' -f2)
    echo -e "${GREEN}✅ can0接口存在，状态: $CAN0_STATE${NC}"
    
    if [ "$CAN0_STATE" != "UP" ]; then
        echo -e "${YELLOW}🔧 尝试启动can0接口...${NC}"
        sudo ip link set can0 down 2>/dev/null
        sudo ip link set can0 type can bitrate 1000000 2>/dev/null
        sudo ip link set can0 up 2>/dev/null
        
        # 重新检查状态
        CAN0_STATE=$(ip link show can0 | grep -o "state [A-Z]*" | cut -d' ' -f2)
        if [ "$CAN0_STATE" = "UP" ]; then
            echo -e "${GREEN}✅ can0接口已启动${NC}"
        else
            echo -e "${RED}❌ can0接口启动失败${NC}"
        fi
    fi
else
    echo -e "${RED}❌ can0接口不存在${NC}"
    echo -e "${YELLOW}💡 如果使用虚拟CAN进行测试，可以运行：${NC}"
    echo "   sudo ip link add dev can0 type vcan"
    echo "   sudo ip link set up can0"
fi

# 检查can1接口（如果配置了第二个设备）
if ip link show can1 >/dev/null 2>&1; then
    CAN1_STATE=$(ip link show can1 | grep -o "state [A-Z]*" | cut -d' ' -f2)
    echo -e "${GREEN}✅ can1接口存在，状态: $CAN1_STATE${NC}"
    
    if [ "$CAN1_STATE" != "UP" ]; then
        echo -e "${YELLOW}🔧 尝试启动can1接口...${NC}"
        sudo ip link set can1 down 2>/dev/null
        sudo ip link set can1 type can bitrate 1000000 2>/dev/null
        sudo ip link set can1 up 2>/dev/null
        
        CAN1_STATE=$(ip link show can1 | grep -o "state [A-Z]*" | cut -d' ' -f2)
        if [ "$CAN1_STATE" = "UP" ]; then
            echo -e "${GREEN}✅ can1接口已启动${NC}"
        else
            echo -e "${RED}❌ can1接口启动失败${NC}"
        fi
    fi
else
    echo -e "${YELLOW}⚠️  can1接口不存在（如果只使用单设备可忽略）${NC}"
fi

echo -e "${YELLOW}🐍 检查Python环境...${NC}"

# 检查Python版本
PYTHON_VERSION=$(python3 --version 2>&1 | cut -d' ' -f2)
echo -e "${GREEN}✅ Python版本: $PYTHON_VERSION${NC}"

# 检查Python SDK
if [ -d "linker_hand_python_sdk/LinkerHand" ]; then
    echo -e "${GREEN}✅ LinkerHand Python SDK 已找到${NC}"
    
    # 检查是否可以导入
    if python3 -c "import sys; sys.path.append('linker_hand_python_sdk'); import LinkerHand.linker_hand_api" 2>/dev/null; then
        echo -e "${GREEN}✅ LinkerHand模块可以正常导入${NC}"
    else
        echo -e "${RED}❌ LinkerHand模块导入失败${NC}"
        echo -e "${YELLOW}💡 尝试安装依赖：${NC}"
        echo "   cd linker_hand_python_sdk && pip3 install -r requirements.txt"
    fi
else
    echo -e "${RED}❌ LinkerHand Python SDK 未找到${NC}"
fi

echo -e "${YELLOW}🔧 检查编译状态...${NC}"

# 检查可执行文件
if [ -f "Touch_Controller_Arm2" ]; then
    echo -e "${GREEN}✅ 主程序已编译${NC}"
else
    echo -e "${RED}❌ 主程序未编译${NC}"
    echo -e "${YELLOW}💡 运行 'make' 来编译程序${NC}"
    exit 1
fi

# 检查配置文件
if [ -f "config.ini" ]; then
    echo -e "${GREEN}✅ 配置文件存在${NC}"
    
    # 检查灵巧手配置
    if grep -q "use_dexterous_hand = true" config.ini; then
        echo -e "${GREEN}✅ 发现灵巧手配置${NC}"
        
        HAND_TYPE=$(grep "hand_type = " config.ini | head -1 | cut -d'=' -f2 | tr -d ' ')
        HAND_JOINT=$(grep "hand_joint = " config.ini | head -1 | cut -d'=' -f2 | tr -d ' ')
        CAN_INTERFACE=$(grep "can_interface = " config.ini | head -1 | cut -d'=' -f2 | tr -d ' ')
        
        echo -e "   手型: ${GREEN}$HAND_TYPE${NC}"
        echo -e "   关节: ${GREEN}$HAND_JOINT${NC}" 
        echo -e "   CAN接口: ${GREEN}$CAN_INTERFACE${NC}"
        
        # 检查对应的位置配置文件
        POSITIONS_FILE="linker_hand_python_sdk/LinkerHand/config/${HAND_JOINT}_positions.yaml"
        if [ -f "$POSITIONS_FILE" ]; then
            echo -e "${GREEN}✅ 位置配置文件存在: $POSITIONS_FILE${NC}"
        else
            echo -e "${RED}❌ 位置配置文件缺失: $POSITIONS_FILE${NC}"
        fi
    else
        echo -e "${YELLOW}⚠️  灵巧手功能未启用（将使用夹爪模式）${NC}"
    fi
else
    echo -e "${RED}❌ 配置文件缺失${NC}"
    exit 1
fi

echo -e "${YELLOW}🎮 检查触觉设备驱动...${NC}"

# 检查OpenHaptics
if [ -d "OpenHaptics/openhaptics_3.4-0-developer-edition-amd64" ]; then
    echo -e "${GREEN}✅ OpenHaptics已安装${NC}"
else
    echo -e "${RED}❌ OpenHaptics未找到${NC}"
fi

echo ""
echo -e "${GREEN}=== 系统检查完成 ===${NC}"
echo ""

# 询问是否启动程序
read -p "是否现在启动程序？(y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${YELLOW}🚀 启动双触觉设备双机械臂控制系统...${NC}"
    echo ""
    exec ./run2.sh
else
    echo -e "${YELLOW}💡 准备就绪。运行 './run2.sh' 来启动程序${NC}"
fi
