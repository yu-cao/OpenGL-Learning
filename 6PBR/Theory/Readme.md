<h2>PBR 原理</h2>

基于物理的渲染。为了使用一种更符合物理学规律的方式来模拟光线，因此这种渲染方式与我们原来的Phong或者Blinn-Phong光照算法相比总体上看起来要更真实一些。

判断一种PBR光照模型是否是基于物理的，必须满足以下三个条件：

1. 基于微平面(Microfacet)的表面模型。
2. 能量守恒。
3. 应用基于物理的BRDF。

<hr>

<h3>微平面模型</h3>

达到微观尺度之后任何平面都可以用被称为微平面(Microfacets)的细小镜面来进行描绘。根据平面粗糙程度的不同，当我们特指镜面光/镜面反射时，入射光线更趋向于向完全不同的方向发散(Scatter)开来，进而产生出分布范围更广泛的镜面反射。而与之相反的是，对于一个光滑的平面，光线大体上会更趋向于向同一个方向反射，造成更小更锐利的反射

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/microfacets_light_rays.png)

在微观尺度下，没有任何平面是完全光滑的。然而由于这些微平面已经微小到无法逐像素的继续对其进行区分，因此我们只有假设一个粗糙度(Roughness)参数，然后用统计学的方法来概略的估算微平面的粗糙程度。我们可以基于一个平面的粗糙度来计算出某个向量的方向与微平面平均取向方向一致的概率。这个向量便是位于光线向量l和视线向量v之间的中间向量

<img src="http://latex.codecogs.com/svg.latex?\overline{H}=\frac {\overline{L}+\overline{V}}{\|\overline {L}+\overline{V}\|}" />

微平面的取向方向与中间向量的方向越是一致，镜面反射的效果就越是强烈越是锐利。然后再加上一个介于0到1之间的粗糙度参数，这样我们就能概略的估算微平面的取向情况了：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/ndf.png)

<h3>能量守恒</h3>

出射光线的能量永远不能超过入射光线的能量（发光面除外）

为了遵守能量守恒定律，我们需要对漫反射光和镜面反射光之间做出明确的区分。当一束光线碰撞到一个表面的时候，它就会分离成一个折射部分和一个反射部分。反射部分就是会直接反射开来而不会进入平面的那部分光线，这就是我们所说的镜面光照。而折射部分就是余下的会进入表面并被吸收的那部分光线，这也就是我们所说的漫反射光照。

我们假设对平面上的每一点所有的折射光都会被完全吸收而不会散开。而有一些被称为次表面散射(Subsurface Scattering)技术的着色器技术将这个问题考虑了进去，它们显著的提升了一些诸如皮肤，大理石或者蜡质这样材质的视觉效果，不过伴随而来的则是性能下降代价。

对于金属(Metallic)表面，当讨论到反射与折射的时候还有一个细节需要注意。金属表面对光的反应与非金属材料还有电介质(Dielectrics)材料表面相比是不同的。它们遵从的反射与折射原理是相同的，但是**金属表面所有的折射光都会被直接吸收而不会散开，只留下反射光或者说镜面反射光**。亦即是说，金属表面不会显示出漫反射颜色。由于金属与电介质之间存在这样明显的区别，因此它们两者在PBR渲染管线中被区别处理

反射光与折射光它们二者之间是互斥的关系。无论何种光线，其被材质表面所反射的能量将无法再被材质吸收。

```glsl
float kS = calculateSpecularComponent(...); // 反射/镜面 部分
float kD = 1.0 - ks;                        // 折射/漫反射 部分
```

<h3>渲染方程</h3>

<img src="http://latex.codecogs.com/svg.latex?L_o(p,\omega_o) = \int\limits_{\Omega} f_r(p,\omega_i,\omega_o) L_i(p,\omega_i) n \cdot \omega_i  d\omega_i" />

辐射通量：辐射通量Φ表示的是一个光源所输出的能量，以瓦特为单位。辐射通量将会计算由不同波长构成的函数的总面积。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/daylight_spectral_distribution.png)

立体角：立体角用ω表示，它可以为我们描述投射到单位球体上的一个截面的大小或者面积。投射到这个单位球体上的截面的面积就被称为立体角(Solid Angle)，可以把立体角想象成为一个带有体积的方向：（把自己想象成为一个站在单位球面的中心的观察者，向着投影的方向看。这个投影轮廓的大小就是立体角）

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/solid_angle.png)

辐射强度*I*：辐射强度(Radiant Intensity)表示的是在单位球面上，一个光源向每单位立体角所投送的辐射通量（计算公式也就是辐射通量除以立体角）

<img src="http://latex.codecogs.com/svg.latex?I = \frac{d\Phi}{d\omega}" />

