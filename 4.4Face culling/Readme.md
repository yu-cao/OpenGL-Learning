<h2>面剔除</h2>

我们看一个正方体，最多只能看到3个面，如果我们在绘制的时候直接丢弃看不到的3个面，我们就能大大提升性能

OpenGL能够检查所有面向观察者的面，并渲染它们，而丢弃那些背向的面，节省了大量的片段着色器调用。但我们仍要告诉OpenGL哪些面是正向面，哪些面是背向面。OpenGL使用了一个很聪明的技巧，分析顶点数据的环绕顺序(Winding Order)。

<h3>环绕顺序</h3>

当我们定义一组三角形顶点时，我们会以特定的环绕顺序来定义它们，可能是顺时针(Clockwise)的，也可能是逆时针(Counter-clockwise)的。每个三角形由3个顶点所组成，我们会从三角形中间来看，为这3个顶点设定一个环绕顺序。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/4.4Face%20culling/Reference/faceculling_windingorder.png)

```cpp
float vertices[] = {
    // 顺时针
    vertices[0], // 顶点1
    vertices[1], // 顶点2
    vertices[2], // 顶点3
    // 逆时针
    vertices[0], // 顶点1
    vertices[2], // 顶点3
    vertices[1]  // 顶点2  
};
```

OpenGL在渲染图元的时候将使用这个信息来决定一个三角形是一个正向三角形还是背向三角形。**默认情况下，逆时针顶点所定义的三角形将会被处理为正向三角形。**

**当你定义顶点顺序的时候，你应该想象对应的三角形是面向你的，所以你定义的三角形从正面看去应该是逆时针的。**这样定义顶点很棒的一点是，实际的环绕顺序是在光栅化阶段进行的，也就是顶点着色器运行之后。这些顶点就是从观察者视角所见的了。

观察者所面向的所有三角形顶点就是我们所指定的正确环绕顺序了，而立方体另一面的三角形顶点则是以相反的环绕顺序所渲染的。这样的结果就是，我们所面向的三角形将会是正向三角形，而背面的三角形则是背向三角形。下面这张图显示了这个效果：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/4.4Face%20culling/Reference/faceculling_frontback.png)

虽然背面的三角形是以逆时针定义的，但是从观察者当前视角它现在是以顺时针顺序，这正是我们想要剔除（Cull，丢弃）的不可见面

<h3>面剔除</h3>

**OpenGL默认是禁用面剔除选项的**

```cpp
glEnable(GL_CULL_FACE);//启用面剔除
```

注意：这只对像立方体这样的封闭形状有效。当我们想要绘制上一节中的草时，我们必须要再次禁用面剔除，因为它们的正向面和背向面都应该是可见的。

OpenGL允许我们改变需要剔除的面的类型:

```cpp
glCullFace(GL_BACK);//剔除背向面（默认值）
glCullFace(GL_FRONT);//剔除正向面而不是背向面
```

除了需要剔除的面之外，我们也可以通过调用glFrontFace，告诉OpenGL我们希望将顺时针的面（而不是逆时针的面）定义为正向面：

```cpp
glFrontFace(GL_CCW);
```

默认值是`GL_CCW`，它代表的是逆时针的环绕顺序，另一个选项是`GL_CW`，它（显然）代表的是顺时针顺序。

告诉OpenGL现在顺时针顺序代表的是正向面：

```cpp
glEnable(GL_CULL_FACE);
glCullFace(GL_BACK);
glFrontFace(GL_CW);
```

这样的结果是只有背向面被渲染了