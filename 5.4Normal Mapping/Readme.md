<h2>法线贴图</h2>

现实中的物体表面并非是平坦的，而是表现出无数（凹凸不平的）细节。而普通纹理不能体现这个效果

如果我们以光的视角来看这个问题：是什么使表面被视为完全平坦的表面来照亮？答案会是表面的法线向量。以光照算法的视角考虑的话，只有一件事决定物体的形状，这就是垂直于它的法线向量。砖块表面只有一个法线向量，表面完全根据这个法线向量被以一致的方式照亮。如果每个fragment都是用自己的不同的法线会怎样？这样我们就可以根据表面细微的细节对法线向量进行改变；这样就会获得一种表面看起来要复杂得多的幻觉：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.4Normal%20Mapping/Reference/normal_mapping_surfaces.png)

这种每个fragment使用各自的法线，替代一个面上所有fragment使用同一个法线的技术叫做法线贴图（normal mapping）细节获得了极大提升，开销却不大

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.4Normal%20Mapping/Reference/normal_mapping_compare.png)

为使法线贴图工作，我们需要为每个fragment提供一个法线。像diffuse贴图和specular贴图一样，我们可以使用一个2D纹理来储存法线数据。

法线向量的范围在-1到1之间，所以我们先要将其映射到0到1的范围使得其能存入纹理中（纹理一般存储颜色，值域为[0,1]）：

```glsl
vec3 rgb_normal = normal * 0.5 + 0.5; // 从 [-1,1] 转换至 [0,1]
```

是一种偏蓝色调的纹理（网上找到的几乎所有法线贴图都是这样的）。这是因为所有法线的指向都偏向z轴（0, 0, 1）这是一种偏蓝的颜色。法线向量从z轴方向也向其他方向轻微偏移，颜色也就发生了轻微变化，这样看起来便有了一种深度。

加载纹理，把它们绑定到合适的纹理单元，然后使用下面的改变了的像素着色器来渲染一个平面：

```glsl
uniform sampler2D normalMap;  

void main()
{           
    // 从法线贴图范围[0,1]获取法线
    normal = texture(normalMap, fs_in.TexCoords).rgb;
    // 将法线向量重新转换回范围[-1,1]
    normal = normalize(normal * 2.0 - 1.0);   

    [...]
    // 像往常那样处理光照
}
```

我们使用的法线贴图里面的所有法线向量都是指向正z方向的。上面的例子能用，是因为那个平面的表面法线也是指向正z方向的。可是，如果我们在表面法线指向正y方向的平面上使用同一个法线贴图，光照就会出现严重错误

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.4Normal%20Mapping/Reference/normal_mapping_ground_normals.png)

所有法线都指向z方向，它们本该朝着表面法线指向y方向的。

<h3>如何解决？-> 使用切线空间</h3>

法线贴图中的法线向量在**切线空间中，法线永远指着正z方向。切线空间是位于三角形表面之上的空间：法线相对于单个三角形的本地参考框架。**它就像法线贴图向量的本地空间；它们都被定义为指向正z方向，无论最终变换到什么方向。使用一个特定的矩阵我们就能将本地/切线空寂中的法线向量转成世界或视图坐标，使它们转向到最终的贴图表面的方向。

解决问题的方式是计算出一种矩阵，把法线从切线空间变换到一个不同的空间，这样它们就能和表面法线方向对齐了：法线向量都会指向正y方向。切线空间的一大好处是我们可以为任何类型的表面计算出一个这样的矩阵，由此我们可以把切线空间的z方向和表面的法线方向对齐。

这种矩阵叫做TBN矩阵，这三个字母分别代表tangent(切线T)、bitangent(副切线B)和normal向量。这是建构这个矩阵所需的向量。要建构这样一个把切线空间转变为不同空间的变异矩阵，我们需要三个相互垂直的向量，它们沿一个表面的法线贴图对齐于：上(N)、右(T)、前(B)；

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.4Normal%20Mapping/Reference/normal_mapping_tbn_vectors.png)

法线贴图的切线和副切线与纹理坐标的两个方向对齐。我们就是用到这个特性计算每个表面的切线和副切线的。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.4Normal%20Mapping/Reference/normal_mapping_surface_edges.png)

