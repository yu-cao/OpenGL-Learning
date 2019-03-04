<h2>视差贴图</h2>

它对根据储存在纹理中的几何信息对顶点进行位移或偏移

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.5Parallax%20Mapping/Reference/parallax_mapping_height_map.png)

平面上每个顶点都根据从高度贴图采样出来的高度值进行位移，根据材质的几何属性平坦的平面变换成凹凸不平的表面。例如一个平坦的平面利用上面的高度贴图进行置换能得到以下结果：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.5Parallax%20Mapping/Reference/parallax_mapping_plane_heightmap.png)

视差贴图背后的思想是修改纹理坐标使一个fragment的表面看起来比实际的更高或者更低，所有这些都根据观察方向和高度贴图。

粗糙的红线代表高度贴图中的数值的立体表达，向量V代表观察方向。如果平面进行实际位移，观察者会在点B看到表面。然而我们的平面没有实际上进行位移，观察方向将在点A与平面接触。视差贴图的目的是，在A位置上的fragment不再使用点A的纹理坐标而是使用点B的。随后我们用点B的纹理坐标采样，观察者就像看到了点B一样。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.5Parallax%20Mapping/Reference/parallax_mapping_scaled_height.png)

如何从点A得到点B的纹理坐标？视差贴图尝试通过对 从fragment到观察者的方向向量V¯ 进行缩放的方式解决这个问题，缩放的大小是 A处fragment的高度H(A)。所以我们将V¯的长度缩放为高度贴图在点A处H(A)采样得来的值得到向量P

随后选出P¯以及这个向量与平面对齐的坐标作为纹理坐标的偏移量，向量P¯是使用从高度贴图得到的高度值计算出来的，所以一个fragment的高度越高位移的量越大。

但是当表面的高度变化很快的时候，看起来就不会真实，因为向量P¯最终不会和B接近，就像下图这样，取到的纹理点是H(P)，与交点有明显误差：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.5Parallax%20Mapping/Reference/parallax_mapping_incorrect_p.png)

另一个问题是，当表面被任意旋转以后很难指出从P¯获取哪一个坐标。

修正的做法：视差贴图

<hr>

通常视差贴图和法线贴图连用。因为视差贴图生成表面位移了的幻觉，当光照不匹配时这种幻觉就被破坏了。法线贴图通常根据高度贴图生成，法线贴图和高度贴图一起用能保证光照能和位移相匹配。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.5Parallax%20Mapping/Reference/parallax_mapping_depth.png)

再次获得A和B，但是这次我们用向量V¯减去点A的纹理坐标得到P¯。我们通过在着色器中用1.0减去采样得到的高度贴图中的值来取得深度值，而不再是高度值，或者简单地在图片编辑软件中把这个纹理进行反色操作，就像我们对连接中的那个深度贴图所做的一样。

我们使用前一个法线贴图的顶点着色器，片元着色器如下：

```cpp
#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D depthMap;

uniform float heightScale;

//返回经过位移后的纹理坐标
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    float height = texture(depthMap, texCoords).r;//用本来的纹理坐标texCoords从高度贴图中来采样出当前fragment高度H(A)  
    vec2 p = viewDir.xy / viewDir.z * (height * height_scale);//计算出P，x和y元素在切线空间中，viewDir向量除以它的z元素，用fragment的高度对它进行缩放。
    return texCoords - p;//用P¯减去纹理坐标来获得最终的经过位移纹理坐标
}

void main()
{
    //核心改动---------
    //切线空间中fragment指向观察者的viewDir和切线空间中的纹理
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec2 texCoords = fs_in.TexCoords;
    
    //在平面的边缘上，纹理坐标超出了0到1的范围进行采样，根据纹理的环绕方式导致了不真实的结果。解决的方法是当它超出默认纹理坐标范围进行采样的时候就丢弃
    texCoords = ParallaxMapping(fs_in.TexCoords,  viewDir);       
    if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;

    //从法线贴图中得到法线
    vec3 normal = texture(normalMap, texCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);   
    //-----------------
   
      
    // get diffuse color
    vec3 color = texture(diffuseMap, texCoords).rgb;
    // ambient
    vec3 ambient = 0.1 * color;
    // diffuse
    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular    
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
```

有一个地方需要注意，就是viewDir.xy除以viewDir.z那里。因为viewDir向量是经过了标准化的，viewDir.z会在0.0到1.0之间的某处。当viewDir大致平行于表面时，它的z元素接近于0.0，除法会返回比viewDir垂直于表面的时候更大的P¯向量。所以基本上我们增加了P¯的大小，**当以一个角度朝向一个表面相比朝向顶部时它对纹理坐标会进行更大程度的缩放；这会在角上获得更大的真实度**。

<h3>陡峭视差映射</h3>

不是使用一个样本而是多个样本来确定向量P¯到B。它将总深度范围分布到同一个深度/高度的多个层中。从每个层中我们沿着P¯方向移动采样纹理坐标，直到我们找到了一个采样得到的低于当前层的深度值的深度值。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.5Parallax%20Mapping/Reference/parallax_mapping_steep_parallax_mapping_diagram.png)

从上到下遍历深度层，我们把每个深度层和储存在深度贴图中的它的深度值进行对比。**如果这个层的深度值小于深度贴图的值，就意味着这一层的P¯向量部分在表面之下。我们继续这个处理过程直到有一层的深度高于储存在深度贴图中的值：这个点就在（经过位移的）表面下方。**

这个例子中我们可以看到第二层(D(2) = 0.73)的深度贴图的值仍低于第二层的深度值0.4，所以我们继续。下一次迭代，这一层的深度值0.6大于深度贴图中采样的深度值(D(3) = 0.37)。我们便可以假设第三层向量P¯是可用的位移几何位置。我们可以用从向量P3¯的纹理坐标偏移T3来对fragment的纹理坐标进行位移。你可以看到随着深度曾的增加精确度也在提高。

只需要修改片元着色器的`ParallaxMapping`就可以了

```glsl
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    //深度层数目
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    
    //计算每一层的size
    float layerDepth = 1.0 / numLayers;
    //当前层的深度
    float currentLayerDepth = 0.0;
    //每层移动纹理坐标的量（来自向量P）
    vec2 P = viewDir.xy / viewDir.z * heightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    //初始化值
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(depthMap, currentTexCoords).r;
    
    //遍历所有层，从上开始，直到找到小于这一层的深度值的深度贴图值
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(depthMap, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    return currentTexCoords;
}
```

<h3>视差遮蔽映射</h3>

视差遮蔽映射(Parallax Occlusion Mapping)和陡峭视差映射的原则相同，但不是用触碰的第一个深度层的纹理坐标，而是在触碰之前和之后，在深度层之间进行线性插值。我们根据表面的高度距离啷个深度层的深度层值的距离来确定线性插值的大小。

```glsl
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(depthMap, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(depthMap, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    //在对（位移的）表面几何进行交叉，找到深度层之后，我们获取交叉前的纹理坐标
    //然后我们计算来自相应深度层的几何之间的深度之间的距离，并在两个值之间进行插值
    //线性插值的方式是在两个层的纹理坐标之间进行的基础插值
    //函数最后返回最终的经过插值的纹理坐标
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(depthMap, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}
```