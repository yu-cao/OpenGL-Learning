<h2>泛光(Bloom)</h2>

强调明亮的光源与区域：一种区分明亮光源的方式是使它们在监视器上发出光芒，光源的的光芒向四周发散。这样观察者就会产生光源或亮区的确是强光区。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.6Bloom/Reference/bloom_example.png)

Bloom和HDR结合使用效果很好。常见的一个误解是HDR和泛光是一样的，很多人认为两种技术是可以互换的。但是它们是两种不同的技术，用于各自不同的目的上。可以使用默认的8位精确度的帧缓冲，也可以在不使用泛光效果的时候，使用HDR。只不过在有了HDR之后再实现泛光就更简单了。

在场景中渲染一个带有4个立方体形式不同颜色的明亮的光源。带有颜色的发光立方体的亮度在1.5到15.0之间。如果我们将其渲染至HDR颜色缓冲，场景看起来会是这样的：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.6Bloom/Reference/bloom_scene.png)

我们得到这个HDR颜色缓冲纹理，**提取所有超出一定亮度的fragment**。这样我们就会获得一个只有fragment超过了一定阈限的颜色区域：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.6Bloom/Reference/bloom_extracted.png)

我们将这个超过一定亮度阈限的纹理进行**模糊**。泛光效果的强度很大程度上被模糊过滤器的范围和强度所决定。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.6Bloom/Reference/bloom_blurred.png)

最终的被模糊化的纹理就是我们用来获得发出光晕效果的东西。这个已模糊的纹理要添加到原来的HDR场景纹理的上部。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.6Bloom/Reference/bloom_small.png)

泛光的品质很大程度上取决于所用的模糊过滤器的质量和类型。简单的改改模糊过滤器就会极大的改变泛光效果的品质。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.6Bloom/Reference/bloom_steps.png)

<hr>

<h3>提取亮色</h3>

第一步我们要从渲染出来的场景中提取两张图片

简单是使用不同的shader渲染场景两次，但也可以使用一个叫做MRT（Multiple Render Targets多渲染目标）的小技巧，这样我们就能定义多个像素着色器了。有了它我们还能够在一个单独渲染处理中提取头两个图片。在像素着色器的输出前，我们指定一个布局location标识符，这样我们便可控制一个像素着色器写入到哪个颜色缓冲：

```glsl
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
```

**使用多个像素着色器输出的必要条件是，有多个颜色缓冲附加到了当前绑定的帧缓冲对象上。**

```cpp
// set up floating point framebuffer to render scene to
unsigned int hdrFBO;
glGenFramebuffers(1, &hdrFBO);
glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

//可以得到一个附加了两个颜色缓冲的帧缓冲对象
unsigned int colorBuffers[2];
glGenTextures(2, colorBuffers);
for (unsigned int i = 0; i < 2; i++)
{
    glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // attach texture to framebuffer
    //通过使用GL_COLOR_ATTACHMENT1，我们可以得到一个附加了两个颜色缓冲的帧缓冲对象：
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
}
```

我们需要显式告知OpenGL我们正在通过glDrawBuffers渲染到多个颜色缓冲，否则OpenGL只会渲染到帧缓冲的第一个颜色附件，而忽略所有其他的。我们可以通过传递多个颜色附件的枚举来做这件事，我们以下面的操作进行渲染：

```cpp
GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
glDrawBuffers(2, attachments);
```

当渲染到这个帧缓冲中的时候，一个着色器使用一个布局location修饰符，那么fragment就会用相应的颜色缓冲就会被用来渲染。这很棒，因为这样省去了我们为提取明亮区域的额外渲染步骤，因为我们现在可以直接从将被渲染的fragment提取出它们：

shaderLight的片元着色器，用来渲染在某个灯光下的场景的光照情况

```glsl
#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform vec3 lightColor;

void main()
{           
    FragColor = vec4(lightColor, 1.0);//先正常计算光照
    //判定它的亮度是否超过了一定阈限
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(FragColor.rgb, 1.0);//如果超过，存到第二个颜色缓冲中
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
```

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.6Bloom/Reference/bloom_attachments.png)

