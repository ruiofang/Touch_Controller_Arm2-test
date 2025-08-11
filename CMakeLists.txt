cmake_minimum_required(VERSION 3.10)

# 项目信息
project(touch_controller_arm 
    VERSION 1.0.0
    DESCRIPTION "双Touch设备双机械臂控制系统"
    LANGUAGES CXX C)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# OpenHaptics路径设置
set(OPENHAPTICS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/OpenHaptics/openhaptics_3.4-0-developer-edition-amd64")
set(OPENHAPTICS_INCLUDE_DIR "${OPENHAPTICS_ROOT_DIR}/usr/include")
set(OPENHAPTICS_LIBRARY_DIR "${OPENHAPTICS_ROOT_DIR}/usr/lib")

# 检查OpenHaptics是否存在
if(NOT EXISTS "${OPENHAPTICS_ROOT_DIR}")
    message(FATAL_ERROR "OpenHaptics未找到！请确保路径正确: ${OPENHAPTICS_ROOT_DIR}")
endif()

# 查找OpenHaptics库
find_library(HD_LIBRARY 
    NAMES HD libHD libHD.so.3.4.0
    PATHS ${OPENHAPTICS_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

find_library(HDU_LIBRARY 
    NAMES HDU libHDU libHDU.a
    PATHS ${OPENHAPTICS_LIBRARY_DIR}
    NO_DEFAULT_PATH
)

if(NOT HD_LIBRARY)
    message(FATAL_ERROR "HD库未找到！路径: ${OPENHAPTICS_LIBRARY_DIR}")
endif()

if(NOT HDU_LIBRARY)
    message(FATAL_ERROR "HDU库未找到！路径: ${OPENHAPTICS_LIBRARY_DIR}")
endif()

# 查找系统库
find_package(Threads REQUIRED)

# 查找Python（仅在非ROS2模式下需要）
if(NOT ROS2_FOUND)
    find_package(Python3 COMPONENTS Interpreter Development QUIET)
    if(Python3_FOUND)
        message(STATUS "找到Python: ${Python3_VERSION}")
    else()
        message(WARNING "未找到Python开发包，Python相关功能可能不可用")
    endif()
endif()

# 查找ROS2包 (可选)
find_package(ament_cmake QUIET)
find_package(rclcpp QUIET)
find_package(std_msgs QUIET)

if(rclcpp_FOUND AND std_msgs_FOUND)
    message(STATUS "找到ROS2，启用ROS2支持")
    set(ROS2_FOUND TRUE)
    add_definitions(-DUSE_ROS2)
else()
    message(STATUS "未找到ROS2，禁用ROS2支持")
    set(ROS2_FOUND FALSE)
endif()

find_library(NCURSES_LIBRARY 
    NAMES ncurses
    REQUIRED
)

# 包含目录
include_directories(${OPENHAPTICS_INCLUDE_DIR})

# 如果找到Python，添加Python头文件路径
if(Python3_FOUND)
    include_directories(${Python3_INCLUDE_DIRS})
endif()

# 源文件
set(SOURCES
    Touch_Controller_Arm2.cpp
    conio.c
)

# 创建可执行文件
add_executable(Touch_Controller_Arm2 ${SOURCES})

# 链接库
target_link_libraries(Touch_Controller_Arm2 
    ${HD_LIBRARY}
    ${HDU_LIBRARY}
    ${NCURSES_LIBRARY}
    Threads::Threads
    rt
)

# 如果找到Python，链接Python库
if(Python3_FOUND)
    target_link_libraries(Touch_Controller_Arm2 ${Python3_LIBRARIES})
endif()

# 如果找到ROS2，添加ROS2依赖
if(ROS2_FOUND)
    ament_target_dependencies(Touch_Controller_Arm2 rclcpp std_msgs)
    message(STATUS "ROS2支持已启用")
    
    # 安装规则（ROS2风格）
    install(TARGETS Touch_Controller_Arm2
        DESTINATION lib/${PROJECT_NAME}
    )
    
    # 导出包信息
    ament_package()
else()
    # 传统安装规则
    install(TARGETS Touch_Controller_Arm2
        RUNTIME DESTINATION bin
    )
endif()

# 设置输出目录 - 仅在非ROS2环境下设置
if(NOT ROS2_FOUND)
    set_target_properties(Touch_Controller_Arm2 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

# 配置文件复制
configure_file(config.ini ${CMAKE_CURRENT_SOURCE_DIR}/build/config.ini COPYONLY)

# 安装配置文件
if(NOT ROS2_FOUND)
    install(FILES config.ini
        DESTINATION etc
    )

    install(FILES 
        "双设备控制说明.md"
        "双Touch设备使用说明.md"
        "加载说明.md"
        "ROS2集成说明.md"
        DESTINATION share/doc/dual-touch-arm-controller
    )
endif()

# 创建软件包信息
set(CPACK_GENERATOR "DEB;TGZ")
set(CPACK_PACKAGE_NAME "dual-touch-arm-controller")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_CONTACT "ruio@example.com")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libncurses5-dev")

include(CPack)

# 自定义目标
add_custom_target(run
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Touch_Controller_Arm2
    DEPENDS Touch_Controller_Arm2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "运行双Touch设备双机械臂控制程序"
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E remove -f Touch_Controller_Arm2
    COMMAND ${CMAKE_COMMAND} -E remove -f conio.o
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "清理所有生成文件"
)

# 检查配置
add_custom_target(check-config
    COMMAND ${CMAKE_COMMAND} -E echo "检查项目配置:"
    COMMAND ${CMAKE_COMMAND} -E echo "  OpenHaptics路径: ${OPENHAPTICS_ROOT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  HD库: ${HD_LIBRARY}"
    COMMAND ${CMAKE_COMMAND} -E echo "  HDU库: ${HDU_LIBRARY}"
    COMMAND ${CMAKE_COMMAND} -E echo "  ncurses库: ${NCURSES_LIBRARY}"
    COMMAND ${CMAKE_COMMAND} -E echo "  编译类型: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "  C++标准: ${CMAKE_CXX_STANDARD}"
    COMMENT "显示项目配置信息"
)

# 显示配置信息
message(STATUS "=== 双Touch设备双机械臂控制系统 CMake 配置 ===")
message(STATUS "项目名称: ${PROJECT_NAME}")
message(STATUS "项目版本: ${PROJECT_VERSION}")
message(STATUS "编译类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenHaptics路径: ${OPENHAPTICS_ROOT_DIR}")
message(STATUS "HD库: ${HD_LIBRARY}")
message(STATUS "HDU库: ${HDU_LIBRARY}")
message(STATUS "ncurses库: ${NCURSES_LIBRARY}")
message(STATUS "===============================================") 