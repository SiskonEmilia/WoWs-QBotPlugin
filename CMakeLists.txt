cmake_minimum_required(VERSION 3.9)

project(WoWsQBotPlugin)

set(CMAKE_CXX_STANDARD 17)
set(CQ_DEV_PLUGIC_DIR "F:/iWarships/CoolQAir/")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Use the code below only if you found "MD-MT conflict" in linking stage
# if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
#     message("You are using MSVC compiler")
#     # Use the static C library for all build types
#     foreach(var 
#         CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
#         CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
#         CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
#         CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
#       )
#       if(${var} MATCHES "/MD")
#         string(REGEX REPLACE "/MD" "/MT" ${var} "${${var}}")
#       endif()
#     endforeach()  
# else()
#     message("You are not using MSVC compiler, pay attention to archive tool.")
# endif()

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# import necessary Qt5 core & its components
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# include cqcppsdk
include(extern/cqcppsdk/cqcppsdk.cmake) 

# read app_id from app_id.txt
file(READ "app_id.txt" APP_ID)
string(STRIP "${APP_ID}" APP_ID)
cq_set_app_id(${APP_ID}) # 设置 app id, 必须


# Qt Executable Command
# add_executable(helloworld
#     mainwindow.ui
#     mainwindow.cpp
#     main.cpp
#     # resources.qrc # Qt Resource File, add if needed
# )

# include rapidjson
include_directories(include)
# set src as include 
include_directories(src)
# import all files in src
file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

if (CQ_CAN_BUILD_STD_MODE)
    # set build target
    cq_add_std_app(${SOURCE_FILES})
    # link with Qt5
    target_link_libraries(app Qt5::Widgets)
    # if you need to install it due to cmake setting
    cq_install_std_app("${CQ_DEV_PLUGIC_DIR}")
else()
    message("ERROR: cannot build in std mode, finish with nothing built")
endif ()
