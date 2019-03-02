<h2>高级光照</h2>

Blinn-Phong模型

冯氏光照不仅对真实光照有很好的近似，而且性能也很高。但是它的镜面反射会在一些情况下出现问题，特别是物体反光度很低时，会导致大片（粗糙的）高光区域。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.1Advanced%20Lighting/Reference/advanced_lighting_phong_limit.png)

出现这个问题的原因是观察向量和反射向量间的夹角不能大于90度。如果点积的结果为负数，镜面光分量会变为0.0。你可能会觉得，当光线与视线夹角大于90度时你应该不会接收到任何光才对，所以这不是什么问题。

```glsl
void main()
{
    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm,lightDir),0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);//这里当视角跟光源位置接近时且比较靠近平面时，直接丢失了镜面特性
    vec3 specular = specularStrength * spec * lightColor;
    // Blinn-Phong方法进行的修正：
    // vec3 viewDir = normalize(viewPos - FragPos);
    // vec3 H = normalize(lightDir + viewDir);
    // float spec = pow(max(dot(norm, H), 0.0), alpha);

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
```

但是，在考虑镜面高光时，我们测量的角度并不是光源与法线的夹角，而是视线与反射光线向量的夹角

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.1Advanced%20Lighting/Reference/advanced_lighting_over_90.png)

左图中是我们熟悉的冯氏光照中的反射向量；右图中，视线与反射方向之间的夹角明显大于90度，镜面光分量会变为0.0。**然而，当物体的镜面光分量非常小时，它产生的镜面高光半径足以让这些相反方向的光线对亮度产生足够大的影响。在这种情况下就不能忽略它们对镜面光分量的贡献了**

Blinn-Phong模型不再依赖于反射向量，而是采用了所谓的半程向量(Halfway Vector)，即光线与视线夹角一半方向上的一个单位向量。当半程向量与法线向量越接近时，镜面光分量就越大。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.1Advanced%20Lighting/Reference/advanced_lighting_halfway_vector.png)

现在，不论观察者向哪个方向看，半程向量与表面法线之间的夹角都不会超过90度（除非光源在表面以下）。它产生的效果会与冯氏光照有些许不同，但是大部分情况下看起来会更自然一点，特别是低高光的区域。

计算半程向量很简单，将光线的方向向量和观察向量加到一起，并将结果正规化(Normalize)就可以了

<img src="http://latex.codecogs.com/svg.latex?\overline{H}=\frac {\overline{L}+\overline{V}}{\|\overline {L}+\overline{V}\|}" />

```glsl
vec3 lightDir   = normalize(lightPos - FragPos);
vec3 viewDir    = normalize(viewPos - FragPos);
vec3 halfwayDir = normalize(lightDir + viewDir);
```

镜面光分量的实际计算只不过是对表面法线和半程向量进行一次约束点乘(Clamped Dot Product)，让点乘结果不为负，从而获取它们之间夹角的余弦值，之后我们对这个值取反光度次方：

```glsl
float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
vec3 specular = lightColor * spec;
```

如果你想获得和冯氏着色类似的效果，就必须在使用Blinn-Phong模型时将镜面反光度设置更高一点。通常我们会选择冯氏着色时反光度分量的2到4倍。Blinn-Phong的镜面光分量会比冯氏模型更锐利一些。为了得到与冯氏模型类似的结果，你可能会需要不断进行一些微调，但Blinn-Phong模型通常会产出更真实的结果。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.1Advanced%20Lighting/Reference/advanced_lighting_comparrison.png)

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.1Advanced%20Lighting/Reference/advanced_lighting_comparrison2.png)