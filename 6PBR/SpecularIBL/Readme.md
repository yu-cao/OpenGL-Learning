### 高光IBL

本节主要关注渲染方程中的镜面反射部分





注意到Cook-Torrance镜面反射部分（乘以$k_S$）在积分上不是常数，而是取决于入射光方向，还取决于观察视角方向。试图解决所有入射光方向（包括所有可能的视图方向）的积分是非常昂贵的。 Epic Games提出了一个解决方案，他们能够给出一些妥协的情况下预先卷积镜面部分来满足实时需求，称为`split sum approximation`。

也就是说把反射方程的镜面部分，分成两个独立的部分，我们可以单独卷积，然后在PBR着色器中组合，用于镜面间接IBL。类似于我们如何预先对irradiance map进行卷积，split sum approximation需要HDR环境贴图作为其卷积输入。为了理解分裂和近似，我们将再次看反射方程，但这次只关注镜面反射部分（我们在前一个教程中提取了漫反射部分）：

$L_o(p,\omega_o) =  \int\limits_{\Omega} (k_s\frac{DFG}{4(\omega_o \cdot n)(\omega_i \cdot n)} 	L_i(p,\omega_i) n \cdot \omega_i  d\omega_i = 	\int\limits_{\Omega} f_r(p, \omega_i, \omega_o) L_i(p,\omega_i) n \cdot \omega_i  d\omega_i$

但是基于同样的道理，我们不能实时求解积分的镜面部分，而是最好跟前面的漫反射一样弄一个像镜面IBL的东西，然后带着片段的法线对这个map进行采样。但是这里的问题是我们能够预计算irradiance map是因为它只依赖$\omega_i$的积分，而且我们可以把恒定的漫反射反射率移出去，而高光部分不是那么简单：

$f_r(p, w_i, w_o) = \frac{DFG}{4(\omega_o \cdot n)(\omega_i \cdot n)}$

这个积分也是取决于$w_0$，但是我们不能用两个向量真正对预计算的cubemap进行采样，位置p是无关的。预计算$w_i$和$w_0$的各种组合是不实际的行为。

Epic Games的split sum approximation使用把这个计算分解成独立的两部分进行处理来解决这个问题：

$$L_o(p,\omega_o) =  \int\limits_{\Omega} L_i(p,\omega_i) d\omega_i	* 	\int\limits_{\Omega} f_r(p, \omega_i, \omega_o) n \cdot \omega_i d\omega_i$$

第一部分被称为预滤波环境贴图(pre-filtered environment map)，与之前的辐照度图(irradiance map)很像，它是一个预计算的环境卷积图，只不过这里考虑了roughness。为了增加粗糙度水平，环境贴图会使用更多的分散的取样向量进行卷积，导致更加模糊的反射。对于我们卷积得到的每个粗糙度层级，我们将模糊的结果顺序存储到pre-filtered environment map的mipmap层级之中。

例如，pre-filtered environment map在其5个mipmap级别中存储5个不同粗糙度值的预卷积结果，如下所示：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/SpecularIBL/Reference/ibl_prefilter_map.png)

我们使用Cook-Torrance BRDF的正态分布函数（NDF）来生成样本向量及其散射强度，该函数将法线和观察方向作为输入。由于我们在卷积环境地图时事先不知道观察方向，因此Epic Games通过假设观察方向（即高光反射方向）总是等于输出样本方向$w_0$来进一步近似。这会转换为以下代码：

```vert
vec3 N = normalize(w_o);
vec3 R = N;
vec3 V = R;
```

这样，pre-filtered environment convolution不需要知道观察方向。这意味着当从一个角度观察镜面表面反射时，我们不会得到很好的镜面反射。然而，这通常被认为是一个可接受的妥协，如下图所示：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/SpecularIBL/Reference/ibl_grazing_angles.png)

第二部分等于BRDF的镜面积分部分。如果我们假定每个方向的入射辐射亮度都是白色的（即$L(p, x) = 1.0$）我们可以根据输入的roughness和一个介于法线n到光线方向$w_i$（或者$n · w_i$）的角度预计算出BRDF的response。

Epic Games将预先计算的BRDF对每个法线和光线方向组合的响应存储在2D查找纹理（2D lookup texture, LUT）中，用不同粗糙度值来体现，称为BRDF积分图(BRDF integration map)。 2D查找纹理向表面的菲涅耳响应输出比例（红色）和偏差值（绿色），为我们提供split specular积分的第二部分：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/SpecularIBL/Reference/ibl_brdf_lut.png)

