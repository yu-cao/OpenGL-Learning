<h2>光照（PBR实践）</h2>

核心就是处理光照的片元着色器，目标是实现这个方程

<img src="http://latex.codecogs.com/svg.latex?L_o(p,\omega_o) = \int\limits_{\Omega} (k_d\frac{c}{\pi} + k_s\frac{DFG}{4(\omega_o \cdot n)(\omega_i \cdot n)})L_i(p,\omega_i) n \cdot \omega_i  d\omega_i" />

首先我们需要把PBR相关的输入放进片段着色器

```glsl
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform vec3 camPos;

uniform vec3  albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

//获取法向量和从frag->观察点的向量（任何光照都需要的计算）
void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);
    
    //...
}
```

我们会采用总共4个点光源来直接表示场景的辐照度。为了满足反射率方程，我们循环遍历每一个光源，计算他们独立的辐射率然后求和，接着根据BRDF和光源的入射角来缩放该辐射率。

```glsl
//反射方程
vec3 Lo = vec3(0.0);
for(int i = 0; i < 4; ++i)
{
    //首先我们来计算一些可以预计算的光照变量
    //计算每个光的半径
    vec3 L = normalize(lightPositions[i] - WorldPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPositions[i] - WorldPos);
    float attenuation = 1.0 / (distance * distance);//基于线性空间的符合物理的计算（Gamma校正会在最后完成）
    vec3 radiance = lightColors[i] * attenuation;
    
    //...
}
```

我们希望对于每个光源都要计算完整的Cook-Torrance镜面的BRDF项：

<img src="http://latex.codecogs.com/svg.latex?\frac{DFG}{4(\omega_o \cdot n)(\omega_i \cdot n)}" />

这分为3部分，也就是正态分布函数D，几何函数G，菲涅尔反射F

首先计算菲涅尔反射

```glsl
//返回的是一个物体表面光线被反射的百分比，也就是我们反射方程中的参数ks
//F0表示0°入射角下的反射（垂直表面）
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

//对于非金属材质来说F0永远保持0.04这个值
//我们会根据表面的金属性来改变F0这个值，并且在原来的F0和反射率中插值计算F0
vec3 F0 = vec3(0.04); 
F0      = mix(F0, albedo, metallic);

vec3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);
```

接下来是D和G

```glsl
//下面所有函数都是直接传了粗糙度给函数，这与之前不同
//通过这种方式，我们可以针对每一个不同的项对粗糙度做一些修改
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

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

由此，我们顺理成章地在计算反射的循环中计算NDF和G项

```glsl
// Cook-Torrance BRDF
float NDF = DistributionGGX(N, H, roughness);
float G   = GeometrySmith(N, V, L, roughness);
```

至此，我们已经得到了足够的条件计算Cook-Torrance的BRDF

```glsl
vec3 nominator    = NDF * G * F;
float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
vec3 specular = nominator / max(denominator, 0.001);//在分母项中加了一个0.001为了避免出现除零错误
```

现在到了最后的时候了，我们可以计算每个光源在反射率方程中的贡献值了；菲涅尔方程直接给出了kS， 我们可以使用F表示镜面反射在所有打在物体表面上的光线的贡献。 从kS我们很容易计算折射的比值kD：

```glsl
//Ks等于菲涅尔反射结果
vec3 kS = F;
//为了保证能量守恒，反射+折射光线比例不能超过1（除非有自发光），为了保持这个关系，我们把diffuse部分KD等于1-Ks
vec3 kD = vec3(1.0) - kS;
//KD乘以金属值的取反，保证纯金属的没有diffuse light，依靠金属值进行线性插值
kD *= 1.0 - metallic;
```

```glsl
//N点乘L的大小
float NdotL = max(dot(N, L), 0.0);

//加到出射光线L0
Lo += (kD * albedo / PI + specular) * radiance * NdotL;//注意我们没有把kS乘进去我们的反射率方程中，这是因为我们已经在specualr BRDF中乘了菲涅尔系数F了，因为kS等于F，因此我们不需要再乘一次
```

剩下的工作就是加一个环境光照项给Lo，加上Gamma校正，然后我们就拥有了片段的最后颜色：

```glsl
// ambient lighting (note that the next IBL tutorial will replace
// this ambient lighting with environment lighting).
vec3 ambient = vec3(0.03) * albedo * ao;

vec3 color = ambient + Lo;

// HDR tonemapping
color = color / (color + vec3(1.0));
// gamma correct
color = pow(color, vec3(1.0/2.2));

FragColor = vec4(color, 1.0);
```

<hr>

<h3>带贴图的PBR</h3>

传入美术人员给的`albedoMap`，`normalMap`，`metallicMap`，`roughnessMap`和`aoMap`更好地提升画质

```glsl
[...]
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

//从切线空间转换到世界空间
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{
    vec3 albedo     = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
    float metallic  = texture(metallicMap, TexCoords).r;
    float roughness = texture(roughnessMap, TexCoords).r;
    float ao        = texture(aoMap, TexCoords).r;

    vec3 N = getNormalFromMap();
    
    //...
}
```