## 漫反射辐照度(Diffuse irradiance)

这篇文章主要是分2部分，第一部分是从`.hdr`中转换得到一个Cubemap，第二部分是把这个Cubemap变成辐照度贴图作为环境光照

<hr>

基于图像的光照（Image based lighting, IBL）是一组照明物体的技术，不同于前一个教程中的直接分析光，而是将周围环境视为一个大光源。这通常通过操纵立方体贴图环境贴图（取自现实世界或从3D场景生成）来完成，这样我们就可以在我们的光照方程中直接使用它：将每个立方体贴图像素视为光发射器。通过这种方式，我们可以有效地捕捉环境的全局照明和一般感觉，使物体更好地融入其环境。

由于IBL算法捕获某些（全局）环境的光照，因此其输入对于环境光而言是更精确的，甚至是GI的粗略近似。这使得IBL对PBR很有意义，因为当我们考虑环境照明时，对象在物理上看起来更加精确。

简单回顾一下渲染方程：
$$
L_{o}\left(p, \omega_{o}\right)=\int_{\Omega}\left(k_{d} \frac{c}{\pi}+k_{s} \frac{D F G}{4\left(\omega_{o} \cdot n\right)\left(\omega_{i} \cdot n\right)}\right) L_{i}\left(p, \omega_{i}\right) n \cdot \omega_{i} d \omega_{i}
$$
回想一下目标：解决所有入射光线方向$$\omega_{i}$$在半球&Omega;的积分。之前解决积分的方法很简单，因为我们事先知道了几个明确的对积分有贡献的光线方向。但是在这里，每一个从环境射入的光线方向$$\omega_{i}$$都会潜在产生影响，使得求解积分变得困难，以下是求解的两个要求：

+ 对于给出的某一方向，我们需要一些方法来检索场景中的辐射
+ 解决积分必须要快而且实时

第一个要求相对容易。我们知道表示环境或场景辐照度的一种方式是（处理过的）环境立方体贴图。给定这样的立方体贴图，我们可以将立方体贴图的每个像素可视化为单个发射光源。通过使用某一方向向量$$\omega_{i}$$对此立方体贴图进行采样，我们从该方向检索场景的辐射。

```cpp
vec3 radiance = texture(_cubemapEnvironment, w_i).rgb;
```

但是，解决积分需要我们不仅从一个方向对环境贴图进行采样，而是对所有可能的方向$$\omega_{i}$$都在半球&Omega;上进行采样，这对于每个片段着色器调用而言开销太大。为了以更有效的方式解决积分，我们需要预处理或预先计算其大部分计算。为此，我们必须重新处理反射方程。

首先，我们知道BRDF中，漫反射项$$k_d$$和镜面反射项$$k_s$$是独立的，所以可以分成两部分积分
$$
L_{o}\left(p, \omega_{o}\right)=\int_{\Omega}\left(k_{d} \frac{c}{\pi}\right) L_{i}\left(p, \omega_{i}\right) n \cdot \omega_{i} d \omega_{i}+\int_{\Omega}\left(k_{s} \frac{D F G}{4\left(\omega_{o} \cdot n\right)\left(\omega_{i} \cdot n\right)}\right) L_{i}\left(p, \omega_{i}\right) n \cdot \omega_{i} d \omega_{i}
$$
我们主要关注漫反射积分。

对于漫反射积分，观察发现漫反射的Lambert系数是一个常数($$c, k_d, \pi$$是定数，故可以提出来)
$$
L_{o}\left(p, \omega_{o}\right)=k_{d} \frac{c}{\pi} \int_{\Omega} L_{i}\left(p, \omega_{i}\right) n \cdot \omega_{i} d \omega_{i}
$$
这时候我们发现积分制依赖于$$w_i$$(我们假定p点在环境图的中心)，由此我们可以通过卷积(convolution)来进行预计算一个新的存储有每个样本方向（或纹理）$$w_0$$的漫反射积分结果的立方体贴图

为了对环境贴图进行卷积，我们通过对半球&Omega;上的大量方向$$w_i$$进行离散采样并对它们的辐照度平均化来解决每个输出$$w_0$$样本方向的积分。我们建立样本方向的半球将面向我们卷积的输出$$w_0$$样本方向。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/IBL/Reference/ibl_hemisphere_sample.png)

这个预计算出来的立方体贴图，对于每个样本方向$$w_0$$存储一个整型的积分结果，用来表示场景的所有击中沿着法线方向$$w_0$$的平面的间接漫反射光之和(即那个方向上的对于 **以$$w_0$$为法线的平面** 入射的所有光照的和)，这样的Cubemap被称为Irradiance，可以较好表达某个方向的辐照度。

