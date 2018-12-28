cmake_minimum_required(VERSION 3.12)
project(OpenGL_CLion_MacOS)

set(CMAKE_CXX_STANDARD 17)
# 添加头文件
set(GLEW_H /usr/local/Cellar/glew/2.1.0/include/GL)
set(GLFW_H /usr/local/Cellar/glfw/3.2.1/include/GLFW)
set(GLAD_H /usr/local/Cellar/glad)
set(GLM_H /usr/local/Cellar/glm) # 增加了glmath库的支持
include_directories(${GLEW_H} ${GLFW_H} ${GLAD_H} ${GLM_H})

# 添加目标链接
set(GLEW_LINK /usr/local/Cellar/glew/2.1.0/lib/libGLEW.2.1.dylib)
# 推荐对GitHub上的glfw库clone然后进行本地源码编译安装，得到3.3的dylib，brew下载的库直接使用，可能会在执行glfwCreateWindow()时报错
# 参考情况：https://github.com/glfw/glfw/issues/908
# 安装方法：https://my.oschina.net/freeblues/blog/687630
#set(GLFW_LINK /usr/local/Cellar/glfw/3.2.1/lib/libglfw.3.dylib)
set(GLFW_LINK /usr/local/lib/libglfw.3.3.dylib) 
link_libraries(${OPENGL} ${GLEW_LINK} ${GLFW_LINK})

# 执行编译命令
# 这个glad.c随意你放在哪，要注意路径就好
set(SOURCE_FILES "/usr/local/Cellar/glad/src/glad.c" "main.cpp")
add_executable(OpenGL_CLion_MacOS ${SOURCE_FILES})
if (APPLE)
    target_link_libraries(OpenGL_CLion_MacOS "-framework OpenGL")
    #target_link_libraries(OpenGL_CLion_MacOS "-framework GLUT")
endif()