<h2>立方体贴图</h2>

简单来说，立方体贴图就是一个包含了6个2D纹理的纹理，每个2D纹理都组成了立方体的一个面：一个有纹理的立方体。

为什么要把6张纹理合并到一张纹理中，而不是直接使用6个单独的纹理呢？立方体贴图有一个非常有用的特性，它可以通过一个方向向量来进行索引/采样。

方向向量的大小并不重要，只要提供了方向，OpenGL就会获取方向向量（最终）所击中的纹素，并返回对应的采样纹理值。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/4.6Cubemaps/Reference/cubemaps_sampling.png)

只要立方体（大小：1 X 1 X 1）的中心位于原点，采样立方体贴图所使用的方向向量将和立方体（插值的）顶点位置非常相像，我们就能使用立方体的实际位置向量来对立方体贴图进行采样了。将所有顶点的纹理坐标当做是立方体的顶点位置。最终得到的结果就是可以访问立方体贴图上正确面(Face)纹理的一个纹理坐标。

<hr>

<h3>构造正方体贴图与使用</h3>

和普通的纹理比较像，只不过这次要绑定到`GL_TEXTURE_CUBE_MAP`

```cpp
unsigned int textureID;
glGenTextures(1, &textureID);
glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
```

然后因为贴图纹理有6个（6个面），所以我们要调用6次`glTexImage2D`，OpenGL给我们提供了6个特殊的纹理目标，专门对应立方体贴图的一个面

|纹理目标|方位|
|:-:|:-:|
|`GL_TEXTURE_CUBE_MAP_POSITIVE_X`|右|
|`GL_TEXTURE_CUBE_MAP_NEGATIVE_X`|左|
|`GL_TEXTURE_CUBE_MAP_POSITIVE_Y`|上|
|`GL_TEXTURE_CUBE_MAP_NEGATIVE_Y`|下|
|`GL_TEXTURE_CUBE_MAP_POSITIVE_Z`|后|
|`GL_TEXTURE_CUBE_MAP_NEGATIVE_Z`|前|

实际上上面这6个值是枚举类型，所以其实只要有一个vector存储了，就可以从`GL_TEXTURE_CUBE_MAP_POSITIVE_X `开始进行遍历

```cpp
int width, height, nrChannels;
unsigned char *data;  
for(unsigned int i = 0; i < textures_faces.size(); i++)
{
    data = stbi_load(textures_faces[i].c_str(), &width, &height, &nrChannels, 0);
    glTexImage2D(
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
        0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
    );
}
```

立方体贴图和其它纹理没什么不同，我们也需要设定它的环绕和过滤方式（`GL_CLAMP_TO_EDGE`：OpenGL将在我们对两个面之间采样的时候，永远返回它们的边界值）：

```cpp
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);//纹理的第三个维度
```

在片元着色器中，我们使用`samplerCube`进行采样，而且存储纹理使用的是`vec3`的方向向量

```cpp
in vec3 textureDir;//代表3D纹理坐标的方向向量
uniform samplerCube cubemap;//立方体贴图的纹理采样器

void main()
{             
    FragColor = texture(cubemap, textureDir);
}
```

<hr>

<h3>天空盒技术</h3>

大部分情况下它们都是6张单独的纹理图像，让玩家产生其位于广袤宇宙中的错觉，但实际上他只是在一个小小的盒子当中。网上有很多这样的[资源](http://www.custommapmakers.org/skyboxes.php)

首先，我们需要将合适的纹理路径按照立方体贴图枚举指定的顺序加载到一个vector中，然后加载天空盒：

```cpp
vector<std::string> faces;
{
    "right.jpg",
    "left.jpg",
    "top.jpg",
    "bottom.jpg",
    "front.jpg",
    "back.jpg"
};

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int cubemapTexture = loadCubemap(faces);//加载为一个立方体贴图了，它的id是cubemapTexture，我们可以将它绑定到一个立方体中
```

显示天空盒

天空盒是绘制在一个立方体上的，和其它物体一样，我们需要另一个VAO、VBO以及新的一组顶点

贴图3D立方体的立方体贴图可以使用立方体的位置作为纹理坐标来采样。当立方体中心处于原点(0, 0, 0)时，它的每一个位置向量都是从原点出发的方向向量。这个方向向量正是获取立方体上特定位置的纹理值所需要的。正是因为这个，我们只需要提供位置向量而不用纹理坐标了

因此我们需要一组新的着色器：

```glsl
//顶点着色器
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}

//片元着色器
#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
}
```

我们只需要绑定立方体贴图纹理，skybox采样器就会自动填充上天空盒立方体贴图了。绘制天空盒时，我们需要将它变为场景中的第一个渲染的物体，并且禁用深度写入。这样子天空盒就会永远被绘制在其它物体的背后了。

优化：提前深度测试(Early Depth Testing)：最后渲染天空盒，以获得轻微的性能提升。这样子的话，深度缓冲就会填充满所有物体的深度值了，我们只需要在提前深度测试通过的地方渲染天空盒的片段就可以了，很大程度上减少了片段着色器的调用。

我们需要欺骗深度缓冲，让它认为天空盒有着最大的深度值1.0，只要它前面有一个物体，深度测试就会失败。

我们知道：透视除法是在顶点着色器运行之后执行的，将`gl_Position`的xyz坐标除以w分量。我们也知道，相除结果的z分量等于顶点的深度值。使用这些信息，我们可以将输出位置的z分量等于它的w分量，让z分量永远等于1.0，这样子的话，当透视除法执行之后，z分量会变为w / w = 1.0。

```glsl
void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
```

最终的标准化设备坐标将永远会有一个等于1.0的z值：最大的深度值。结果就是天空盒只会在没有可见物体的地方渲染了（只有这样才能通过深度测试，其它所有的东西都在天空盒前面）。

我们还要改变一下深度函数，将它从默认的`GL_LESS`改为`GL_LEQUAL`。深度缓冲将会填充上天空盒的1.0值，所以我们需要保证天空盒在值小于或等于深度缓冲而不是小于时通过深度测试。

<hr>

<h3>环境映射</h3>

通过使用环境的立方体贴图，我们可以给物体反射和折射的属性。

反射：需要使用法线和相机位置

```glsl
//顶点着色器
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 Normal;
out vec3 Position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Position = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}



//片元着色器
#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{             
    vec3 I = normalize(Position - cameraPos);
    vec3 R = reflect(I, normalize(Normal));
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
}
```

折射：如果要想获得物理上精确的结果，我们还需要在光线离开物体的时候再次折射，现在我们使用的只是单面折射(Single-side Refraction)，但它对大部分场合都是没问题的。

```glsl
//片元着色器
void main()
{             
    float ratio = 1.00 / 1.52;
    vec3 I = normalize(Position - cameraPos);
    vec3 R = refract(I, normalize(Normal), ratio);
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
}
```