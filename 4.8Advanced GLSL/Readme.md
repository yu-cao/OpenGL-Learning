<h2>高级GLSL</h2>

本节就是在组合使用OpenGL和GLSL创建程序时的一些最好要知道的东西，和一些会让你生活更加轻松的特性。

<hr>

<h3>内建变量</h3>

如果需要当前着色器以外地方的数据的话，我们必须要将数据传进来。我们已经学会使用顶点属性、uniform和采样器来完成这一任务

除此之外，GLSL还定义了另外几个以`gl_`为前缀的变量，它们能提供给我们更多的方式来读取/写入数据。我们已经在前面接触过其中的两个：顶点着色器的输出向量`gl_Position`，和片段着色器的`gl_FragCoord`。

`gl_Position`：顶点着色器的裁剪空间输出位置向量。如果你想在屏幕上显示任何东西，在顶点着色器中设置gl_Position是必须的步骤。这是它的全部功能。

<hr>

`gl_PointSize`

我们能够选用的其中一个图元是`GL_POINT`S，如果使用它的话，每一个顶点都是一个图元，都会被渲染为一个点。我们可以通过OpenGL的`glPointSize`函数来设置渲染出来的点的大小，但我们也可以在顶点着色器中修改这个值。

GLSL定义了一个叫做`gl_PointSize`输出变量，它是一个float变量，你可以使用它来设置点的宽高（像素）在顶点着色器中修改“点”大小的功能默认是禁用的，如果你需要启用它的话，你需要启用OpenGL的`GL_PROGRAM_POINT_SIZE`：

```cpp
glEnable(GL_PROGRAM_POINT_SIZE);
```

例子：将点的大小设置为裁剪空间位置的z值，也就是顶点距观察者的距离。点的大小会随着观察者距顶点距离变远而增大。

```glsl
void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);    
    gl_PointSize = gl_Position.z;    
}
```

这个在例子生成等技术上有一些有趣的应用

<hr>

`gl_VertexID`

一个输入变量，我们只能对它进行读取，储存了正在绘制顶点的当前ID，当（使用`glDrawElements`）进行索引渲染的时候，这个变量会存储正在绘制顶点的当前索引。当（使用`glDrawArrays`）不使用索引进行绘制的时候，这个变量会储存从渲染调用开始的已处理顶点数量。

<hr>

<h3>片元着色器变量</h3>

`gl_FragCoord`：它的z分量等于对应片段的深度值。x和y分量是片段的窗口空间(Window-space)坐标，其原点为窗口的左下角。如果使用`glViewport`设定了一个800x600的窗口，那么片段窗口空间坐标的x分量将在0到800之间，y分量在0到600之间。（获取当前片段的窗口空间坐标并得到它的深度值）

可以根据片段的窗口坐标，计算出不同的颜色。gl_FragCoord的一个常见用处是用于对比不同片段计算的视觉输出效果：将屏幕分成两部分，在窗口的左侧渲染一种输出，在窗口的右侧渲染另一种输出。

```glsl
void main()
{             
    if(gl_FragCoord.x < 400)//屏幕左侧物体通过这个片元显示为红色，右侧为绿色
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    else
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);        
}
```

`gl_FrontFacing`

OpenGL能够根据顶点的环绕顺序来决定一个面是正向还是背向面。如果我们不（启用`GL_FACE_CULL`来）使用面剔除，那么`gl_FrontFacing`将会告诉我们当前片段是属于正向面的一部分还是背向面的一部分。举例来说，我们能够对正向面计算出不同的颜色。

`gl_FrontFacing`变量是一个bool，如果当前片段是正向面的一部分那么就是true，否则就是false

创建一个立方体，在内部和外部使用不同的纹理，如果往箱子里面看（穿模），就能看到不同的纹理：

```glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D frontTexture;
uniform sampler2D backTexture;

void main()
{
    if(gl_FrontFacing)
        FragColor = texture(frontTexture, TexCoords);
    else
        FragColor = texture(backTexture, TexCoords);
}
```

`gl_FragDepth`

相比较刚刚的`gl_FragCoord`是只读变量，实际上修改片段的深度值还是可能的。GLSL提供给我们一个叫做`gl_FragDepth`的输出变量，我们可以使用它来在着色器内设置片段的深度值。