我们这个生成得到的lookup texture，对每个水平纹理坐标(0.0~1.0)来说作为BRDF的输入$n·w_i$ (`NdotV`)，对于每个垂直坐标纹理来说是作为roughness的值，使用这个BRDF积分图和预过滤的环境贴图我们能够组合出镜面反射的结果：

```frag
float lod             = getMipLevelFromRoughness(roughness);
vec3 prefilteredColor = textureCubeLod(PrefilteredEnvMap, refVec, lod);
vec2 envBRDF          = texture2D(BRDFIntegrationMap, vec2(NdotV, roughness)).xy;
vec3 indirectSpecular = prefilteredColor * (F * envBRDF.x + envBRDF.y) 
```

以上就是Epic Games的split sum approximation的实践。

#### 预过滤一个HDR环境贴图

这与我们之前对irradiance map进行卷积很像，区别在于我们现在需要考虑roughness而且要根据预过滤贴图的mip level来按顺序存储rougher reflection

首先，我们要生成一个新的立方体贴图来保存这个pre-filtered environment map，为了确保我们为mip levels分配足够的内存，我们使用`glGenerateMipmap`来简单分配内存：

```cpp
unsigned int prefilterMap;
glGenTextures(1, &prefilterMap);
glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
for (unsigned int i = 0; i < 6; ++i)
{
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
}
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
```

因为我们打算在`prefilterMap`它的mipmaps中进行采样，所以必须确保它的minification filter设置成为`GL_LINEAR_MIPMAP_LINEAR`来保证三线过滤。我们把预过滤的镜面反射的每个面都存储为128×128作为base mip level。对于大多数反射来说，这可能已经足够了，但如果有大量光滑材料（如汽车dm反射），就可能需要再提高分辨率。

之前我们通过使用球面坐标系来生成均匀分布在半球&Omega;上采样矢量，转化了environment map。尽管这在辐照度(irradiance)上有效，但是用于镜面反射的效果就很差，当涉及到镜面反射时，基于表面的粗糙度，光大致围绕着法线**n**进行反射：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/SpecularIBL/Reference/ibl_specular_lobe.png)

可能的反射光线的方向组合形成的图像被称为Specular lobe，随着粗糙度增加，lobe的大小也会增加，形状也会随着入射光方向而变化。Specular lobe的形状高度依赖于材质本身。

当我们涉及微表面模型时，可以将Specular lobe想象为给出一些入射光方向的微平面中间向量的反射方向。可以看到大多数光线最终反射在微平面中间矢量周围的Specular lobe中，这使得用类似方式生成样本矢量在大多数情况下是有意义的，这个过程被称为重要性采样(`importance sampling`）

####蒙特卡洛积分和重要性采样

蒙特卡洛积分：
$$
O=\int_{a}^{b} f(x) d x=\frac{1}{N} \sum_{i=0}^{N-1} \frac{f(x)}{p d f(x)}
$$
其中pdf代表概率密度函数（probability density function）

低差异序列是一种特殊的生成随机数的方法，可以生成一组“并不是那么随机的随机数”。我们可以对低差异序列（low-discrepancy sequences）进行蒙特卡罗积分，这些序列仍会生成随机样本，但每个样本的分布更均匀：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/SpecularIBL/Reference/ibl_low_discrepancy_sequence.png)

当使用低差异序列生成蒙特卡罗样本向量时，该过程称为准蒙特卡罗积分。准蒙特卡罗方法具有更快的收敛速度，在同样的sample数量下获得更好的效果。

我们可以使用一个有趣的属性来实现更快的收敛速度，称为重要性采样，我们之前已经提到过它。但是当涉及光的镜面反射时，反射光矢量被约束在Specular lobe中，其尺寸由表面的粗糙度决定。看到Specular lobe外的任何（准）随机生成的sample与镜面积分无关，将sample生成集中在Specular lobe内是有意义的，而代价是蒙特卡罗估计偏差。

这就是importance sampling的本质：在一些区域内生成样本矢量，这些区域受到围绕微平面中间向量的粗糙度的限制。通过将准蒙特卡罗采样与低差异序列相结合并使用重要性采样偏置采样矢量，我们获得了高收敛率。因为我们以更快的速度到达解决方案，所以我们只需要更少的样本就能达到足够的近似值。因此，该组合甚至允许图形应用程序实时解决镜面反射积分，尽管它仍然比预先计算结果慢得多。

#### 低差异化序列