辐射率方程式：一个拥有辐射强度Φ的光源在单位面积A，单位立体角ω上的辐射出的总能量；也就是代表通过某个无限小的立体角ωi在某个点上的辐射率

<img src="http://latex.codecogs.com/svg.latex?L=\frac{d^2\Phi}{ dA d\omega \cos\theta}" />

辐射率是辐射度量学上表示一个区域平面上光线总量的物理量，它受到入射(Incident)（或者来射）光线与平面法线间的夹角θ的余弦值cosθ的影响：当直接辐射到平面上的程度越低时，光线就越弱，而当光线完全垂直于平面时强度最高。cosθ 就直接对应于光线的方向向量和平面法向量的点积：`float cosTheta = dot(lightDir, N);`

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/radiance.png)

事实上，当涉及到辐射率时，我们通常关心的是所有投射到点p上的光线的总和，而这个和就称为辐射照度或者辐照度

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/hemisphere.png)

我们知道在渲染方程中L代表通过某个无限小的立体角ωi在某个点上的辐射率，而立体角可以视作是**入射方向向量ωi**。注意我们利用**光线和平面间的入射角的余弦值cosθ来计算能量，亦即从辐射率公式L转化至反射率公式时的n⋅ωi**。**ωo表示观察方向**，也就是出射方向。**反射率公式计算了点p在ωo方向上被反射出来的辐射率Lo(p,ωo)的总和。**或者换句话说：**Lo表示了从ωo方向上观察，光线投射到点p上反射出来的辐照度。**

然后对于半球上的每一个立体角进行一个球面积分，这个问题就转化为，在半球领域Ω中按一定的步长将反射率方程分散求解，然后再按照步长大小将所得到的结果平均化。（因为不存在解析解）

我们现在还没有搞定的是`fr(p,wi,w0)`这个函数，这就是BRDF

<h3>BRDF与Cook-Torrance模型</h3>

双向反射分布函数(BRDF)，它接受入射（光）方向ωi，出射（观察）方向ωo，平面法线n以及一个用来表示微平面粗糙程度的参数a作为函数的输入参数。BRDF可以近似的求出每束光线对一个给定了材质属性的平面上最终反射出来的光线所作出的贡献程度。

比如，如果一个平面拥有完全光滑的表面（比如镜面），那么对于所有的入射光线ωi（除了一束以外）而言BRDF函数都会返回0.0，只有一束与出射光线ωo拥有相同（被反射）角度的光线会得到1.0这个返回值。

几乎所有实时渲染管线使用的都是一种被称为Cook-Torrance BRDF模型

<img src="http://latex.codecogs.com/svg.latex?f_r = k_d f_{lambert} +  k_s f_{cook-torrance}" />

这里的kd是早先提到过的入射光线中被折射部分的能量所占的比率，而ks是被反射部分的比率。BRDF的左侧表示的是漫反射部分，这里用flambert来表示。它被称为Lambertian漫反射，这和我们之前在漫反射着色中使用的常数因子类似，用如下的公式来表示：

<img src="http://latex.codecogs.com/svg.latex?f_{lambert} = \frac{c}{\pi}" />

c表示表面颜色（回想一下漫反射表面纹理）。除以π是为了对漫反射光进行标准化，因为前面含有BRDF的积分方程是受π影响的

镜面反射包含三个函数，此外分母部分还有一个标准化因子 。字母D，F与G分别代表着一种类型的函数，各个函数分别用来近似的计算出表面反射特性的一个特定部分。三个函数分别为正态分布函数(Normal Distribution Function)，菲涅尔方程(Fresnel Rquation)和几何函数(Geometry Function)：(后面的是虚幻4引擎中使用的方法）

<img src="http://latex.codecogs.com/svg.latex?f_{cook-torrance} = \frac{DFG}{4(\omega_o \cdot n)(\omega_i \cdot n)}" />

+ 正态分布函数D：估算在受到表面粗糙度的影响下，取向方向与中间向量一致的微平面的数量。这是用来估算微平面的主要函数。`Trowbridge-Reitz GGX`
+ 几何函数G：描述了微平面自成阴影的属性。当一个平面相对比较粗糙的时候，平面表面上的微平面有可能挡住其他的微平面从而减少表面所反射的光线。`Smith’s Schlick-GGX`
+ 菲涅尔方程F：菲涅尔方程描述的是在不同的表面角下表面所反射的光线所占的比率。`Fresnel-Schlick近似`

<h4>Trowbridge-Reitz GGX近似</h4>

<img src="http://latex.codecogs.com/svg.latex?NDF_{GGX TR}(n, h, \alpha) = \frac{\alpha^2}{\pi((n \cdot h)^2 (\alpha^2 - 1) + 1)^2}" />

h表示用来与平面上微平面做比较用的中间向量，而a表示表面粗糙度。粗糙度很低（也就是说表面很光滑）的时候，与中间向量取向一致的微平面会高度集中在一个很小的半径范围内。由于这种集中性，NDF最终会生成一个非常明亮的斑点。但是当表面比较粗糙的时候，微平面的取向方向会更加的随机。视觉效果：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/ndf.png)

