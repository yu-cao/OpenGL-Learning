# OpenGL-Learning
This is my learning of OpenGL following https://learnopengl.com/<br>
The Chinese Edition is https://learnopengl-cn.github.io/

这个环境经过测试能够在当前最新版本下（Mojave）顺利运行

您需要之前先下载brew，之后使用brew执行以下命令下载支持库

```bash
brew install assimp
brew install glad
brew install glew
brew install glm
```

特别要注意：推荐对GitHub上的glfw库clone然后进行本地源码编译安装，得到3.3的dylib，如果使用brew下载的库直接使用，得到的是3.2.1的release版，踩坑发现在调试/运行时可能会在执行glfwCreateWindow()时报错

参考情况：https://github.com/glfw/glfw/issues/908 <br>
安装方法：https://my.oschina.net/freeblues/blog/687630

进一步的详细配置可见我的CMakeLists.txt，有比较详细的注释