辐射方程也取决于我们界定位于辐照度图中心的位置$$p$$，这意味着所有漫反射间接光必须来自单个环境map，这可能打破现实的感觉（特别是在室内）。一般引擎中都会使用ReflectionProbe（反射探针）来处理这个问题。通过这种方式，在位置$$p$$的辐照度（irradiance）与辐射亮度（radiance）就是它最近的反射探针的插值辐照度。我们假设我们总是在中心对环境贴图进行采样。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/IBL/Reference/ibl_irradiance.png)

通过存储convolution的结果在每个Cubemap的纹理像素中（以$$w_0$$为方向），辐射度图显示有点像环境的平均颜色。从该环境地图中采样任何方向将为我们提供该特定方向的场景辐照度。

### PBR和HDR

我们在之前的教程中了解到：在PBR中考虑场景的HDR非常重要，因为PBR是基于物理的，光照和其物理属性也是相匹配的。不论我们是猜还是基于测量的方式来描述光源的辐射功率，一个灯泡的光能量和太阳光能量都是天壤之别，但是没有HDR可能就会无法表现这两者的区别。

这与IBL关系是什么？我们之前的教程中看到让PBR在HDR下工作是相对容易的，但是当我们使用IBL将环境的间接光强度建立在环境立方体贴图的颜色值上，我们需要某种方式将照明的高动态范围存储到环境贴图中。

我们一直使用的环境贴图就像cubemap（例如用作skybox）一样，处于低动态范围（LDR）。我们直接使用各个面的图像的颜色值，范围介于0.0和1.0之间，并按原样处理。虽然这可能适用于视觉输出，但当它们作为物理输入参数时，它不会起作用。

####辐射HDR的文件格式

输入光度文件格式。辐射文件格式（扩展名为.hdr）存储一个完整的立方体贴图，其中所有6个面作为浮点数据，允许任何人指定0.0到1.0范围之外的颜色值，以使灯具有正确的颜色强度。文件格式也使用一个聪明的技巧来存储每个浮点值：不是使用每个通道的32位值，而是使用颜色的alpha通道作为指数，每个通道都是8位（这确实会导致精度损失）（也就是RGB通道都是8位底数，A通道是8位指数）。这非常有效，但需要解析程序将每种颜色重新转换为它们的浮点等价物。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/IBL/Reference/ibl_hdr_radiance.png)

这张图未显示我们之前看到的环境贴图的6个单独立方体贴图面中的任何一个。此环境贴图从球体投影到平面上，以便我们可以更轻松地将环境存储到称为 equirectangular map的单个map中。这确实带来了一个小警告，因为大多数视觉分辨率存储在水平视图方向，而较少的保留在底部和顶部方向。在大多数情况下，这是一个不错的折衷方案，因为对于几乎所有渲染器，您都可以在水平观察方向上找到大部分有趣的照明和环境。

#### HDR和stb_image.h

stb_image.h支持将辐射HDR图像直接加载为一个浮点值数组：

```cpp
#include "stb_image.h"
[...]

stbi_set_flip_vertically_on_load(true);
int width, height, nrComponents;
float *data = stbi_loadf("newport_loft.hdr", &width, &height, &nrComponents, 0);
unsigned int hdrTexture;
if (data)
{
    glGenTextures(1, &hdrTexture);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}
else
{
    std::cout << "Failed to load HDR image." << std::endl;
}
```

stb_image.h自动将HDR值映射到浮点值列表中：默认情况下，每个通道32位，每种颜色3个通道。这就是我们需要将equirectangular HDR环境贴图存储到2D浮点纹理中。

#### 从Equirectangular（方形贴图）到Cubemap

可以直接使用Equirectanglar映射到环境中进行查找，但这带来的cost很大，使用Cubemap更加高效一点。

为了完成转换，我们需要把这个贴图渲染到一个单位立方体上，把立方体的六个面当作CubeMap上的每一个面。并从内部投影所有立方体面上的equirectangular map，并将每个立方体边的6个image的每一个作为cubemap的一个面。此立方体的顶点着色器只是按原样渲染立方体，并将其本地位置（local pos）作为3D样本矢量传递给片段着色器：

```vert
//最简单的顶点着色器
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 localPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    localPos = aPos;  
    gl_Position =  projection * view * vec4(localPos, 1.0);
}
```

对于片段着色器，我们为立方体的每个部分着色，就像我们将立方体贴图整齐地折叠到立方体的每一边一样。

为了实现这一点，我们将片段的样本方向从立方体的本地位置（local pos）进行插值，然后使用此方向向量和一些三角函数的魔法对Equirectangular map进行采样，就好像它是立方体图本身一样。我们直接将结果存储到cube-face的片段中，这应该是我们需要做的：

```frag
#version 330 core
out vec4 FragColor;
in vec3 localPos;
 
uniform sampler2D equirectangularMap;
 
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}
 
void main()
{       
    vec2 uv = SampleSphericalMap(normalize(localPos)); // make sure to normalize localPos
    vec3 color = texture(equirectangularMap, uv).rgb;
     
    FragColor = vec4(color, 1.0);
}
```