```glsl
float D_GGX_TR(vec3 N, vec3 H, float a)
{
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;

    return nom / denom;
}
```

<h4>Smith’s Schlick-GGX近似</h4>

从统计学上近似的求得了微平面间相互遮蔽的比率，这种相互遮蔽会损耗光线的能量。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/geometry_shadowing.png)

<img src="http://latex.codecogs.com/svg.latex?G_{SchlickGGX}(n,v,k) = \frac{n \cdot v}{(n \cdot v)(1-k)+k}" />

k是α基于几何函数是针对直接光照或是针对IBL光照的重映射(Remapping) :

<img src="http://latex.codecogs.com/svg.latex?k_{direct} = \frac{(\alpha + 1)^2}{8}" />

<img src="http://latex.codecogs.com/svg.latex?k_{IBL} = \frac{\alpha^2}{2}" />

为了有效的估算几何部分，需要将观察方向（几何遮蔽(Geometry Obstruction)）和光线方向向量（几何阴影(Geometry Shadowing)）都考虑进去。我们可以使用史密斯法(Smith’s method)来把两者都纳入其中：

<img src="http://latex.codecogs.com/svg.latex?G(n, v, l, k) = G_{sub}(n, v, k) G_{sub}(n, l, k)" />

使用史密斯法与Schlick-GGX作为Gsub可以得到如下所示不同粗糙度的视觉效果：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/geometry.png)

```glsl
float GeometrySchlickGGX(float NdotV, float k)
{
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);

    return ggx1 * ggx2;
}
```

<h4>Fresnel-Schlick近似</h4>

描述的是被反射的光线对比光线被折射的部分所占的比率，这个比率会随着我们观察的角度不同而不同。

垂直观察的时候，任何物体或者材质表面都有一个基础反射率(Base Reflectivity)，但是如果以一定的角度往平面上看的时候所有反光都会变得明显起来。

用Fresnel-Schlick近似法求得近似解：

<img src="http://latex.codecogs.com/svg.latex?F_{Schlick}(h, v, F_0) = F_0 + (1 - F_0) ( 1 - (h \cdot v))^5" />

F0表示平面的基础反射率，它是利用所谓折射指数(Indices of Refraction)或者说IOR计算得出的。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/fresnel.png)

上图中Fresnel现象在观察角与表面法线呈90°时尤其明显，反光越强

菲涅尔方程还存在一些细微的问题。其中**一个问题是Fresnel-Schlick近似仅仅对电介质或者说非金属表面有定义**。对于导体(Conductor)表面（金属），使用它们的折射指数计算基础折射率并不能得出正确的结果，这样我们就需要使用一种不同的菲涅尔方程来对导体表面进行计算。由于这样很不方便，**所以我们预先计算出平面对于法向入射（F0）的反应（处于0度角，好像直接看向表面一样）然后基于相应观察角的Fresnel-Schlick近似对这个值进行插值，用这种方法来进行进一步的估算。这样我们就能对金属和非金属材质使用同一个公式了。**

[这里](https://refractiveindex.info/)可以查到很多材质的法向入射等的数值

导体材质表面的基础反射率起点比电介质高一些并且大多在0.5和1.0之间变化，对于导体或者金属表面而言基础反射率一般是带有色彩的F0要用RGB三色表示，这些独有的特性引出了所谓的金属工作流的概念，需要额外使用一个被称为金属度(Metalness)的参数来参与编写表面材质

通过预先计算电介质与导体的F0值，我们可以对两种类型的表面使用相同的Fresnel-Schlick近似，但是如果是金属表面的话就需要对基础反射率添加色彩。

```glsl
vec3 F0 = vec3(0.04);//取最常见的电解质表面的平均值，对于大多数电介质表面已经足够好了
F0      = mix(F0, surfaceColor.rgb, metalness);//通过金属度的参数调整
```

```glsl
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);//cosTheta是表面法向量n与观察方向v的点乘的结果
}
```

最后我们得到这样的BRDF：

<img src="http://latex.codecogs.com/svg.latex?L_o(p,\omega_o) = \int\limits_{\Omega} (k_d\frac{c}{\pi} + k_s\frac{DFG}{4(\omega_o \cdot n)(\omega_i \cdot n)}) L_i(p,\omega_i) n \cdot \omega_i  d\omega_i" />

PBR常见的输出，通过Substance Designer：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/6PBR/Theory/Reference/textures.png)