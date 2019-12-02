cmake_minimum_required(VERSION 3.15)
project(OpenGL_CLion_MacOS)

set(CMAKE_CXX_STANDARD 20)
# 添加头文件
set(GLEW_H /usr/local/Cellar/glew/2.1.0/include/GL)
set(GLFW_H /usr/local/Cellar/glfw/3.3/include)
set(GLAD_H /usr/local/Cellar/glad)
set(GLM_H /usr/local/Cellar/glm/0.9.9.5/include) # 增加了glmath库的支持
set(ASSIMP_H /usr/local/Cellar/assimp/5.0.0/include/assimp)
include_directories(${GLEW_H} ${GLFW_H} ${GLAD_H} ${GLM_H} ${ASSIMP_H})

# 添加目标链接
set(GLEW_LINK /usr/local/Cellar/glew/2.1.0/lib/libGLEW.2.1.dylib)
set(GLFW_LINK /usr/local/Cellar/glfw/3.3/lib/libglfw.3.3.dylib)
set(ASSIMP_LINK /usr/local/Cellar/assimp/5.0.0/lib/libassimp.5.0.0.dylib)
link_libraries(${OPENGL} ${GLEW_LINK} ${GLFW_LINK} ${ASSIMP_LINK})

# 执行编译命令
# 这个glad.c随意你放在哪，要注意路径就好
set(SOURCE_FILES "/usr/local/Cellar/glad/src/glad.c" "src/main.cpp" "include/stb_image/stb_image.h"
        "include/stb_image/stb_image_implementation.cpp" "include/Camera.h" "include/Mesh.h" "include/Model.h")
add_executable(OpenGL_CLion_MacOS ${SOURCE_FILES})
if (APPLE)
    target_link_libraries(OpenGL_CLion_MacOS "-framework OpenGL")
    #target_link_libraries(OpenGL_CLion_MacOS "-framework GLUT")
endif()