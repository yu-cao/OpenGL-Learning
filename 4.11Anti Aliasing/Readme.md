<h2>抗锯齿</h2>

能够清楚看见形成边缘的像素。这种现象被称之为走样(Aliasing)。我们的目标是抗锯齿（Anti-aliasing，也被称为反走样）缓解这种现象，从而产生更平滑的边缘。

1. 超采样抗锯齿(Super Sample Anti-aliasing, SSAA)的技术，它会使用比正常分辨率更高的分辨率（即超采样）来渲染场景，当图像输出在帧缓冲中更新时，分辨率会被下采样(Downsample)至正常的分辨率。
2. 多重采样抗锯齿(Multisample Anti-aliasing, MSAA)：只是在光栅化阶段，判断一个三角形是否被像素覆盖的时候会计算多个覆盖样本（Coverage sample），但是在pixel shader着色阶段计算像素颜色的时候每个像素还是只计算一次。

<hr>

<h3>MSAA</h3>

由于屏幕像素总量的限制，有些边缘的像素能够被渲染出来，而有些则不会。结果就是我们使用了不光滑的边缘来渲染图元，导致锯齿边缘。

MSAA所做的正是将单一的采样点变为多个采样点（这也是它名称的由来）。我们不再使用像素中心的单一采样点，取而代之的是以特定图案排列的4个子采样点(Subsample)。我们将用这些子采样点来决定像素的遮盖度。当然，这也意味着颜色缓冲的大小会随着子采样点的增加而增加。（采样点的数量可以是任意的，更多的采样点能带来更精确的遮盖率）

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/4.11Anti%20Aliasing/Reference/anti_aliasing_sample_points.png)

MSAA真正的工作方式是，无论三角形遮盖了多少个子采样点，（每个图元中）每个像素只运行一次片段着色器。片段着色器所使用的顶点数据会插值到每个像素的中心，所得到的结果颜色会被储存在每个被遮盖住的子采样点中。当颜色缓冲的子样本被图元的所有颜色填满时，所有的这些颜色将会在每个像素内部平均化。因为上图的4个采样点中只有2个被遮盖住了，这个像素的颜色将会是三角形颜色与其他两个采样点的颜色（在这里是无色）的平均值，最终形成一种淡蓝色。颜色缓冲中所有的图元边缘将会产生一种更平滑的图形。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/4.11Anti%20Aliasing/Reference/anti_aliasing_rasterization_samples.png)

MSAA的一个问题就是和现在大街小巷都是的deferred shading框架并不是那么兼容。因为用deferred shading的时候场景都先被光栅化到GBuffer上去了，不直接做shading。（文刀秋二）

<hr>

<h3>OpenGL中的MSAA</h3>

根据MSAA的原理，需要使用一个能在每个像素中存储大于1个颜色值的颜色缓冲（因为多重采样需要我们为每个采样点都储存一个颜色）。所以，我们需要一个新的缓冲类型，来存储特定数量的多重采样样本，它叫做多重采样缓冲(Multisample Buffer)。

GLFW同样给了我们这个功能，我们所要做的只是提示(Hint) GLFW，我们希望使用一个包含N个样本的多重采样缓冲。这可以在创建窗口之前调用glfwWindowHint来完成。然后显式调用一下启动多重采样缓冲

```cpp
glfwWindowHint(GLFW_SAMPLES, 4);

glEnable(GL_MULTISAMPLE);
```

<hr>

<h3>离屏MSAA</h3>

使用我们自己的帧缓冲来进行离屏渲染，就必须要手动生成多重采样缓冲

<h4>创建多重采样纹理附件</h4>

两种方式可以创建多重采样缓冲，将其作为帧缓冲的附件：纹理附件和渲染缓冲附件（这些跟Framebuffers节中的普通附件很像）

1、多重采样纹理附件

使用`glTexImage2DMultisample`来替代`glTexImage2D`，它的纹理目标是`GL_TEXTURE_2D_MULTISAPLE`。第二个参数设置的是纹理所拥有的样本个数。如果最后一个参数为GL_TRUE，图像将会对每个纹素使用相同的样本位置以及相同数量的子采样点个数。

```cpp
glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
```

使用`glFramebufferTexture2D`将多重采样纹理附加到帧缓冲上，但这里纹理类型使用的是`GL_TEXTURE_2D_MULTISAMPLE`：

```cpp
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex, 0);
```

当前绑定的帧缓冲现在就有了一个纹理图像形式的多重采样颜色缓冲。

2、多重采样渲染缓冲对象

在指定（当前绑定的）渲染缓冲的内存存储时，将`glRenderbufferStorage`的调用改为`glRenderbufferStorageMultisample`就可以了。函数中，渲染缓冲对象后的参数我们将设定为样本的数量，在当前的例子中是4。

```cpp
glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
```

<h4>渲染到多重采样帧缓冲</h4>

一个多重采样的图像包含比普通图像更多的信息，我们所要做的是缩小或者还原(Resolve)图像。多重采样帧缓冲的还原通常是通过`glBlitFramebuffer`来完成，它能够将一个帧缓冲中的某个区域复制到另一个帧缓冲中，并且将多重采样缓冲还原。

当我们绑定到`GL_FRAMEBUFFER`时，我们是同时绑定了读取和绘制的帧缓冲目标。我们也可以将帧缓冲分开绑定至`GL_READ_FRAMEBUFFER`与`GL_DRAW_FRAMEBUFFER`。`glBlitFramebuffer`函数会根据这两个目标，决定哪个是源帧缓冲，哪个是目标帧缓冲。接下来，我们可以将图像位块传送(Blit)到默认的帧缓冲中，将多重采样的帧缓冲传送到屏幕上。

```cpp
glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampledFBO);
glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
```

[源代码](https://learnopengl.com/code_viewer_gh.php?code=src/4.advanced_opengl/11.anti_aliasing_offscreen/anti_aliasing_offscreen.cpp)参考

我们还可以自定义采样，如果将多重采样与离屏渲染结合起来，我们需要自己负责一些额外的细节。但所有的这些细节都是值得额外的努力的，因为多重采样能够显著提升场景的视觉质量。当然，要注意，如果使用的采样点非常多，启用多重采样会显著降低程序的性能。在本节写作时，通常采用的是4采样点的MSAA。

这部分的离屏MSAA有点懵逼，需要再好好研读一下...