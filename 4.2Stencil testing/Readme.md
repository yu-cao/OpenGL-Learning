<h2>模板测试</h2>

当片段着色器处理完一个片段之后，模板测试(Stencil Test)会开始执行，和深度测试一样，它也可能会丢弃片段。接下来，被保留的片段会进入深度测试，它可能会丢弃更多的片段。模板测试是根据又一个缓冲来进行的，它叫做模板缓冲(Stencil Buffer)，可以在渲染的时候更新它来获得一些很有意思的效果。

一个模板缓冲中，（通常）每个模板值(Stencil Value)是8位的。所以每个像素/片段一共能有256种不同的模板值。我们可以将这些模板值设置为我们想要的值，然后当某一个片段有某一个模板值的时候，我们就可以选择丢弃或是保留这个片段了。

模板缓冲操作允许我们在渲染片段时将模板缓冲设定为一个特定的值。通过在渲染时修改模板缓冲的内容，我们写入了模板缓冲。在同一个（或者接下来的）渲染迭代中，我们可以读取这些值，来决定丢弃还是保留某个片段。使用模板缓冲的大体的步骤如下：

+ 启用模板缓冲的写入。
+ 渲染物体，更新模板缓冲的内容。
+ 禁用模板缓冲的写入。
+ 渲染（其它）物体，这次根据模板缓冲的内容丢弃特定的片段。

所以，通过使用模板缓冲，我们可以根据场景中已绘制的其它物体的片段，来决定是否丢弃特定的片段。

这个测试跟深度测试一样，都要先通过`glEnabel(GL_STENCIL_TEST);`才能够启动，而且每次迭代前也要通过`glClear()`清除模板缓冲

是否进行写入是通过跟掩码进行and操作决定，一般只用以下两个掩码操作

```cpp
glStencilMask(0xFF); // 每一位写入模板缓冲时都保持原样
glStencilMask(0x00); // 每一位在写入模板缓冲时都会变成0（禁用写入）
```

使用`glStencilFunc()`会描述OpenGL应该对模板内容做什么，它会设置一个模板测试函数，给出模板测试参考值和掩码，如果掩码后的结果经过测试函数符合参考值条件，就能够通过测试，否则就被丢弃，如`GL_LESS`等，在这里你也可以给它赋值，如`GL_ALWAYS`，`GL_NEVER`等

然后使用`glStencilOp()`这个函数依次给出如果**模板测试失败**，**模板测试通过，深度测试失败**，**都通过**这三种情况下对模板缓冲的行为<br>
是继续保持当前的存储值(`GL_KEEP`)？还是将模板值设置为`glStencilFunc`函数设置的ref值(`GL_REPLACE`)？还是其他...默认情况是`glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP)`，也就是不论任何测试的结果是如何，模板缓冲都会保留它的值，所以如果想写入模板缓冲的话，至少需要对其中一个选项设置不同的值

<hr>

<h3>应用：比如我们可以实现物体轮廓的绘制：</h3>

+ 先通过`glStencilFunc(GL_ALWAYS, 1, 0xFF)`和`glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE)`配合，保证了箱子的每个片段都会将模板缓冲的模板值更新为1。因为片段永远会通过模板测试，在绘制片段的地方，模板缓冲会被更新为参考值。
+ 然后再绘制放大的箱子，但是只绘制模板值不为1的部分，同时禁用深度测试，让边框不会被地板覆盖

整体流程如下：

```cpp
glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  

glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); 

// 先把地板画出来
glStencilMask(0x00); // 记得保证我们在绘制地板的时候不会更新模板缓冲
normalShader.use();
DrawFloor()  

// 再把两个箱子画出来，同时更新画过箱子地方的模板缓冲
glStencilFunc(GL_ALWAYS, 1, 0xFF); 
glStencilMask(0xFF); 
DrawTwoContainers();

// 绘制放大的箱子，但是只绘制模板值不为1的部分
glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
glStencilMask(0x00); 
glDisable(GL_DEPTH_TEST);//禁用深度测试，让边框不会被地板覆盖
shaderSingleColor.use(); 
DrawTwoScaledUpContainers();
// 重新放开，准备绘制下一帧
glStencilMask(0xFF);
glEnable(GL_DEPTH_TEST);  
```