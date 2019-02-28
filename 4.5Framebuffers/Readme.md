<h2>帧缓冲</h2>

用于写入颜色值的颜色缓冲、用于写入深度信息的深度缓冲和允许我们根据一些条件丢弃特定片段的模板缓冲。这些缓冲结合起来叫做帧缓冲(Framebuffer)，它被储存在内存中；OpenGL允许我们定义我们自己的帧缓冲

我们目前所做的所有操作都是在默认帧缓冲的渲染缓冲上进行的。默认的帧缓冲是在你创建窗口的时候生成和配置的（GLFW帮我们做了这些）。有了我们自己的帧缓冲，我们就能够有更多方式来渲染了。

<hr>

<h3>创建一个帧缓冲</h3>

一个完整的帧缓冲需要满足以下的条件：

+ 附加至少一个缓冲（颜色、深度或模板缓冲）
+ 至少有一个颜色附件(Attachment)
+ 所有的附件都必须是完整的（保留了内存）
+ 每个缓冲都应该有相同的样本数

```cpp
unsigned int fbo;
glGenFramebuffers(1, &fbo);
```

首先我们创建一个帧缓冲对象，将它绑定为激活的(Active)帧缓冲，做一些操作，之后解绑帧缓冲。

```cpp
glBindFramebuffer(GL_FRAMEBUFFER, fbo);
```

**在绑定到`GL_FRAMEBUFFER`目标之后，所有的读取和写入帧缓冲的操作将会影响当前绑定的帧缓冲。**我们也可以使用`GL_READ_FRAMEBUFFER`或`GL_DRAW_FRAMEBUFFER`，将一个帧缓冲分别绑定到读取目标或写入目标。绑定到`GL_READ_FRAMEBUFFER`的帧缓冲将会使用在所有像是`glReadPixels`的读取操作中，而绑定到`GL_DRAW_FRAMEBUFFER`的帧缓冲将会被用作渲染、清除等写入操作的目标。大部分情况你都不需要区分它们，通常都会使用`GL_FRAMEBUFFER`，绑定到两个上。

从上面的条件中可以知道，我们需要为帧缓冲创建一些附件，并将附件附加到帧缓冲上。在完成所有的条件之后，我们可以以`GL_FRAMEBUFFER`为参数调用`glCheckFramebufferStatus`，检查帧缓冲是否完整。它将会检测当前绑定的帧缓冲，并返回规范中这些值的其中之一。如果它返回的是`GL_FRAMEBUFFER_COMPLETE`，帧缓冲就是完整的了。

```cpp
if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	//...
```

**之后所有的渲染操作将会渲染到当前绑定帧缓冲的附件中。由于我们的帧缓冲不是默认帧缓冲，渲染指令将不会对窗口的视觉输出有任何影响。**出于这个原因，渲染到一个不同的帧缓冲被叫做离屏渲染(Off-screen Rendering)。**要保证所有的渲染操作在主窗口中有视觉效果，我们需要再次激活默认帧缓冲，将它绑定到0**

```cpp
glBindFramebuffer(GL_FRAMEBUFFER, 0);//再次激活默认帧缓冲，将它绑定到0
glDeleteFramebuffers(1, &fbo);//完成所有的帧缓冲操作之后，要删除这个帧缓冲对象
```

<hr>

完整性检查之前，我们要给帧缓冲一个附件，作为帧缓冲的一个缓冲，可以想象为一个图像，这个附件有两个选项：纹理附件/渲染缓冲对象附件

<h3>纹理附件</h3>

当把一个纹理附加到帧缓冲的时候，所有的渲染指令将会写入到这个纹理中，就想它是一个普通的颜色/深度或模板缓冲一样。使用纹理的优点是，所有渲染操作的结果将会被储存在一个纹理图像中，我们之后可以在着色器中很方便地使用它。

```cpp
//为帧缓冲创建一个纹理和创建一个普通的纹理差不多
//主要的区别就是，我们将维度设置为了屏幕大小（尽管这不是必须的），并且我们给纹理的data参数传递了NULL
unsigned int texture;
glGenTextures(1, &texture);
glBindTexture(GL_TEXTURE_2D, texture);

glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
```

最后一件事就是将它附加到帧缓冲上了

```cpp
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
```

`glFrameBufferTexture2D`有以下的参数：

