# Touch Controller Environment Scripts

这里提供了一套完整的Touch设备环境配置和管理脚本。

## 脚本说明

### 1. setup_touch_environment.sh - 环境配置脚本
完整的OpenHaptics环境配置脚本，将OpenHaptics库和头文件安装到系统目录。

**功能：**
- 将OpenHaptics库文件复制到 `/usr/lib`
- 将头文件复制到 `/usr/include`
- 创建必要的符号链接
- 更新系统动态链接库缓存
- 设置正确的文件权限
- 创建配置目录 `/usr/config`
- 备份现有文件
- 验证安装结果

**使用方法：**
```bash
./setup_touch_environment.sh
```

**注意：** 脚本会自动使用sudo权限，请不要以root用户运行。

### 2. check_touch_environment.sh - 环境检查脚本
全面检查Touch环境配置状态的诊断工具。

**检查项目：**
- 环境变量设置 (GTDD_HOME, LD_LIBRARY_PATH)
- OpenHaptics库文件存在性
- 符号链接完整性
- 头文件目录状态
- 系统库缓存状态
- 配置目录权限
- 可执行文件状态

**使用方法：**
```bash
./check_touch_environment.sh
```

**输出：** 
- 详细的检查结果
- 环境状态评分 (0-100分)
- 具体的修复建议

### 3. start_touch_controller.sh - 程序启动脚本
智能的程序启动脚本，自动设置环境变量并进行基本检查。

**功能：**
- 自动设置 `GTDD_HOME=/usr`
- 检查程序文件存在性
- 快速环境验证
- 启动Touch Controller程序

**使用方法：**
```bash
./start_touch_controller.sh
```

## 使用流程

### 首次设置
1. **配置环境：**
   ```bash
   ./setup_touch_environment.sh
   ```

2. **验证配置：**
   ```bash
   ./check_touch_environment.sh
   ```

3. **编译程序（如需要）：**
   ```bash
   make clean && make
   ```

4. **启动程序：**
   ```bash
   ./start_touch_controller.sh
   ```

### 日常使用
直接运行启动脚本即可：
```bash
./start_touch_controller.sh
```

### 故障排除
如果程序运行出现问题：

1. **检查环境状态：**
   ```bash
   ./check_touch_environment.sh
   ```

2. **根据提示修复问题**

3. **重新配置环境（如需要）：**
   ```bash
   ./setup_touch_environment.sh
   ```

## 文件结构

```
Touch_Controller_Arm2-test3/
├── setup_touch_environment.sh    # 环境配置脚本
├── check_touch_environment.sh     # 环境检查脚本
├── start_touch_controller.sh      # 程序启动脚本
├── Touch_Controller_Arm2.cpp      # 主程序源码
├── config.ini                     # 配置文件
├── Makefile                       # 编译配置
└── OpenHaptics/                   # OpenHaptics源文件
    └── openhaptics_3.4-0-developer-edition-amd64/
```

## 系统要求

- Ubuntu/Debian Linux
- sudo权限
- 基本的开发工具 (gcc, make)
- OpenHaptics源文件

## 环境变量

脚本会自动设置以下环境变量：
- `GTDD_HOME=/usr` - OpenHaptics根目录

## 系统目录

脚本会在以下系统目录中安装文件：
- `/usr/lib/` - OpenHaptics库文件
- `/usr/include/HD/`, `/usr/include/HDU/`, `/usr/include/HL/`, `/usr/include/HLU/` - 头文件
- `/usr/config/` - 配置目录
- `/usr/local/bin/setup-touch-env` - 环境变量设置脚本

## 常见问题

### Q: 程序提示找不到库文件
A: 运行 `./check_touch_environment.sh` 检查环境，然后根据提示修复。

### Q: 编译失败
A: 确保已运行 `./setup_touch_environment.sh` 配置环境。

### Q: 权限错误
A: 确保有sudo权限，不要以root用户运行脚本。

### Q: 环境检查评分较低
A: 按照检查脚本提供的修复建议操作。

## 技术支持

如遇到问题，请：
1. 先运行环境检查脚本
2. 按照修复建议操作
3. 查看程序日志输出

---

**版本：** 1.0  
**更新日期：** 2025年8月12日
