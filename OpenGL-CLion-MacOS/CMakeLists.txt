cmake_minimum_required(VERSION 3.12)
project(OpenGL_CLion_MacOS)

set(CMAKE_CXX_STANDARD 17)
# 添加头文件
set(GLEW_H /usr/local/Cellar/glew/2.1.0/include/GL)
set(GLFW_H /usr/local/Cellar/glfw/3.2.1/include/GLFW)
set(GLAD_H /usr/local/Cellar/glad)
include_directories(${GLEW_H} ${GLFW_H} ${GLAD_H})

# 添加目标链接
set(GLEW_LINK /usr/local/Cellar/glew/2.1.0/lib/libGLEW.2.1.dylib)
set(GLFW_LINK /usr/local/Cellar/glfw/3.2.1/lib/libglfw.3.dylib)
link_libraries(${OPENGL} ${GLEW_LINK} ${GLFW_LINK})

# 执行编译命令
# 这个glad.c随意你放在哪，要注意路径就好
set(SOURCE_FILES "/usr/local/Cellar/glad/src/glad.c" "main.cpp")
add_executable(OpenGL_CLion_MacOS ${SOURCE_FILES})
if (APPLE)
    target_link_libraries(OpenGL_CLion_MacOS "-framework OpenGL")
    #target_link_libraries(OpenGL_CLion_MacOS "-framework GLUT")
endif()