调用时把所有的光源设定为立方体

```cpp
// finally show all the light sources as bright cubes
shaderLight.use();
shaderLight.setMat4("projection", projection);
shaderLight.setMat4("view", view);

for (unsigned int i = 0; i < lightPositions.size(); i++)
{
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(lightPositions[i]));
    model = glm::scale(model, glm::vec3(0.25f));
    shaderLight.setMat4("model", model);
    shaderLight.setVec3("lightColor", lightColors[i]);
    renderCube();
}
glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

<hr>

<h3>高斯模糊</h3>

高斯曲线通常被描述为一个钟形曲线，中间的值达到最大化，随着距离的增加，两边的值不断减少。高斯曲线在它的中间处的面积最大，使用它的值作为权重使得近处的样本拥有最大的优先权。从fragment的32×32的四方形区域采样，这个权重随着和fragment的距离变大逐渐减小；通常这会得到更好更真实的模糊效果，这种模糊叫做高斯模糊。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.6Bloom/Reference/bloom_gaussian_two_pass.png)

高斯方程有个非常巧妙的特性，它允许我们把二维方程分解为两个更小的方程：一个描述水平权重，另一个描述垂直权重。我们首先用水平权重在整个纹理上进行水平模糊，然后在经改变的纹理上进行垂直模糊。利用这个特性，结果是一样的，但是可以节省难以置信的性能，因为我们现在只需做32+32次采样，不再是1024了！这叫做两步高斯模糊。

如果我们对一个图像进行模糊处理，至少需要两步，最好使用帧缓冲对象做这件事。

```glsl
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D image;

uniform bool horizontal;

uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{             
    vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
    vec3 result = texture(image, TexCoords).rgb * weight[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
```

为图像的模糊处理创建两个基本的帧缓冲，每个只有一个颜色缓冲纹理：

```cpp
unsigned int pingpongFBO[2];
unsigned int pingpongBuffer[2];
glGenFramebuffers(2, pingpongFBO);
glGenTextures(2, pingpongBuffer);
for (unsigned int i = 0; i < 2; i++)
{
    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
    glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0
    );
}

//得到一个HDR纹理后，我们用提取出来的亮区纹理填充一个帧缓冲，然后对其模糊处理10次（5次垂直5次水平）：
bool horizontal = true, first_iteration = true;
int amount = 10;
shaderBlur.use();
for (unsigned int i = 0; i < amount; i++)
{
    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]); 
    shaderBlur.setInt("horizontal", horizontal);
    glBindTexture(
        GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongBuffers[!horizontal]
    ); 
    RenderQuad();
    horizontal = !horizontal;
    if (first_iteration)
        first_iteration = false;
}
glBindFramebuffer(GL_FRAMEBUFFER, 0); 
```

每次循环我们根据我们打算渲染的是水平或是垂直来绑定两个缓冲其中之一，而将另一个绑定为纹理进行模糊。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.6Bloom/Reference/bloom_blurred_large.png)

泛光的最后一步是把模糊处理的图像和场景原来的HDR纹理进行结合。

```glsl
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float exposure;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    hdrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0f);
}
```

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.6Bloom/Reference/bloom.png)

```cpp
main.cpp整体流程
1. 建立1个hdrFBO的帧缓冲，然后建立2个colorBuffer的颜色附件(RGB16F)（一个原样渲染，一个存储高于一定亮度与之的fragment）和1个rboDepth的渲染缓冲对象附件并依次进行绑定
2. 建立2个pingpongFBO的帧缓冲准备用作高斯渲染（一个负责水平拓展，一个负责垂直拓展）

渲染循环
1. 渲染场景到浮点帧缓冲（先渲染盒子，再连续渲染完6盏灯）
2. 对于存储了高于一定亮度的fragment的颜色附件缓冲进行高斯模糊
3. 渲染浮点帧缓冲到要被显示的帧缓冲中
```

[参考代码](https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/7.bloom/bloom.cpp)