<h2>延迟着色</h2>

我们现在一直使用的光照方式叫做正向渲染(Forward Rendering)或者正向着色法(Forward Shading)，它是我们渲染物体的一种非常直接的方式，在场景中我们根据所有光源照亮一个物体，之后再渲染下一个物体，以此类推。（性能开销很大）

正向着色缺点：

对于每一个需要渲染的物体，程序都要对每一个光源每一个需要渲染的片段进行迭代，这是非常多的！因为大部分片段着色器的输出都会被之后的输出覆盖，正向渲染还会在场景中因为高深的复杂度(多个物体重合在一个像素上)浪费大量的片段着色器运行时间。

<h2>延迟着色法</h2>

延迟着色法基于延迟大部分计算量非常大的渲染(像是光照)到后期进行处理的想法。它包含两个处理阶段(Pass)：

1. 几何处理阶段(Geometry Pass)：我们先渲染场景一次，之后获取对象的各种几何信息，并储存在一系列叫做G缓冲(G-buffer)的纹理中；包括位置向量、颜色向量、法向量和/或镜面值。场景中这些储存在G缓冲中的几何信息将会在之后用来做(更复杂的)光照计算。

2. 光照处理阶段(Lighting Pass)：使用G-缓冲内的纹理数据。在光照处理阶段中，我们渲染一个屏幕大小的方形，并使用G缓冲中的几何数据对每一个片段计算场景的光照；在每个像素中我们都会对G缓冲进行迭代。

**我们对于渲染过程进行解耦，将它高级的片段处理挪到后期进行，而不是直接将每个对象从顶点着色器带到片段着色器。光照计算过程还是和我们以前一样，但是现在我们需要从对应的G缓冲而不是顶点着色器(和一些uniform变量)那里获取输入变量了。**

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.8Deferred%20Shading/Reference/deferred_overview.png)

优点：

+ **能保证在G缓冲中的片段和在屏幕上呈现的像素所包含的片段信息是一样的，因为深度测试已经最终将这里的片段信息作为最顶层的片段**。这样保证了对于在光照处理阶段中处理的**每一个像素都只处理一次**，所以我们能够省下很多无用的渲染调用。除此之外，延迟渲染还允许我们做更多的优化，从而渲染更多的光源。
+ 在几何处理阶段中填充G缓冲非常高效，因为我们直接储存像是位置，颜色或者是法线等对象信息到帧缓冲中，而这几乎不会消耗处理时间。在此基础上使用多渲染目标(Multiple Render Targets, MRT)技术，我们甚至可以在一个渲染处理之内完成这所有的工作。

缺点：

+ G缓冲要求我们在纹理颜色缓冲中存储相对比较大的场景数据，这会消耗比较多的显存，尤其是类似位置向量之类的需要高精度的场景数据。
+ 另外一个缺点就是他不支持混色(因为我们只有最前面的片段信息)，因此也不能使用MSAA了，因为G缓冲中所有的数据都是从一个单独的片段中来的，而混合需要对多个片段的组合进行操作。
+ 迫使对大部分场景的光照使用相同的光照算法

<hr>

<h3>G-Buffer</h3>

正向渲染中照亮一个片段所需要的所有数据：（前4个每个片元都是不同的，后2个可以通过Uniform统一传入）

+ 一个3D**位置向量**来计算(插值)片段位置变量供lightDir和viewDir使用
+ 一个RGB**漫反射颜色向量**，也就是反照率(Albedo)
+ 一个3D**法向量**来判断平面的斜率
+ 一个**镜面强度**浮点值
+ **所有光源的位置和颜色向量**(uniform传入)
+ 玩家或者**观察者的位置向量**(uniform传入)

延迟渲染思想：把上面的东西存到G缓冲里面，然后逐一把物体draw出来；之后在第二个Pass中计算物体的光照

所以过程伪代码为：

```cpp
while(...) //游戏循环
{
    // 1. 几何处理阶段：渲染所有的几何/颜色数据到G缓冲 
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gBufferShader.Use();
    for(Object obj : Objects)
    {
        ConfigureShaderTransformsAndUniforms();
        obj.Draw();
    }
    
    // 2. 光照处理阶段：使用G缓冲计算场景的光照
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    lightingPassShader.Use();
    BindAllGBufferTextures();
    SetLightingUniforms();
    RenderQuad();
}
```

对于G-Buffer中存的每一个片段要保留跟正向渲染前4个一样的信息（一个位置向量、一个法向量，一个颜色向量，一个镜面强度值），这部分的渲染可以使用多渲染目标(Multiple Render Targets)可以使用多渲染目标(Multiple Render Targets)来在一个渲染处理之内渲染多个颜色缓冲