我们打算使用importance sampling来预计算间接反射方程的镜面反射部分，给出基于准蒙特卡罗方法的随机低差异序列。我们将会使用名为*Hammersley Sequence*的序列，它基于*Van Der Corpus*序列，它反映了小数点周围的十进制数的二进制表示。

```cpp
float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}  
```

GLSL的Hammersley函数为我们提供了大小为N的总样本集的低差异样本i。

如果OpenGL版本没有提供位操作(比如WebGL和OpenGL ES 2.0等)我们可以这样生成序列（性能较差，但保证所有平台都可以运行）：

```glsl
float VanDerCorpus(uint n, uint base)
{
    float invBase = 1.0 / float(base);
    float denom   = 1.0;
    float result  = 0.0;

    for(uint i = 0u; i < 32u; ++i)
    {
        if(n > 0u)
        {
            denom   = mod(float(n), 2.0);
            result += denom * invBase;
            invBase = invBase / 2.0;
            n       = uint(float(n) / 2.0);
        }
    }

    return result;
}
// ----------------------------------------------------------------------------
vec2 HammersleyNoBitOps(uint i, uint N)
{
    return vec2(float(i)/float(N), VanDerCorpus(i, 2u));
}
```

#### GGX重要性采样

我们将生成基于表面粗糙度偏向于微表面中间向量的一般反射方向的样本向量，而不是均匀或随机（蒙特卡罗）在积分半球&Omega;上生成样本向量。采样过程将类似于我们之前看到的：开始一个大循环，生成一个随机（低差异）序列值，取序列值在切线空间中生成一个样本向量，转换到世界空间并采样场景的radiance。不同的是，我们现在使用低差异序列值作为输入来生成样本向量：

```glsl
const uint SAMPLE_COUNT = 4096u;
for(uint i = 0u; i < SAMPLE_COUNT; ++i)
{
    vec2 Xi = Hammersley(i, SAMPLE_COUNT);   
```

另外，为了构建样本矢量，我们需要一些方法来定向和偏置那些*在一些表面粗糙度下朝向Specular lobe*的样本矢量，我们可以按照之前的教程来抹去NDF，并将GGX NDF结合到球形样本矢量过程中：

```glsl
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}  
```

这给了我们一个样本向量，它基于一些输入粗糙度和低差异序列值Xi，在预期的微表面的中间向量周围。请注意，Epic Games使用**平方粗糙度**相比于Disney最初的PBR研究而言有更好的视觉效果。

最后总结到一个pre-filter convolution shader中：

```glsl
#version 330 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;
uniform float roughness;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits);
vec2 Hammersley(uint i, uint N);
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness);
  
void main()
{		
    vec3 N = normalize(localPos);    
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;   
    vec3 prefilteredColor = vec3(0.0);     
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            prefilteredColor += texture(environmentMap, L).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    FragColor = vec4(prefilteredColor, 1.0);
}
```

我们根据一些输入的roughness来预先过滤环境，这些roughness在pre-filter cubemap的每个mipmap level（从0.0到1.0）中变化，并将结果存储在`prefilteredColor`中。得到的预过滤颜色除以总样品权值，其中对最终结果影响较小的样品（`NdotL`较小）对最终权重的贡献较小。

####捕获预过滤器mipmap级别

剩下要做的是让OpenGL在多个mipmap级别上对不同粗糙度值的环境贴图进行预过滤。