可以得到（以下数学公式如果渲染错误请Clone下来本地查看，在MacDown中测试正常）

<img src="http://latex.codecogs.com/svg.latex?\begin{aligned} E_1 = \Delta U_1T + \Delta V_1B \\ E_2 = \Delta U_2T + \Delta V_2B \end{aligned}" />

进而得到

<img src="http://latex.codecogs.com/svg.latex?\begin{bmatrix} E_{1x} & E_{1y} & E_{1z} \\ E_{2x} & E_{2y} & E_{2z} \end{bmatrix} = \begin{bmatrix} \Delta U_1 & \Delta V_1 \\ \Delta U_2 & \Delta V_2 \end{bmatrix} \begin{bmatrix} T_x & T_y & T_z \\ B_x & B_y & B_z \end{bmatrix}" />

左乘UV的逆得到

<img src="http://latex.codecogs.com/svg.latex?\begin{bmatrix} \Delta U_1 & \Delta V_1 \\ \Delta U_2 & \Delta V_2 \end{bmatrix}^{-1} \begin{bmatrix} E_{1x} & E_{1y} & E_{1z} \\ E_{2x} & E_{2y} & E_{2z} \end{bmatrix} = \begin{bmatrix} T_x & T_y & T_z \\ B_x & B_y & B_z \end{bmatrix}" />

写成伴随矩阵的形式：

<img src="http://latex.codecogs.com/svg.latex?\begin{bmatrix} T_x & T_y & T_z \\ B_x & B_y & B_z \end{bmatrix}  = \frac{1}{\Delta U_1 \Delta V_2 - \Delta U_2 \Delta V_1} \begin{bmatrix} \Delta V_2 & -\Delta V_1 \\ -\Delta U_2 & \Delta U_1 \end{bmatrix} \begin{bmatrix} E_{1x} & E_{1y} & E_{1z} \\ E_{2x} & E_{2y} & E_{2z} \end{bmatrix}" />

代码实现如下：

```cpp
// positions
glm::vec3 pos1(-1.0,  1.0, 0.0);
glm::vec3 pos2(-1.0, -1.0, 0.0);
glm::vec3 pos3(1.0, -1.0, 0.0);
glm::vec3 pos4(1.0, 1.0, 0.0);
// texture coordinates
glm::vec2 uv1(0.0, 1.0);
glm::vec2 uv2(0.0, 0.0);
glm::vec2 uv3(1.0, 0.0);
glm::vec2 uv4(1.0, 1.0);
// normal vector
glm::vec3 nm(0.0, 0.0, 1.0);

//计算第一个三角形边和deltaUV坐标
glm::vec3 edge1 = pos2 - pos1;
glm::vec3 edge2 = pos3 - pos1;
glm::vec2 deltaUV1 = uv2 - uv1;
glm::vec2 deltaUV2 = uv3 - uv1;

//应用上面的等式
float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
tangent1 = glm::normalize(tangent1);

bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
bitangent1 = glm::normalize(bitangent1);  

[...] // 对平面的第二个三角形采用类似步骤计算切线和副切线
```

因为一个**三角形永远是平坦的形状**，我们只需为每个三角形计算一个切线/副切线，它们对于每个三角形上的顶点都是一样的。要注意的是大多数实现通常三角形和三角形之间都会共享顶点。这种情况下开发者通常将每个顶点的法线和切线/副切线等顶点属性平均化，以获得更加柔和的效果。我们的平面的三角形之间分享了一些顶点，但是因为两个三角形相互并行，因此并不需要将结果平均化，但无论何时只要你遇到这种情况记住它就是件好事。

**最后的切线和副切线向量的值应该是(1, 0, 0)和(0, 1, 0)，它们和法线(0, 0, 1)组成相互垂直的TBN矩阵**。在平面上显示出来TBN应该是这样的：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.4Normal%20Mapping/Reference/normal_mapping_tbn_shown.png)

<hr>

<h3>切线空间法线贴图</h3>