对于几何渲染处理阶段，我们首先需要初始化一个帧缓冲对象，我们很直观的称它为gBuffer，它包含了多个颜色缓冲和一个单独的深度渲染缓冲对象(Depth Renderbuffer Object)。位置和法向量的纹理，应该使用高精度的纹理(每分量16或32位的浮点数)，而对于反照率和镜面值，使用默认的纹理(每分量8位浮点数)

```cpp
unsigned int gBuffer;
glGenFramebuffers(1, &gBuffer);
glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
unsigned int gPosition, gNormal, gColorSpec;
  
//位置颜色缓冲
glGenTextures(1, &gPosition);
glBindTexture(GL_TEXTURE_2D, gPosition);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
  
//法线颜色缓冲
glGenTextures(1, &gNormal);
glBindTexture(GL_TEXTURE_2D, gNormal);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
  
//颜色+镜面颜色缓冲
glGenTextures(1, &gAlbedoSpec);
glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

//由于使用了多渲染目标，我们需要显式告诉OpenGL我们需要使用glDrawBuffers渲染的是和GBuffer关联的哪个颜色缓冲
unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
glDrawBuffers(3, attachments);
  
//之后同样添加渲染缓冲对象(Render Buffer Object)为深度缓冲(Depth Buffer)，并检查完整性
[...]
```

需要注意的是，我们使用RGB纹理来储存位置和法线的数据，因为每个对象只有三个分量；但是我们将颜色和镜面强度数据合并到一起，存储到一个单独的RGBA纹理里面，这样我们就不需要声明一个额外的颜色缓冲纹理了。

```glsl
#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{    
    // 存储第一个G缓冲纹理中的片段位置向量
    gPosition = FragPos;
    // 同样存储对每个逐片段法线到G缓冲中
    gNormal = normalize(Normal);
    // 和漫反射对每个逐片段颜色
    gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
    // 存储镜面强度到gAlbedoSpec的alpha分量
    gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;
}
```

<h3>警告：因为有光照计算，所以保证所有变量在一个坐标空间当中至关重要。</h3>

<hr>

<h3>延迟光照处理阶段</h3>

现在我们已经有了一大堆的片段数据储存在G缓冲中供我们处置，我们可以选择通过一个像素一个像素地遍历各个G缓冲纹理，并将储存在它们里面的内容作为光照算法的输入，来完全计算场景最终的光照颜色。（已经变换到2D屏幕空间了）由于**所有的G缓冲纹理都代表的是最终变换的片段值，我们只需要对每一个像素执行一次昂贵的光照运算就行了**。这使得延迟光照非常高效，特别是在需要调用大量重型片段着色器的复杂场景中。

```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
shaderLightingPass.Use();
//等于说已经构建出了屏幕大小的位置，法线贴图和漫反射贴图
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, gPosition);
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, gNormal);
glActiveTexture(GL_TEXTURE2);
glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
// 同样发送光照相关的uniform
SendAllLightUniformsToShader(shaderLightingPass);
glUniform3fv(glGetUniformLocation(shaderLightingPass.Program, "viewPos"), 1, &camera.Position[0]);
RenderQuad();
```

添加了一个新的方法，从而使我们能够获取光照的输入变量，当然这些变量我们会从G缓冲中直接采样，其他跟之前没有区别

```glsl
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct Light {
    vec3 Position;
    vec3 Color;
};
const int NR_LIGHTS = 32;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

void main()
{             
    // 从G缓冲中获取数据
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;

    // 然后和往常一样地计算光照
    vec3 lighting = Albedo * 0.1; // 硬编码环境光照分量
    vec3 viewDir = normalize(viewPos - FragPos);
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // 漫反射
        vec3 lightDir = normalize(lights[i].Position - FragPos);
        vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * lights[i].Color;
        lighting += diffuse;
    }

    FragColor = vec4(lighting, 1.0);
}  
```

<hr>

延迟渲染有不能混合和光照算法单一的缺点，我们通常分割我们的渲染器为两个部分：一个是延迟渲染的部分，另一个是专门为了混合或者其他不适合延迟渲染管线的着色器效果而设计的的正向渲染的部分。

<h3>结合延迟渲染与正向渲染</h3>

现在我们想要渲染每一个光源为一个3D立方体，并放置在光源的位置上随着延迟渲染器一起发出光源的颜色。很明显，我们需要做的第一件事就是在延迟渲染方形之上正向渲染所有的光源，它会在延迟渲染管线的最后进行。所以我们只需要像正常情况下渲染立方体，只是会在我们完成延迟渲染操作之后进行。