```cpp
prefilterShader.use();
prefilterShader.setInt("environmentMap", 0);
prefilterShader.setMat4("projection", captureProjection);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
unsigned int maxMipLevels = 5;
for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
{
    // reisze framebuffer according to mip-level size.
    unsigned int mipWidth  = 128 * std::pow(0.5, mip);
    unsigned int mipHeight = 128 * std::pow(0.5, mip);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
    glViewport(0, 0, mipWidth, mipHeight);

    float roughness = (float)mip / (float)(maxMipLevels - 1);
    prefilterShader.setFloat("roughness", roughness);
    for (unsigned int i = 0; i < 6; ++i)
    {
        prefilterShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderCube();
    }
}
glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

该过程类似于辐照度图卷积，但这次我们将帧缓冲区的尺寸缩放到适当的mipmap的大小，每个mip级别将尺寸减小2。另外，我们指定我们在`glFramebufferTexture2D`的最后一个参数中渲染的mip level，将我们预过滤的粗糙度传递给预过滤器着色器。

这将给我们一个适当的预过滤环境贴图，它返回模糊反射，我们访问它的更高的mip level，这个反射也就越模糊。如果我们在天空盒着色器中显示预过滤的环境立方体贴图，并在其着色器中优先采样高于其第一个mip级别：

```glsl
vec3 envColor = textureLod(environmentMap, WorldPos, 1.2).rgb; 
```

我们就可以直接观察这个结果，确实看起来像原始环境的模糊版本。

#### 预滤波器卷积中的亮点(Bright dots in the pre-filter convolution)

由于镜面反射中的高频细节和剧烈变化的光强度，使镜面反射卷积需要大量样本以适当地解释HDR环境反射的广泛变化的性质。我们已经采集了大量样本，但在某些环境中，在某些较粗糙的mip级别上可能仍然不够，在这种情况下，我们会看到明亮区域周围出现点状图案：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/SpecularIBL/Reference/ibl_prefilter_dots.png)

一种选择是进一步增加样本数，但这不能适应所有环境。

我们可以通过（在pre-filter卷积中）不直接对环境贴图采样来减少瑕疵，而是基于积分的pdf和粗糙度来对环境贴图的一个mipmap level进行采样：

```glsl
float D   = DistributionGGX(NdotH, roughness);
float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001; 