+ `target`：帧缓冲的目标（绘制、读取或者两者皆有）
+ `attachment`：我们想要附加的附件类型。当前我们正在附加一个颜色附件。注意最后的0意味着我们可以附加多个颜色附件（如果深度缓冲：`GL_DEPTH_ATTACHMENT`，如果模板缓冲：`GL_STENCIL_ATTACHMENT`，深度和模板两者结合为一个也可以，见下面一段）
+ `textarget`：希望附加的纹理类型（如果模板缓冲：`GL_STENCIL_INDEX`）
+ `texture`：要附加的纹理本身
+ `level`：多级渐远纹理的级别。我们将它保留为0。

也可以深度缓冲和模板缓冲附加为一个单独的纹理。纹理的每32位数值将包含24位的深度信息和8位的模板信息。要将深度和模板缓冲附加为一个纹理的话，我们使用`GL_DEPTH_STENCIL_ATTACHMENT`类型，并配置纹理的格式，让它包含合并的深度和模板值：

```cpp
glTexImage2D(
  GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0, 
  GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
);

glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
```

<h3>渲染缓冲对象附件</h3>

和纹理图像一样，渲染缓冲对象是一个真正的缓冲，即一系列的字节、整数、像素等。渲染缓冲对象附加的好处是，**它会将数据储存为OpenGL原生的渲染格式，它是为离屏渲染到帧缓冲优化过的。**

渲染缓冲对象直接将所有的渲染数据储存到它的缓冲中，不会做任何针对纹理格式的转换，让它变为一个更快的可写储存介质。然而，**渲染缓冲对象通常都是只写的，所以你不能读取它们**（比如使用纹理访问）。当然你仍然还是能够使用`glReadPixels`来读取它，这会从当前绑定的帧缓冲，而不是附件本身，中返回特定区域的像素。

它的数据已经是原生的格式了，当写入或者复制它的数据到其它缓冲中时是非常快的；交换缓冲这样的操作在使用渲染缓冲对象时会非常快；**原来我们在每个渲染迭代最后使用的`glfwSwapBuffers`，这也可以通过渲染缓冲对象实现：只需要写入一个渲染缓冲图像，并在最后交换到另外一个渲染缓冲就可以了。**渲染缓冲对象对这种操作非常完美。

创建一个渲染缓冲对象的代码和帧缓冲的代码很类似：

```cpp
unsigned int rbo;
glGenRenderbuffers(1, &rbo);
glBindRenderbuffer(GL_RENDERBUFFER, rbo);//进行缓冲对象绑定
```

**渲染缓冲对象一般都是只写的，会经常用于深度和模板附件，因为大部分时间我们都只关心深度和模板测试**，不需要从深度和模板缓冲中读取值。我们**需要深度和模板值用于测试，但不需要对它们进行采样，所以渲染缓冲对象非常适合它们**。当我们不需要从这些缓冲中采样的时候，通常都会选择渲染缓冲对象，因为它会更优化一点。

```cpp
glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600);//创建一个深度和模板渲染缓冲对象
```

创建一个渲染缓冲对象和纹理对象类似，不同的是这个对象是专门被设计作为图像使用的，而不是纹理那样的通用数据缓冲(General Purpose Data Buffer)。这里我们选择`GL_DEPTH24_STENCIL8`作为内部格式，它封装了24位的深度和8位的模板缓冲。

最后是附加这个渲染缓冲对象

```cpp
glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
```

**选择：常的规则是，如果你不需要从一个缓冲中采样数据，那么对这个缓冲使用`渲染缓冲对象`会是明智的选择。如果你需要从缓冲中采样颜色或深度值等数据，那么你应该选择`纹理附件`**

<hr>

<h3>实践：渲染到纹理</h3>

将场景渲染到一个附加到帧缓冲对象上的颜色纹理中，之后将在一个横跨整个屏幕的四边形上绘制这个纹理。（这样视觉输出和没使用帧缓冲时是完全一样的，但这次是打印到了一个四边形上，方便了后处理）

后处理的Screen片元着色器可以通过简单操作达到反相，灰度化等效果，也可以通过卷积核的方法做出锐化，模糊，边缘检测等效果

**以上部分都通过代码提供**

但是需要注意的是：核在对屏幕纹理的边缘进行采样的时候，由于还会对中心像素周围的8个像素进行采样，其实会取到纹理之外的像素。由于环绕方式默认是`GL_REPEAT`，所以在没有设置的情况下取到的是屏幕另一边的像素，而另一边的像素本不应该对中心像素产生影响，这就可能会在屏幕边缘产生很奇怪的条纹。为了消除这一问题，我们可以将屏幕纹理的环绕方式都设置为`GL_CLAMP_TO_EDGE`。这样子在取到纹理外的像素时，就能够重复边缘的像素来更精确地估计最终的值了。