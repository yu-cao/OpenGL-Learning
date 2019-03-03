<h2>阴影映射</h2>

阴影是光线被阻挡的结果；当一个光源的光线由于其他物体的阻挡不能够达到一个物体的表面的时候，那么这个物体就在阴影中了。阴影能够使场景看起来真实得多，并且可以让观察者获得物体之间的空间位置关系。场景和物体的深度感因此能够得到极大提升

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_with_without.png)

有阴影的时候你能更容易地区分出物体之间的位置关系，例如，当使用阴影的时候浮在地板上的立方体的情况更加明晰。

阴影是比较不好实现的，因为当前实时渲染领域还没找到一种完美的阴影算法。目前有几种近似阴影技术，但它们都有自己的弱点和不足。

视频游戏中较多使用的一种技术是阴影贴图（shadow mapping），效果不错，而且相对容易实现。

<hr>

<h3>阴影映射</h3>

我们以光的位置为视角进行渲染，我们能看到的东西都将被点亮，看不见的一定是在阴影之中了。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_theory.png)

所有蓝线代表光源可以看到的fragment。黑线代表被遮挡的fragment：它们应该渲染为带阴影的。

我们希望得到射线第一次击中的那个物体，然后用这个最近点和射线上其他点进行对比。然后我们将测试一下看看射线上的其他点是否比最近点更远，如果是的话，这个点就在阴影中。对从光源发出的射线上的成千上万个点进行遍历是个极端消耗性能的举措，实时渲染上基本不可取。（离线渲染中光线追踪的策略）

我们可以有替代方案：深度缓冲。在深度缓冲里的一个值是摄像机视角下，对应于一个片元的一个0到1之间的深度值。如果我们从光源的透视图来渲染场景，并把深度值的结果储存到纹理中会怎样？通过这种方式，我们就能对光源的透视图所见的最近的深度值进行采样。最终，深度值就会显示从光源的透视图下见到的第一个片元了。我们管储存在纹理中的所有这些深度值，叫做深度贴图（depth map）或阴影贴图。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_theory_spaces.png)

左侧的图片展示了一个定向光源（所有光线都是平行的）在立方体下的表面投射的阴影。右边的图中我们显示出同样的平行光和观察者。我们渲染一个点P处的片元，需要决定它是否在阴影中。我们先得使用T把P变换到光源的坐标空间里。既然点P是从光的透视图中看到的，它的z坐标就对应于它的深度，例子中这个值是0.9。使用点P在光源的坐标空间的坐标，我们可以索引深度贴图，来获得从光的视角中最近的可见深度，结果是点C，最近的深度是0.4。因为索引深度贴图的结果是一个小于点P的深度，我们可以断定P被挡住了，它在阴影中了。

**深度映射由两个步骤组成：首先，我们渲染深度贴图，然后我们像往常一样渲染场景，使用生成的深度贴图来计算片元是否在阴影之中。**

<hr>

<h2>深度贴图</h2>

第一步我们需要生成一张深度贴图(Depth Map)。深度贴图是从光的透视图里渲染的深度纹理，用它计算阴影。因为我们需要将场景的渲染结果储存到一个纹理中，这里，我们将再次需要帧缓冲。

创建一个帧缓冲对象，然后创建一个2D纹理提供给帧缓冲的深度缓冲使用

```cpp
unsigned int depthMapFBO;
glGenFramebuffers(1, &depthMapFBO);


const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;//深度贴图的解析度

unsigned int depthMap;
glGenTextures(1, &depthMap);
glBindTexture(GL_TEXTURE_2D, depthMap);
//这里把纹理格式设定为GL_DEPTH_COMPONENT
glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  
```

接下来，我们把生成的深度纹理作为帧缓冲的深度缓冲：

```cpp
glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
//需要的只是在从光的透视图下渲染场景的时候深度信息，所以颜色缓冲没有用
glDrawBuffer(GL_NONE);//显式告诉OpenGL我们不适用任何颜色数据进行渲染
glReadBuffer(GL_NONE);
glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

合理配置将深度值渲染到纹理的帧缓冲后，我们就可以开始第一步了：生成深度贴图。

整体流程伪代码如下（一定要记得调用`glViewport`。因为阴影贴图经常和我们原来渲染的场景（通常是窗口解析度）有着不同的解析度，我们需要改变视口（viewport）的参数以适应阴影贴图的尺寸）：

```cpp
// 1. 首选渲染深度贴图
glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    ConfigureShaderAndMatrices();//下一段详述
    RenderScene();