float resolution = 512.0; // resolution of source cubemap (per face)
float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
```

当想从它的mip level进行采样时，不要忘记在环境贴图上启用三线过滤：

```cpp
glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
```

然后让OpenGL在设置立方体贴图的基本纹理后生成mipmap：

```cpp
// convert HDR equirectangular environment map to cubemap equivalent
[...]
// then generate mipmaps
glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
```

这种效果非常好，并且应该在粗糙表面上的预过滤器地图中删除大多数（如果不是全部）点。

#### 预计算BRDF

重新回顾一下split sum approximation：
$$
L_{o}\left(p, \omega_{o}\right)=\int_{\Omega} L_{i}\left(p, \omega_{i}\right) d \omega_{i} * \int_{\Omega} f_{r}\left(p, \omega_{i}, \omega_{o}\right) n \cdot \omega_{i} d \omega_{i}
$$
我们已经通过根据不同的粗糙度等级用pre-filter map预计算了左边的积分。右边的积分要求我们围绕角度$n · w_in · w_i$、roughness和菲涅耳的$F_0$ 对BRDF方程进行卷积。这类似于对纯白色环境贴图或辐照度为常量1.0的BRDF积分。对3个变量卷积太难了，**但我们可以尝试把$F_0$ 移到积分外面**：
$$
\int_{\Omega} f_{r}\left(p, \omega_{i}, \omega_{o}\right) n \cdot \omega_{i} d \omega_{i}=\int_{\Omega} f_{r}\left(p, \omega_{i}, \omega_{o}\right) \frac{F\left(\omega_{o}, h\right)}{F\left(\omega_{o}, h\right)} n \cdot \omega_{i} d \omega_{i} = \int_{\Omega} \frac{f_{r}\left(p, \omega_{i}, \omega_{o}\right)}{F\left(\omega_{o}, h\right)} F\left(\omega_{o}, h\right) n \cdot \omega_{i} d \omega_{i}
$$
用Fresnel-Schlick近似代替右边的F，得到：
$$
\int_{\Omega} \frac{f_{r}\left(p, \omega_{i}, \omega_{o}\right)}{F\left(\omega_{o}, h\right)}\left(F_{0}+\left(1-F_{0}\right)\left(1-\omega_{o} \cdot h\right)^{5}\right) n \cdot \omega_{i} d \omega_{i}
$$
令$\left(1-\omega_{o} \cdot h\right)^{5} =  \alpha$，以$F_0 $为主项整理：
$$
\int_{\Omega} \frac{f_{r}\left(p, \omega_{i}, \omega_{o}\right)}{F\left(\omega_{o}, h\right)}\left(F_{0}+\left(1-F_{0}\right) \alpha\right) n \cdot \omega_{i} d \omega_{i} = \int_{\Omega} \frac{f_{r}\left(p, \omega_{i}, \omega_{o}\right)}{F\left(\omega_{o}, h\right)}\left(F_{0}+1 * \alpha-F_{0} * \alpha\right) n \cdot \omega_{i} d \omega_{i} = \int_{\Omega} \frac{f_{r}\left(p, \omega_{i}, \omega_{o}\right)}{F\left(\omega_{o}, h\right)}\left(F_{0} *(1-\alpha)+\alpha\right) n \cdot \omega_{i} d \omega_{i}
$$
现在，我们终于可以拆开这个积分了：
$$
\int_{\Omega} \frac{f_{r}\left(p, \omega_{i}, \omega_{o}\right)}{F\left(\omega_{o}, h\right)}\left(F_{0} *(1-\alpha)\right) n \cdot \omega_{i} d \omega_{i}+\int_{\Omega} \frac{f_{r}\left(p, \omega_{i}, \omega_{o}\right)}{F\left(\omega_{o}, h\right)}(\alpha) n \cdot \omega_{i} d \omega_{i}
$$
又因为$F_0 $是对积分而言是常数，可以提出来，再将&alpha;变成原来的形式，得到最后的拆分后结果：
$$
F_{0} \int_{\Omega} f_{r}\left(p, \omega_{i}, \omega_{o}\right)\left(1-\left(1-\omega_{o} \cdot h\right)^{5}\right) n \cdot \omega_{i} d \omega_{i}+\int_{\Omega} f_{r}\left(p, \omega_{i}, \omega_{o}\right)\left(1-\omega_{o} \cdot h\right)^{5} n \cdot \omega_{i} d \omega_{i}
$$
这两个积分分别表示对F0的缩放和偏移。注意，由于$f\left(p, \omega_{i}, \omega_{o}\right)$已经包含了F项，他们抵消了，所以将分母中的F从f中移除了。

类似之前的卷积操作，我们可以针对BRDF的输入参数进行卷积：n和ωo之间的角度，以及粗糙度。然后将结果保存到贴图中。我们将卷积结果保存到一个2D的查询纹理（LUT），即BRDF积分贴图，稍后用于PBR光照shader，得到最终的非直接specular光照结果。

BRDF卷积shader在二维平面上工作，它使用2D纹理坐标直接作为输入参数，以此卷积BRDF（`NdotV` 和`roughness`）。卷积代码和pre-filter卷积很相似，除了它现在根据BRDF的几何函数G和Fresnel-Schlick近似来处理采样向量：

```glsl
vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}
// ----------------------------------------------------------------------------
void main() 
{
    vec2 integratedBRDF = IntegrateBRDF(TexCoords.x, TexCoords.y);
    FragColor = integratedBRDF;
}
```

BRDF卷积就是直接把数学公式转换为代码。我们以角度θ和粗糙度为输入参数，用重要性采样方法生成采样向量，用几何函数G和菲涅耳项处理它，输出F0的缩放和偏移量，最后求平均值。

与之前的教程不同，当与IBL一起使用时，BRDF的几何项略有不同，因为它的k变量的解释略有不同：
$$
\begin{array}{c}{k_{\text {direct}}=\frac{(\alpha+1)^{2}}{8}} \\ {k_{I B L}=\frac{\alpha^{2}}{2}}\end{array}
$$
由于BRDF卷积是镜面反射IBL积分的一部分，我们将使用$k_{IBL}$作为Schlick-GGX几何函数：

```glsl
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}  
```

注意，尽管k以α为输入参数，我们没有将roughness 的平方作为α，我们之前却是这么做的。可能这里的α已经平方过了。我不确定这是Epic Games的不一致性，还是最初的Disney文献的问题，但是直接将roughness 传递给α，确实得到了与Epic游戏公司相同的BRDF积分贴图。

最后，为保存BRDF卷积结果，我们创建一个二维纹理，尺寸为512x512：

```cpp
unsigned int brdfLUTTexture;
glGenTextures(1, &brdfLUTTexture);

// pre-allocate enough memory for the LUT texture.
glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
```

注意，我们使用16位精度浮点数格式，这是Epic Games所推荐的。确保wrapping模式为`GL_CLAMP_TO_EDGE` ，防止边缘采样瑕疵。

然后，我们复用相同的Framebuffer对象，在NDC屏幕空间四边形中运行shader：

```cpp
glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

glViewport(0, 0, 512, 512);
brdfShader.use();
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
RenderQuad();