这表明我们有效地将equirectangular图像映射到立方体形状，但还没有帮助我们将源HDR图像转换为立方体贴图纹理。为了实现这一点，我们必须渲染相同的立方体6次，查看立方体的每个单独的面，同时用framebuffer对象记录其可视结果：

```cpp
unsigned int captureFBO, captureRBO;
glGenFramebuffers(1, &captureFBO);
glGenRenderbuffers(1, &captureRBO);

glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
```

我们还会生成相应的立方体贴图，为其6个面中的每个面预先分配内存：

```CPP
unsigned int envCubemap;
glGenTextures(1, &envCubemap);
glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
for (unsigned int i = 0; i < 6; ++i)
{
    // note that we store each face with 16 bit floating point values
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
                 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
}
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
```

剩下要做的就是将立方体2D纹理捕捉到立方体贴图面上：简单原理就是——面向立方体每一面设置6个不同的视图矩阵，给定投影矩阵的fov为90度捕获整个面，并渲染一个立方体6次将结果存储在浮点帧缓冲区中：

```cpp
glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 captureViews[] = 
{
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

// convert HDR equirectangular environment map to cubemap equivalent
equirectangularToCubemapShader.use();
equirectangularToCubemapShader.setInt("equirectangularMap", 0);
equirectangularToCubemapShader.setMat4("projection", captureProjection);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, hdrTexture);

glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
for (unsigned int i = 0; i < 6; ++i)
{
    equirectangularToCubemapShader.setMat4("view", captureViews[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderCube(); // renders a 1x1 cube
}
glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

我们采用帧缓冲的颜色附件并且为cubemap的每个面切换纹理目标，使得可以直接将场景渲染到cubemap的面上。一旦这个例程完成（我们只需做一次），立方体贴图`envCubemap`就应该是原始HDR图像的立方体贴图的环境的版本。

通过编写一个非常简单的天空盒着色器来测试立方体贴图：

```vert
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 localPos;

void main()
{
    localPos = aPos;

    mat4 rotView = mat4(mat3(view)); // remove translation from the view matrix
    vec4 clipPos = projection * rotView * vec4(localPos, 1.0);

    gl_Position = clipPos.xyww;
}
```

注意这里的xyww技巧确保渲染的立方体片段的深度值总是最终为1.0，即最大深度值（在最后面）。注意，我们需要将深度比较功能更改为`GL_LEQUAL`：

```cpp
glDepthFunc(GL_LEQUAL);
```

片段着色器然后使用立方体的局部片段位置直接对立方体贴图环境贴图进行采样：

```frag
#version 330 core
out vec4 FragColor;

in vec3 localPos;
  
uniform samplerCube environmentMap;
  
