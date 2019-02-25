<h2>混合</h2>

OpenGL中，混合(Blending)通常是实现物体透明度(Transparency)的一种技术；透明的物体可以是完全透明的（让所有的颜色穿过），或者是半透明的（它让颜色通过，同时也会显示自身的颜色）。一个物体的透明度是通过它颜色的aplha值来决定的。alpha = 0.0 - 完全透明

我们目前一直使用的纹理有三个颜色分量：RGB。但一些材质会有一个内嵌的alpha通道，对每个纹素(Texel)都包含了一个alpha值。这个alpha值精确地告诉我们纹理各个部分的透明度。

<hr>

但是还有一种技术：通过测试，决定是否丢弃某个片段，使之只有完全透明或者完全不透明的纹理的透明度。例如，下面这个草我们只希望在场景中看到草的部分，而不是整个方形图像，所以我们就需要**丢弃**这个图显示的纹理中透明部分的片段，不将这些片段存到颜色缓冲中

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/4.3Blending/Reference/grass.png)

<hr>

现在，我们开始处理半透明的图像，首先需要`glEnable(GL_BLEND);`命令来启用混合

`glBlendFunc(GLenum sfactor, GLenum dfactor)`函数接受两个参数，来设置源和目标因子。

混合方程为：

<img src="http://latex.codecogs.com/svg.latex?\overline{C}_{\text{result}}=\overline{C}_{\text{source}}*F_{\text{source}}+\overline{C}_{\text{destination}}*F_{\text{destination}}" />

**使用源颜色向量（纹理中的颜色）的alpha作为源因子，使用1−alpha作为目标因子（目标颜色是存在颜色缓冲中的颜色）。**这将会产生以下的glBlendFunc：

```cpp
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

也可以使用glBlendFuncSeparate为RGB和alpha通道分别设置不同的选项：

```cpp
glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
```

所以渲染半透明的纹理只需要

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

但是，深度测试和混合一起使用的话会产生一些麻烦。当写入深度缓冲时，深度缓冲不会检查片段是否是透明的，所以透明的部分会和其它值一样写入到深度缓冲中。结果就是窗户的整个四边形不论透明度都会进行深度测试。即使透明的部分应该显示背后的窗户，深度测试仍然丢弃了它们。（也就是发生了透明没有连续的情况）

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/4.3Blending/Reference/blending_incorrect_order.png)

解决方法是需要保持住渲染的顺序

 + 先绘制所有不透明的物体
 + 对所有透明的物体排序
 + 按顺序绘制所有透明的物体

排序透明物体的一种方法是，从观察者视角获取物体的距离。这可以通过计算摄像机位置向量和物体的位置向量之间的距离所获得。接下来我们把距离和它对应的位置向量存储到一个map数据结构中。map会自动根据键值(Key)对它的值排序，所以只要我们添加了所有的位置，并以它的距离作为键，它们就会自动根据距离值排序，然后使用map的一个反向迭代器(Reverse Iterator)，反向遍历其中的条目，并将每个窗户四边形位移到对应的窗户位置上。这是排序透明物体的一个比较简单的实现，它能够修复之前的问题

但是现在并没有考虑旋转，缩放等问题，简单通过位置向量并不是完全正确的，可以考虑次序无关透明度(Order Independent Transparency, OIT)