glBindFramebuffer(GL_FRAMEBUFFER, 0);  
```

拆分求和积分的BRDF部分的卷积会得到如下图所示的结果（与上面那个图一样）：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/SpecularIBL/Reference/ibl_brdf_lut.png)

有了pre-filter环境贴图和BRDF的二维LUT贴图，我们可以根据拆分求和近似来重构非直接specular积分。相乘的结果就是非直接或环境specular光。

#### 完成IBL反射

为了让反射率方程中的间接specular部分运行，我们需要将split sum approximation的两部分钉在一起。开始时，让我们把预计算的光照数据放到PBR shaer的开头：

```glsl
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;
```

首先，使用反射向量，通过对pre-filter环境贴图采样，我们得到表面的间接specular反射强度。注意，我们根据表面的粗糙度度对最接近的mipmap层采样，这会让更粗糙的表面表现出更模糊的反射效果。

```cpp
void main()
{
    [...]
    vec3 R = reflect(-V, N);   

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    [...]
}
```

在pre-tilter步骤中我们只卷积了5个mipmap层（0-4），我们这里用`MAX_REFLECTION_LOD`确保不对没有数据的位置采样。

给定材质的粗糙度和角度（法线和观察者之间的角度），我们从BRDF的查询纹理中采样：

```glsl
vec3 F        = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
vec2 envBRDF  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
```

从BRDF的查询纹理中找到了F0的缩放和偏移（这里我们直接使用了间接菲涅耳函数F），我们联合IBL反射率方程左边的pre-filter部分，重构出specular的近似积分值。

这就得到了反射率方程的间接specular部分。现在，联合反射率方程的diffuse部分（从上一篇文章中）我们就得到了PBR全部的IBL结果：

```glsl
vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

vec3 kS = F;
vec3 kD = 1.0 - kS;
kD *= 1.0 - metallic;	  
  
vec3 irradiance = texture(irradianceMap, N).rgb;
vec3 diffuse    = irradiance * albedo;
  
const float MAX_REFLECTION_LOD = 4.0;
vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;   
vec2 envBRDF  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
  
vec3 ambient = (kD * diffuse + specular) * ao; 
```

注意，我们没有用kS 乘以specular ，因为我们这里已经有个菲涅耳乘过了。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/SpecularIBL/Reference/ibl_specular_result.png)

<hr>

尾声：

我们在程序开始时预计算了所有相关的IBL数据，然后启动渲染循环。这对教程是没问题的，但是对任何实际应用的软件是不可行的。首先，预计算只需要完成一次，不需要每次启动都做。其次，使用多环境贴图时，你不得不每次都把它们全算一遍，这太浪费了。

因此，你应该将预计算结果保存到硬盘上（注意BRDF积分贴图不依赖环境贴图，所以你只需计算和加载一次）。这意味着你需要找一个自定义的图片格式来存储HDR的cubemap，包括它们的各个mipmap层。或者，你用现有的格式（比如*.dss支持保存多个mipmap层）。

此外，我们在教程中描述了预计算的所有过程，这是为了加深对PBR管道的理解。你完全可以用一些优秀的工具（例如[cmftStudio](https://github.com/dariomanesku/cmftStudio) 或[IBLBaker](https://github.com/derkreature/IBLBaker) ）来为你完成这些预计算。

我们跳过的一个点，是将预计算的cubemap用作反射探针（Reflection Probes）：cubemap插值和视差校正。这个说的是，在你的场景中放置几个反射探针，它们在各自的位置拍个cubemap快照，卷积为IBL数据，用作场景中那一部分的光照计算。通过在camera最近的几个探针间插值，我们可以实现局部高细节度的IBL光照（只要我们愿意放置够多的探针）。这样，当从一个明亮的门外部分移动到比较暗的门内部分时，IBL光照可以正确地更新。

文献延伸：

- [Real Shading in Unreal Engine 4](http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf): explains Epic Games' split sum approximation. This is the article the IBL PBR code is based of.
- [Physically Based Shading and Image Based Lighting](http://www.trentreed.net/blog/physically-based-shading-and-image-based-lighting/): great blog post by Trent Reed about integrating specular IBL into a PBR pipeline in real time.
- [Image Based Lighting](https://chetanjags.wordpress.com/2015/08/26/image-based-lighting/): very extensive write-up by Chetan Jags about specular-based image-based lighting and several of its caveats, including light probe interpolation.
- [Moving Frostbite to PBR](https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf): well written and in-depth overview of integrating PBR into a AAA game engine by Sébastien Lagarde and Charles de Rousiers.
- [Physically Based Rendering – Part Three](https://jmonkeyengine.github.io/wiki/jme3/advanced/pbr_part3.html): high level overview of IBL lighting and PBR by the JMonkeyEngine team.
- [Implementation Notes: Runtime Environment Map Filtering for Image Based Lighting](https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/): extensive write-up by Padraic Hennessy about pre-filtering HDR environment maps and significanly optimizing the sample process.