```glsl
gl_FragDepth = 0.0; // 这个片段现在的深度值为 0.0
```

如果着色器没有写入值到`gl_FragDepth`，它会自动取用gl_FragCoord.z的值。

由我们自己设置深度值有一个很大的缺点，只要我们在片段着色器中对`gl_FragDepth`进行写入，OpenGL就会禁用所有的提前深度测试(Early Depth Testing)。它被禁用的原因是，OpenGL无法在片段着色器运行之前得知片段将拥有的深度值，因为片段着色器可能会完全修改这个深度值。

在写入`gl_FragDepth`时，你就需要考虑到它所带来的性能影响。

<hr>

<h3>接口块</h3>

程序变得更大时，希望发送的可能就不只是几个变量了，它还可能包括数组和结构体。

为了帮助我们管理这些变量，GLSL为我们提供了一个叫做接口块(Interface Block)的东西，来方便我们组合这些变量。接口块的声明和struct的声明有点相像，不同的是，现在根据它是一个输入还是输出块(Block)，使用in或out关键字来定义的。

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT
{
    vec2 TexCoords;
} vs_out;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);    
    vs_out.TexCoords = aTexCoords;
}
```

之后，我们还需要在下一个着色器，即片段着色器，中定义一个输入接口块。块名(Block Name)应该是和着色器中一样的（`VS_OUT`），但实例名(Instance Name)（顶点着色器中用的是`vs_out`）可以是随意的，但要避免使用误导性的名称，比如对实际上包含输入变量的接口块命名为`vs_out`。

```glsl
#version 330 core
out vec4 FragColor;