void main()
{
    vec3 envColor = texture(environmentMap, localPos).rgb;
    
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
  
    FragColor = vec4(envColor, 1.0);
}
```

我们使用插值的顶点立方体位置对环境贴图进行采样，这些位置直接对应于要采样的正确方向矢量。看到相机的平移组件被忽略，在多维数据集上渲染此着色器应该会将环境贴图作为非移动背景。另外，请注意，当我们将环境贴图的HDR值直接输出到默认的LDR帧缓冲区时，我们希望正确地对颜色值进行色调映射。此外，默认情况下，几乎所有HDR地图都处于线性色彩空间中，因此我们需要在写入默认帧缓冲区之前应用伽马校正。

至此，我们成功地设法读取了HDR环境贴图，将其从equirectangular映射转换为立方体贴图，并将HDR立方体贴图作为天空盒渲染到场景中。

#### Cubemap卷积

我们的重点是求积分部分，策略是进行离散采样，生成出一张预计算的辐照度贴图，然后计算光照时候只要进行采样就可以了，采样的结果表达了这个点所受到的所有入射光之和。

由于半球的方向决定我们捕获辐照度的位置，我们可以预先计算每个可能的半球方向的辐照度，这些方向围绕所有传出方向：
$$
L_{o}\left(p, \omega_{o}\right)=k_{d} \frac{c}{\pi} \int_{\Omega} L_{i}\left(p, \omega_{i}\right) n \cdot \omega_{i} d \omega_{i}
$$

```cpp
vec3 irradiance = texture(irradianceMap, N);
```

由于我们之前已经设置了将equirectangular环境贴图转换为立方体贴图，我们可以采用完全相同的方法，但使用不同的片段着色器（在Shader中进行预积分操作）：

```vert
#version 330 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main()
{		
    // the sample direction equals the hemisphere's orientation 
    vec3 normal = normalize(localPos);
  
    vec3 irradiance = vec3(0.0);
  
    [...] // convolution code
  
    FragColor = vec4(irradiance, 1.0);
}
```

environmentMap是从equirectangular HDR environment map转化过来的

在这里，我们将沿着半球&Omega;的每个立方体贴图的texel生成一定数量的样本矢量，并且围绕着样本方向进行平均化。固定量的样本矢量将会均匀地分布在半球内部，我们使用的样本矢量越多，我们越接近积分值。

反射方程的积分$\int$应围绕相当难以使用的立体角$d \omega$旋转。我们将积分转换到对于球面坐标&theta;和φ上，我们使用极化坐标φ角在0和2π之间的半球环周围采样，并使用0和$\frac{1}{2}\pi$之间的俯仰角进行采样。这将为我们提供更新的反射积分：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/IBL/Reference/ibl_spherical_integrate.png)

$$L_o(p, \theta_o, \phi_o) = \frac{c}{\pi} \int_{\phi = 0}^{2\pi} \int_{\theta = 0}^{\frac{1}{2}\pi} L_i(p, \theta_i, \phi_i) \cos(\theta_i) \sin(\theta_i)   d\theta_i d\phi_i$$

蒙特卡洛积分法得：
$$
\begin{array}{c}{L_{o}\left(p, \theta_{o}, \phi_{o}\right)=\frac{c}{\pi} \frac{2 \pi}{N_{1}} \frac{\pi}{2 N_{2}} \sum^{N_{1}} \sum^{N_{2}} L_{i}\left(p, \theta_{i}, \phi_{i}\right) \cos \left(\theta_{i}\right) \sin \left(\theta_{i}\right)} \\ {L_{o}\left(p, \theta_{o}, \phi_{o}\right)=\frac{\pi c}{N_{1} N_{2}} \sum^{N_{1}} \sum^{N_{2}} L_{i}\left(p, \theta_{i}, \phi_{i}\right) \cos \left(\theta_{i}\right) \sin \left(\theta_{i}\right)}\end{array}
$$
其中由于球形的一般性质，当样本区域朝向中心顶部会聚时（俯仰角&theta;越大），半球的离散样本区域越小。为了弥补较小的区域，所以我们通过使用sinθ来缩放区域来权衡其贡献。

最后代码如下：

```frag
vec3 irradiance = vec3(0.0);  

vec3 up    = vec3(0.0, 1.0, 0.0);
vec3 right = cross(up, normal);
up         = cross(normal, right);

float sampleDelta = 0.025;//固定的一个delta值遍历半球，表达离散程度
float nrSamples = 0.0; 
for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
{
    for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
    {
        // spherical to cartesian (in tangent space) 球面->笛卡尔系
        vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
        // tangent space to world
        vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

        irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
        nrSamples++;//记录采样次数
    }
}
irradiance = PI * irradiance * (1.0 / float(nrSamples));
```

**随着Pixel Shader的每一次执行，所有的N都会遍历，所有方向的入射辐照度都会被计算出来。**

接下来结合进OpenGL的代码中，首先我们创建辐照度立方体贴图（同样，我们只需要在渲染循环之前执行一次）：

```cpp
unsigned int irradianceMap;
glGenTextures(1, &irradianceMap);
glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
for (unsigned int i = 0; i < 6; ++i)
{
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, 
                 GL_RGB, GL_FLOAT, nullptr);
}
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
```

由于辐照度图均匀地平均所有周围辐射，因此它不具有大量高频细节，因此我们可以以低分辨率（32x32）存储地图，并让OpenGL的线性滤波完成大部分工作。接下来，我们将捕获帧缓冲区重新缩放到新的分辨率：

```cpp
glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32); 
```

使用卷积着色器，我们以与捕获环境立方体贴图类似的方式卷积环境贴图：

```cpp
irradianceShader.use();
irradianceShader.setInt("environmentMap", 0);
irradianceShader.setMat4("projection", captureProjection);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
for (unsigned int i = 0; i < 6; ++i)
{
    irradianceShader.setMat4("view", captureViews[i]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderCube();
}
glBindFramebuffer(GL_FRAMEBUFFER, 0);  
```

这样我们就得到了一个预先计算的辐照度贴图，我们可以直接将其用于基于漫反射图像的照明。

#### PBR与间接辐照光

我们把预计算出来的辐照度贴图应用再PBR Shader上，取代之前写死的Ambient Light

```cpp
// vec3 ambient = vec3(0.03);
vec3 ambient = texture(irradianceMap, N).rgb;
 
vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness); 
vec3 kD = 1.0 - kS;
vec3 irradiance = texture(irradianceMap, N).rgb;
vec3 diffuse    = irradiance * albedo;
vec3 ambient    = (kD * diffuse) * ao;
```

菲涅尔部分：

```cpp
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   
```

现在我们可以得到一个图，但是我们现在只考虑了漫反射，缺乏高光，在后面我们会加上高光来实现一个真正的简单版的PBR