```glsl
//思想：统一在切线坐标系下进行计算。优点：因为lightPos和viewPos不是每个fragment运行都要改变，对于fs_in.FragPos，我们也可以在顶点着色器计算它的切线空间位置。（性能更好）
//当然也可以统一在世界坐标系下，但是对于每个法线向量都要变换，因为对每个片元着色器都不一样
//顶点着色器
#version 330 core
//把计算出来的切线与副切线传入着色器
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;//可以不需要

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));   
    vs_out.TexCoords = aTexCoords;
    
    //构建出TBN矩阵
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);//B直接叉乘得到副切线
    
    mat3 TBN = transpose(mat3(T, B, N));//得到了TBN的逆矩阵
    
    //把世界坐标->切线坐标
    vs_out.TangentLightPos = TBN * lightPos;
    vs_out.TangentViewPos  = TBN * viewPos;
    vs_out.TangentFragPos  = TBN * vs_out.FragPos;
        
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

//片元着色器
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

uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{           
    //从法线贴图中得到[0,1]的数据，转换到[-1.1]中
    vec3 normal = texture(normalMap, fs_in.TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);//这个法线是在切线空间中的，不再需要变换
    
    //但是剩下的坐标计算都要依靠TBN的逆矩阵转换到切线空间来计算（这个在顶点着色器里面已经完成）
    // get diffuse color
    vec3 color = texture(diffuseMap, fs_in.TexCoords).rgb;
    // ambient
    vec3 ambient = 0.1 * color;
    // diffuse
    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
```

最后进行调用即可

```cpp
shader.use();
shader.setInt("diffuseMap", 0);
shader.setInt("normalMap", 1);

//渲染循环中
// configure view/projection matrices
glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
glm::mat4 view = camera.GetViewMatrix();
shader.use();
shader.setMat4("projection", projection);
shader.setMat4("view", view);

// render normal-mapped quad
glm::mat4 model = glm::mat4(1.0f);
model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))); // rotate the quad to show normal mapping from multiple directions
shader.setMat4("model", model);
shader.setVec3("viewPos", camera.Position);
shader.setVec3("lightPos", lightPos);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, diffuseMap);
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, normalMap);
renderQuad();
```

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.4Normal%20Mapping/Reference/normal_mapping_correct_tangent.png)

<hr>

<h3>复杂物体</h3>

我们可以使用Assimp加载器实现，加载模型的时候调用`aiProcess_CalcTangentSpace`。当`aiProcess_CalcTangentSpace`应用到Assimp的ReadFile函数时，Assimp会为每个加载的顶点计算出柔和的切线和副切线向量，它所使用的方法和上面类似。

```cpp
const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

//得到计算出来的切线空间
vector.x = mesh->mTangents[i].x;
vector.y = mesh->mTangents[i].y;
vector.z = mesh->mTangents[i].z;
vertex.Tangent = vector;
```

然后，还必须更新模型加载器，用以从带纹理模型中加载法线贴图。wavefront的模型格式（.obj）导出的法线贴图有点不一样，Assimp的`aiTextureType_NORMAL`并不会加载它的法线贴图，而`aiTextureType_HEIGHT`却能，所以我们经常这样加载它们：

```cpp
vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
```

对于每个模型的类型和文件格式来说都是不同的。`aiProcess_CalcTangentSpace`并不能总是很好的工作

使用法线贴图也是一种提升场景的表现的重要方式。在使用法线贴图之前不得不使用相当多的顶点才能表现出一个更精细的网格，但使用了法线贴图我们可以使用更少的顶点表现出同样丰富的细节。

<hr>

<h3>画质再提升</h3>

在更大的网格上计算切线向量的时候，它们往往有很大数量的共享顶点，当法线贴图应用到这些表面时将切线向量平均化通常能获得更好更平滑的结果。这样做有个问题，就是TBN向量可能会不能互相垂直，这意味着TBN矩阵不再是正交矩阵了。使用施密特正交化方法可以重正交化，顶点着色器中代码如下：

```glsl
vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
vec3 N = normalize(vec3(model * vec4(normal, 0.0)));
// re-orthogonalize T with respect to N
T = normalize(T - dot(T, N) * N);
// then retrieve perpendicular vector B with the cross product of T and N
vec3 B = cross(T, N);

mat3 TBN = mat3(T, B, N)
```