glBindFramebuffer(GL_FRAMEBUFFER, 0);

// 2. 像往常一样渲染场景，但这次使用深度贴图
glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
ConfigureShaderAndMatrices();
glBindTexture(GL_TEXTURE_2D, depthMap);
RenderScene();
```

<hr>

<h3>光源空间的变换</h3>

`ConfigureShaderAndMatrices`：用来在第二个步骤确保为每个物体设置了合适的投影和视图矩阵，以及相关的模型矩阵。

第一个步骤中，我们从光的位置的视野下使用了不同的投影和视图矩阵来渲染的场景。

我们使用的是一个所有光线都平行的定向光。出于这个原因，我们将为光源使用正交投影矩阵，透视图将没有任何变形：(因为投影矩阵间接决定可视区域的范围，以及哪些东西不会被裁切，你需要保证投影视锥（frustum）的大小，以包含打算在深度贴图中包含的物体)

```cpp
GLfloat near_plane = 1.0f, far_plane = 7.5f;
glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
```

为了创建一个视图矩阵来变换每个物体，把它们变换到从光源视角可见的空间中，我们将使用glm::lookAt函数；这次从光源的位置看向场景中央：

```cpp
glm::mat4 lightView = glm::lookAt(glm::vec(-2.0f, 4.0f, -1.0f), glm::vec3(0.0f), glm::vec3(1.0));
```

二者相结合为我们提供了一个光空间的变换矩阵，它将每个世界空间坐标变换到光源处所见到的那个空间；这正是我们渲染深度贴图所需要的：

```cpp
glm::mat4 lightSpaceMatrix = lightProjection * lightView;
```

这个`lightSpaceMatrix`正是前面图上称为T的那个变换矩阵。

<hr>

<h3>渲染至深度贴图</h3>

以光的透视图进行场景渲染的时候，我们会用一个比较简单的着色器，这个着色器除了把顶点变换到光空间以外，不会做得更多了：

```glsl
#version 330 core
layout (location = 0) in vec3 position;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(position, 1.0f);
}
```

由于我们没有颜色缓冲，最后的片元不需要任何处理，所以我们可以简单地使用一个空片元着色器，运行完后，深度缓冲会被更新：

```glsl
#version 330 core

void main()
{             
    // gl_FragDepth = gl_FragCoord.z;
}
```

渲染深度缓冲现在成了：

```cpp
simpleDepthShader.Use();
glUniformMatrix4fv(lightSpaceMatrixLocation, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    RenderScene(simpleDepthShader);
glBindFramebuffer(GL_FRAMEBUFFER, 0);
```

这里的RenderScene函数的参数是一个着色器程序（shader program），它调用所有相关的绘制函数，并在需要的地方设置相应的模型矩阵。

将深度贴图渲染到四边形上的像素着色器：

```glsl
#version 330 core
out vec4 color;
in vec2 TexCoords;

uniform sampler2D depthMap;

void main()
{             
    float depthValue = texture(depthMap, TexCoords).r;
    color = vec4(vec3(depthValue), 1.0);
}
```

注意：当用透视投影矩阵取代正交投影矩阵来显示深度时，有一些轻微的改动，因为使用透视投影时，深度是非线性的。

然后渲染出深度贴图

```cpp
//渲染出深度贴图
glViewport(0,0,SHADOW_WIDTH,SHADOW_HEIGHT);
glBindFramebuffer(GL_FRAMEBUFFER,depthMapFBO);
glClear(GL_DEPTH_BUFFER_BIT);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D,woodTexture);
renderScene(simpleDepthShader);
glBindFramebuffer(GL_FRAMEBUFFER,0);
	
glViewport(0,0,SCR_WIDTH,SCR_HEIGHT);
glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
```

<hr>

<h3>渲染阴影</h3>

建立一个着色器，使用上面建立好的深度贴图进行渲染即可（具体见Project文件中的`shader/shadow_mapping.vert和.frag`和`shader/shadow_mapping_depth.vert和.frag`

```cpp
//先要记得载入深度贴图
shader.use();
shader.setInt("diffuseTexture", 0);
shader.setInt("shadowMap", 1);//把刚刚绑定好的shadowMap绑定进来