in VS_OUT
{
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture;

void main()
{             
    FragColor = texture(texture, fs_in.TexCoords);   
}
```

只要两个接口块的名字一样，它们对应的输入和输出将会匹配起来。

<hr>

<h3>Uniform缓冲对象</h3>

**痛点：当使用多于一个的着色器时，尽管大部分的uniform变量都是相同的，我们还是需要不断地设置它们，这些冗余代码令人疲倦**

OpenGL为我们提供了一个叫做Uniform缓冲对象(Uniform Buffer Object)的工具，它允许我们定义一系列在多个着色器中相同的全局Uniform变量。当使用Uniform缓冲对象的时候，我们只需要设置相关的uniform一次。当然，我们仍需要手动设置每个着色器中不同的uniform。并且创建和配置Uniform缓冲对象会有一点繁琐。

因为Uniform缓冲对象仍是一个缓冲，我们可以使用`glGenBuffers`来创建它，将它绑定到`GL_UNIFORM_BUFFER`缓冲目标，并将所有相关的uniform数据存入缓冲。

首先，我们将使用一个简单的顶点着色器，将projection和view矩阵存储到所谓的Uniform块(Uniform Block)中

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;

layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

我们声明了一个叫做`Matrices`的Uniform块，它储存了两个4x4矩阵。Uniform块中的变量可以直接访问，不需要加块名作为前缀。接下来，我们在OpenGL代码中将这些矩阵值存入缓冲中，每个声明了这个Uniform块的着色器都能够访问这些矩阵。

`layout (std140)`这个语句是说，当前定义的Uniform块对它的内容使用一个特定的内存布局。这个语句设置了Uniform块布局(Uniform Block Layout)

<hr>

<h3>Uniform块布局</h3>

Uniform块的内容是储存在一个缓冲对象中的，它实际上只是一块预留内存。因为这块内存并不会保存它具体保存的是什么类型的数据，我们还需要告诉OpenGL内存的哪一部分对应着着色器中的哪一个uniform变量。

对于一个结构体，glsl的内存对齐规则是硬件决定的，glsl会使用一个共享内存布局的方式进行Uniform内存布局。因为一旦硬件定义了偏移量，它们在多个程序中是共享并一致的。能够使用像是glGetUniformIndices这样的函数来查询这个信息。

虽然共享布局给了我们很多节省空间的优化，但是我们需要查询每个uniform变量的偏移量，这会产生非常多的工作量。通常的做法是，不使用共享布局，而是使用std140布局。std140布局声明了每个变量的偏移量都是由一系列规则所决定的，这显式地声明了每个变量类型的内存布局。由于这是显式提及的，我们可以手动计算出每个变量的偏移量。

GLSL中的每个变量，比如说int、float和bool，都被定义为4字节量。**每4个字节将会用一个N来表示。**

|类型|布局规则|
|:-:|:-:|
|标量，比如int和bool|每个标量的基准对齐量为N。|
|向量|2N或者4N。这意味着`vec3`的基准对齐量为`4N`。|
|标量或向量的数组|每个元素的基准对齐量与vec4的相同。|
|矩阵|储存为列向量的数组，每个向量的基准对齐量与vec4的相同。|
|结构体|等于所有元素根据规则计算后的大小，但会填充到vec4大小的倍数。|

实例：

```glsl
layout (std140) uniform ExampleBlock//告诉OpenGL这个Uniform块使用的是std140布局
{
                     // 基准对齐量       // 对齐偏移量
    float value;     // 4               // 0 
    vec3 vector;     // 16              // 16  (必须是16的倍数，所以 4->16)
    mat4 matrix;     // 16              // 32  (列 0)
                     // 16              // 48  (列 1)
                     // 16              // 64  (列 2)
                     // 16              // 80  (列 3)
    float values[3]; // 16              // 96  (values[0])
                     // 16              // 112 (values[1])
                     // 16              // 128 (values[2])
    bool boolean;    // 4               // 144
    int integer;     // 4               // 148
}; 
```

根据std140布局的规则，我们就能使用像是`glBufferSubData`的函数将变量数据按照偏移量填充进缓冲中了。虽然std140布局不是最高效的布局，但它保证了内存布局在每个声明了这个Uniform块的程序中是一致的。

<hr>

使用Uniform这个缓冲

首先，我们需要调用`glGenBuffers`，创建一个Uniform缓冲对象。一旦我们有了一个缓冲对象，我们需要将它绑定到`GL_UNIFORM_BUFFER`目标，并调用`glBufferData`，分配足够的内存。

```cpp
unsigned int uboExampleBlock;
glGenBuffers(1, &uboExampleBlock);
glBindBuffer(GL_UNIFORM_BUFFER, uboExampleBlock);
glBufferData(GL_UNIFORM_BUFFER, 152, NULL, GL_STATIC_DRAW);//分配152字节的内存，原因见上面内存布局
glBindBuffer(GL_UNIFORM_BUFFER, 0);
```

每当我们需要对缓冲更新或者插入数据，我们都会绑定到uboExampleBlock，并使用glBufferSubData来更新它的内存。我们只需要更新这个Uniform缓冲一次，所有使用这个缓冲的着色器就都使用的是更新后的数据了。

Q：如何才能让OpenGL知道哪个Uniform缓冲对应的是哪个Uniform块？
A：在OpenGL上下文中，定义了一些绑定点(Binding Point)，我们可以将一个Uniform缓冲链接至它。在创建Uniform缓冲之后，我们将它绑定到其中一个绑定点上，并将着色器中的Uniform块绑定到相同的绑定点，把它们连接到一起。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/4.8Advanced%20GLSL/Reference/advanced_glsl_binding_points.png)

将Uniform块绑定到一个特定的绑定点中，我们需要调用`glUniformBlockBinding`函数，它的第一个参数是一个程序对象，之后是一个Uniform块索引和链接到的绑定点。Uniform块索引(Uniform Block Index)是着色器中已定义Uniform块的位置值索引。这可以通过调用`glGetUniformBlockIndex`来获取，它接受一个程序对象和Uniform块的名称。

将图示中的Lights Uniform块链接到绑定点2：

```cpp
unsigned int lights_index = glGetUniformBlockIndex(shaderA.ID, "Lights");//先get到在自己着色器内的索引
glUniformBlockBinding(shaderA.ID, lights_index, 2);//把这个索引跟绑定点2绑定
```

接下来，我们还需要绑定Uniform缓冲对象到相同的绑定点上，这可以使用glBindBufferBase或glBindBufferRange来完成。

```cpp
glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboExampleBlock);//需要一个目标，一个绑定点索引和一个Uniform缓冲对象作为它的参数
// 或
glBindBufferRange(GL_UNIFORM_BUFFER, 2, uboExampleBlock, 0, 152);
```

`glBindBufferBase`这个函数将uboExampleBlock链接到绑定点2上，自此，绑定点的两端都链接上了

现在，所有的东西都配置完毕了，我们可以开始向Uniform缓冲中添加数据了。只要我们需要，就可以使用glBufferSubData函数，用一个字节数组添加所有的数据，或者更新缓冲的一部分。要想更新uniform变量boolean，我们可以用以下方式更新Uniform缓冲对象：

```cpp
glBindBuffer(GL_UNIFORM_BUFFER, uboExampleBlock);
int b = true;//GLSL中的bool是4字节的，所以我们将它存为一个integer
glBufferSubData(GL_UNIFORM_BUFFER, 144, 4, &b); 
glBindBuffer(GL_UNIFORM_BUFFER, 0);
```

<hr>

<h3>优化我们自己的代码</h3>

我们不断地在使用3个矩阵：投影、观察和模型矩阵。在所有的这些矩阵中，只有模型矩阵会频繁变动。如果我们有多个着色器使用了这同一组矩阵，那么使用Uniform缓冲对象可能会更好。

总体操作流程如下：

先修改vertex shader，打包view和projection这两个矩阵，其他的不用更改

```glsl
//使用Uniform缓冲
layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};
```

再修改主函数，绑定每个shader要渲染的物体的各自的

```cpp
//Uniform缓冲将顶点着色器的Uniform块设置为绑定点0。注意我们需要对每个着色器都设置一遍。
unsigned int uniformBlockIndexRed    = glGetUniformBlockIndex(shaderRed.ID, "Matrices");
unsigned int uniformBlockIndexGreen  = glGetUniformBlockIndex(shaderGreen.ID, "Matrices");
unsigned int uniformBlockIndexBlue   = glGetUniformBlockIndex(shaderBlue.ID, "Matrices");
unsigned int uniformBlockIndexYellow = glGetUniformBlockIndex(shaderYellow.ID, "Matrices");  