```cpp
//渲染循环中------
// 延迟渲染光照渲染阶段
[...]
RenderQuad();

// 现在像正常情况一样正向渲染所有光立方体
shaderLightBox.Use();
glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(projection));
glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
for (GLuint i = 0; i < lightPositions.size(); i++)
{
    model = glm::mat4();
    model = glm::translate(model, lightPositions[i]);
    model = glm::scale(model, glm::vec3(0.25f));
    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(locLightcolor, 1, &lightColors[i][0]);
    RenderCube();
}
//--------------
```

**但是直接这样做会出现：这些渲染出来的立方体并没有考虑到我们储存的延迟渲染器的几何深度(Depth)信息**，结果是它被渲染在之前渲染过的物体之上（类似画家算法情形）

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.8Deferred%20Shading/Reference/deferred_lights_no_depth.png)

**解决方案：复制出在几何渲染阶段中储存的深度信息，并输出到默认的帧缓冲的深度缓冲，然后我们才渲染光立方体。**这样之后只有当它在之前渲染过的几何体上方的时候，光立方体的片段才会被渲染出来。我们可以使用`glBlitFramebuffer`这个函数，它允许我们复制一个用户定义的帧缓冲区域到另一个用户定义的帧缓冲区域。

如果我们仅仅是简单复制它的深度缓冲内容到默认帧缓冲的深度缓冲中，那么光立方体就会像是场景中所有的几何体都是正向渲染出来的一样渲染出来。我们需要指定一个帧缓冲为读帧缓冲(Read Framebuffer)，并且类似地指定一个帧缓冲为写帧缓冲(Write Framebuffer)：

```cpp
//复制整个读帧缓冲的深度缓冲信息到默认帧缓冲的深度缓冲，对于颜色缓冲和模板缓冲我们也可以这样处理
glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // 写入到默认帧缓冲
glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
glBindFramebuffer(GL_FRAMEBUFFER, 0);
// 现在像之前一样渲染光立方体
[...]
```

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.8Deferred%20Shading/Reference/deferred_lights_depth.png)

<hr>

<h3>更多光源</h3>

让大量光源成为可能的是我们能够对延迟渲染管线引用的一个非常棒的优化：光体积(Light Volumes)

当我们渲染一个复杂光照场景下的片段着色器时，我们会计算场景中每一个光源的贡献，不管它们离这个片段有多远。很大一部分的光源根本就不会到达这个片段，所以为什么我们还要浪费这么多光照运算呢？

**计算光源的半径，或是体积，也就是光能够到达片段的范围。由于大部分光源都使用了某种形式的衰减(Attenuation)，我们可以用它来计算光源能够到达的最大路程，或者说是半径。**我们接下来只需要对那些在一个或多个光体积内的片段进行繁重的光照运算就行了。这可以给我们省下来很可观的计算量，因为我们现在只在需要的情况下计算光照。

```glsl
struct Light {
    [...]
    float Radius;//求解衰减方程得到光体积半径
}; 

void main()
{
    [...]
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // 计算光源和该片段间距离
        float distance = length(lights[i].Position - FragPos);
        if(distance < lights[i].Radius)//这一步实际是不可行的，GPU对分支语句无法优化！！
        {
            // 执行大开销光照
            [...]
        }
    }   
}
```

使用光体积更好的方法是渲染一个实际的球体，并根据光体积的半径缩放。这些球的中心放置在光源的位置，由于它是根据光体积半径缩放的，这个球体正好覆盖了光的可视体积。这就是我们的技巧：我们使用大体相同的延迟片段着色器来渲染球体。

它被应用在场景中每个光源上，并且所得的片段相加混合在一起。这个结果和之前场景是一样的，但这一次只渲染对于光源相关的片段。它有效地减少了从`nr_objects * nr_lights`到`nr_objects + nr_lights`的计算量，这使得多光源场景的渲染变得无比高效。这正是为什么延迟渲染非常适合渲染很大数量光源。

然而这个方法仍然有一个问题：面剔除(Face Culling)需要被启用(否则我们会渲染一个光效果两次)，并且在它启用的时候用户可能进入一个光源的光体积，然而这样之后这个体积就不再被渲染了(由于背面剔除)，这会使得光源的影响消失。这个问题可以通过一个模板缓冲技巧来解决。

这仍然不是最好的优化。另外两个基于延迟渲染的更流行(并且更高效)的拓展叫做延迟光照(Deferred Lighting)和切片式延迟着色法(Tile-based Deferred Shading)。这些方法会很大程度上提高大量光源渲染的效率，并且也能允许一个相对高效的多重采样抗锯齿(MSAA)。