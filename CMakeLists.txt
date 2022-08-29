cmake_minimum_required(VERSION 3.5)
project(coroutine-cpp)

add_definitions(-DDEV_DEBUGGING)

set(CMAKE_VERBOSE_MAKEFILE ON)
# -O3的编译优化等级有好处也有坏处
# -fno-stack-protector编译器关闭栈保护，默认应该是打开，最好不要关
# -Wno-xxx表示排除某些类型的警告
# -Werror表示将所有警告当作错误来处理
#－rdynamic指示链接器将所有符号都添加到动态符号表中，以便dlopen, backtrace这样的函数使用
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -g -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

set(LIB_SRC
    src/singleton.hpp
    src/containers.hpp
    src/utils.cpp
    src/thread.cpp
    src/mutexes.cpp
    src/coroutine.cpp
    src/coexecutor.cpp
    src/coscheduler.cpp
    src/threadpool.cpp)

include_directories(${PROJECT_SOURCE_DIR}/src)

# 添加可执行文件函数
function(ahri_add_executable targetname srcname depends libs)
    add_executable(${targetname} ${srcname})
    add_dependencies(${targetname} ${depends})
    target_link_libraries(${targetname} ${libs})
endfunction(ahri_add_executable)


include_directories(.)
# 可以将其打包成动态库
add_library(cocpp SHARED ${LIB_SRC})
set(LIBS
    cocpp
    rt
    pthread)

# 添加测试可执行文件
ahri_add_executable(test_utils tests/test_utils.cpp "cocpp" "${LIBS}")
ahri_add_executable(test_coroutine tests/test_coroutine.cpp "cocpp" "${LIBS}")
ahri_add_executable(test_coexecutor tests/test_coexecutor.cpp "cocpp" "${LIBS}")
ahri_add_executable(test_containers tests/test_containers.cpp "cocpp" "${LIBS}")
ahri_add_executable(test_cosched tests/test_cosched.cpp "cocpp" "${LIBS}")
ahri_add_executable(test_threadpool tests/test_threadpool.cpp "cocpp" "${LIBS}")


# 设置可执行文件的输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置.so库文件的输出路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)  