glUniformBlockBinding(shaderRed.ID,    uniformBlockIndexRed, 0);
glUniformBlockBinding(shaderGreen.ID,  uniformBlockIndexGreen, 0);
glUniformBlockBinding(shaderBlue.ID,   uniformBlockIndexBlue, 0);
glUniformBlockBinding(shaderYellow.ID, uniformBlockIndexYellow, 0);

//接下来，我们创建Uniform缓冲对象本身，并将其绑定到绑定点0：
unsigned int uboMatrices
glGenBuffers(1, &uboMatrices);

glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
glBindBuffer(GL_UNIFORM_BUFFER, 0);

glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));

//写入投影矩阵和视角矩阵（内存什么写在前面和后面务必要和Matrices的顺序匹配，这里先projection后view）
glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width/(float)height, 0.1f, 100.0f);
glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
glBindBuffer(GL_UNIFORM_BUFFER, 0);

glm::mat4 view = camera.GetViewMatrix();           
glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
glBindBuffer(GL_UNIFORM_BUFFER, 0);

//进入渲染循环后
//把视角矩阵和投影矩阵写入到Uniform块中（只需要每个循环1次即可）
glm::mat4 view = camera.GetViewMatrix();
glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
glBindBuffer(GL_UNIFORM_BUFFER, 0);

glBindVertexArray(cubeVAO);
shaderRed.use();
glm::mat4 model;
model = glm::translate(model, glm::vec3(-0.75f, 0.75f, 0.0f));  // 移动到左上角
shaderRed.setMat4("model", model);
glDrawArrays(GL_TRIANGLES, 0, 36);        
// ... 绘制绿色立方体
// ... 绘制蓝色立方体
// ... 绘制黄色立方体 
```

也可以参见这个文件夹里面我的Project，这里我用Uniform Buffer处理了上一个场景的代码

Uniform缓冲对象比起独立的uniform有很多好处。

+ 一次设置很多uniform会比一个一个设置多个uniform要快很多。
+ 比起在多个着色器中修改同样的uniform，在Uniform缓冲中修改一次会更容易一些。
+ 如果使用Uniform缓冲对象的话，你可以在着色器中使用更多的uniform。OpenGL限制了它能够处理的uniform数量，这可以通过GL_MAX_VERTEX_UNIFORM_COMPONENTS来查询。当使用Uniform缓冲对象时，最大的数量会更高。所以，当你达到了uniform的最大数量时（比如再做骨骼动画(Skeletal Animation)的时候），你总是可以选择使用Uniform缓冲对象。