//循环中
//把场景渲染出来
shader.use();
glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
glm::mat4 view = camera.GetViewMatrix();
shader.setMat4("projection", projection);
shader.setMat4("view", view);
// set light uniforms
shader.setVec3("viewPos", camera.Position);
shader.setVec3("lightPos", lightPos);
shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, woodTexture);
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, depthMap);
renderScene(shader);
```

```glsl
//vertex shader
//执行光源空间变换
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

out VS_OUT {
    vec3 FragPos;//普通经过变换到世界坐标系下的顶点
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;//我们用同一个lightSpaceMatrix，把世界空间顶点位置转换为光源空间
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = transpose(inverse(mat3(model))) * aNormal;
    vs_out.TexCoords = aTexCoords;
    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}


//fragment shader
#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    //进行透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    //上面的projCoords的xyz分量都是[-1,1]（下面会指出这对于远平面之类的点才成立）
    //而为了和深度贴图的深度相比较，z分量需要变换到[0,1]
    //为了作为从深度贴图中采样的坐标，xy分量也需要变换到[0,1]；所以整个projCoords向量都需要变换到[0,1]范围
    projCoords = projCoords * 0.5 + 0.5;

    //得到光的位置视野下最近的深度
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // 得到片元的当前深度，我们简单获取投影向量的z坐标，它等于来自光源视角的片元的深度
    float currentDepth = projCoords.z;

    //简单检查currentDepth是否高于closetDepth，如果是，那么片元就在阴影中。
    float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

    return shadow;
}

//使用Blinn-Phong光照模型渲染场景
void main()
{
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.3);
    // ambient
    vec3 ambient = 0.3 * color;
    // diffuse
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;
    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace);//计算出一个shadow值，当fragment在阴影中时是1.0，在阴影外是0.0
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

    FragColor = vec4(lighting, 1.0);
}
```

<hr>

<h2>阴影贴图改进</h2>

<h3>阴影失真</h3>

线条样式极其明显

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_acne.png)

原因如下：

因为阴影贴图受限于解析度，在距离光源比较远的情况下，多个片元可能从深度贴图的同一个值中去采样。图片每个斜坡代表深度贴图一个单独的纹理像素（也就是光源视角下的一个像素）。你可以看到，多个片元从同一个深度值进行采样。（黑色横线表示出现阴影，因为光照的深度判定的深度会小于当前视角下的深度值）

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_acne_diagram.png)

虽然很多时候没问题，但是当光源以一个角度朝向表面的时候就会出问题，这种情况下深度贴图也是从一个角度下进行渲染的。多个片元就会从同一个斜坡的深度纹理像素中采样，有些在地板上面，有些在地板下面；这样我们所得到的阴影就有了差异。因为这个，有些片元被认为是在阴影之中，有些不在，由此产生了图片中的条纹样式。

可以使用阴影偏移的方法进行修复：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_acne_bias.png)

这部分都是微调至可接受即可

```glsl
float bias = 0.005;
float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

//或者使用点乘
float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
```

<h3>悬浮</h3>

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_peter_panning.png)

我们可以使用一个叫技巧解决大部分的Peter panning问题：当渲染深度贴图时候使用正面剔除（front face culling），OpenGL默认是背面剔除。我们要告诉OpenGL我们要剔除正面。

因为我们只需要深度贴图的深度值，对于实体物体无论我们用它们的正面还是背面都没问题。使用背面深度不会有错误，因为阴影在物体内部有错误我们也看不见。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_culling.png)

为了修复peter游移，我们要进行正面剔除，先必须开启`glEnable(GL_CULL_FACE);`：（但是这里有一个问题，地板没有在深度贴图中渲染出来，导致地板被剔除）

```cpp
glCullFace(GL_FRONT);
RenderSceneToDepthMap();
glCullFace(GL_BACK); // 不要忘记设回原先的culling face
```

<h3>采样过多</h3>

光的视锥不可见的区域一律被认为是处于阴影中，不管它真的处于阴影之中。出现这个状况是因为超出光的视锥的投影坐标比1.0大，这样采样的深度纹理就会超出他默认的0到1的范围。根据纹理环绕方式，我们将会得到不正确的深度结果，它不是基于真实的来自光源的深度值。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_outside_frustum.png)

**错误点：光照有一个区域，超出该区域就成为了阴影；这个区域实际上代表着深度贴图的大小，这个贴图投影到了地板上。**发生这种情况的原因是我们之前将深度贴图的环绕方式设置成了`GL_REPEAT`。

我们宁可让所有超出深度贴图的坐标的深度范围是1.0，这样超出的坐标将永远不在阴影之中。我们可以储存一个边框颜色，然后把深度贴图的纹理环绕选项设置为`GL_CLAMP_TO_BORDER`：现在如果我们采样深度贴图0到1坐标范围以外的区域，纹理函数总会返回一个1.0的深度值，阴影值为0.0。结果看起来会更真实：

```cpp
//原来
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

//现在
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
```

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_clamp_edge.png)

仍有一部分是黑暗区域。那里的坐标超出了光的正交视锥的远平面。你可以看到这片黑色区域总是出现在光源视锥的极远处。当一个点比光的远平面还要远时，它的投影坐标的z坐标大于1.0。这种情况下，GL_CLAMP_TO_BORDER环绕方式不起作用，因为我们把坐标的z元素和深度贴图的值进行了对比；它总是为大于1.0的z返回true。

解决这个问题只要投影向量的z坐标大于1.0，我们就把shadow的值强制设为0.0：

```glsl
float ShadowCalculation(vec4 fragPosLightSpace)
{
    [...]
    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}
```

**只有在深度贴图范围以内的被投影的fragment坐标才有阴影，所以任何超出范围的都将会没有阴影。由于在游戏中通常这只发生在远处，就会比我们之前的那个明显的黑色区域效果更真实**：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_over_sampling_fixed.png)

<h3>PCF</h3>

放大看阴影，阴影映射对解析度的依赖很快变得很明显

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_zoom.png)

多个片元对应于一个纹理像素。结果就是多个片元会从深度贴图的同一个深度值进行采样，这几个片元便得到的是同一个阴影，这就会产生锯齿边。

你可以通过增加深度贴图解析度的方式来降低锯齿块，也可以尝试尽可能的让光的视锥接近场景。（可能不太现实）

另一个（并不完整的）解决方案叫做PCF（percentage-closer filtering），这是一种多个不同过滤方式的组合，它产生柔和阴影，使它们出现更少的锯齿块和硬边。**核心思想是从深度贴图中多次采样，每一次采样的纹理坐标都稍有不同。每个独立的样本可能在也可能不在阴影中。所有的次生结果接着结合在一起，进行平均化，我们就得到了柔和阴影。**

```glsl
//简单的PCF模糊柔和化阴影锯齿(取周围和本身共9个点进行模糊化)
shadow = 0.0;
vec2 texelSize = 1.0 / textureSize(shadowMap, 0);//返回一个给定采样器纹理的0级mipmap的vec2类型的宽和高。用1除以它返回一个单独纹理像素的大小
for(int x = -1; x <= 1; ++x)
{
    for(int y = -1; y <= 1; ++y)
    {
        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
    }    
}
shadow /= 9.0;
```

<hr>

<h3>正交 VS 投影</h3>

渲染深度贴图的时候，正交(Orthographic)和投影(Projection)矩阵之间有所不同。正交投影矩阵并不会将场景用透视图进行变形，所有视线/光线都是平行的，这使它对于定向光来说是个很好的投影矩阵。然而透视投影矩阵，会将所有顶点根据透视关系进行变形，结果因此而不同。下图展示了两种投影方式所产生的不同阴影区域：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/5.3.1Shadow%20Mapping/Reference/shadow_mapping_projection.png)

透视投影因此更经常用在点光源和聚光灯上，而正交投影经常用在定向光上。

另一个细微差别是，透视投影矩阵，将深度缓冲视觉化经常会得到一个几乎全白的结果。发生这个是因为透视投影下，深度变成了非线性的深度值，它的大多数可辨范围接近于近平面。

像使用正交投影一样合适的观察到深度值，你必须先讲过非线性深度值转变为线性的：

```glsl
#version 330 core
out vec4 color;
in vec2 TexCoords;

uniform sampler2D depthMap;
uniform float near_plane;
uniform float far_plane;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

void main()
{             
    float depthValue = texture(depthMap, TexCoords).r;
    color = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
    // color = vec4(vec3(depthValue), 1.0); // orthographic
}
```