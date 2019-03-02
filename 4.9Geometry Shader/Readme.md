<h2>几何着色器</h2>

顶点和片段着色器之间有一个可选的几何着色器(Geometry Shader)，几何着色器的输入是一个图元（如点或三角形）的一组顶点。几何着色器可以在顶点发送到下一着色器阶段之前对它们随意变换。

它能够将（这一组）顶点变换为完全不同的图元，并且还能生成比原来更多的顶点。

```glsl
#version 330 core
layout (points) in;
layout (line_strip, max_vertices = 2) out;

void main() {    
    gl_Position = gl_in[0].gl_Position + vec4(-0.1, 0.0, 0.0, 0.0); 
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4( 0.1, 0.0, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}
```

在几何着色器的顶部，我们需要声明从顶点着色器输入的图元类型。需要在in关键字前声明一个布局修饰符。这个输入布局修饰符可以从顶点着色器接收下列任何一个图元值

+ `points`：绘制`GL_POINTS`图元时（1）。
+ `lines`：绘制`GL_LINES`或`GL_LINE_STRIP`时（2）
+ `lines_adjacency`：`GL_LINES_ADJACENCY`或`GL_LINE_STRIP_ADJACENCY`（4）
+ `triangles`：`GL_TRIANGLES、GL_TRIANGLE_STRIP`或`GL_TRIANGLE_FAN`（3）
+ `triangles_adjacency`：`GL_TRIANGLES_ADJACENCY`或`GL_TRIANGLE_STRIP_ADJACENCY`（6）

以上是能提供给glDrawArrays渲染函数的几乎所有图元了。如果我们想要将顶点绘制为GL_TRIANGLES，我们就要将输入修饰符设置为triangles。括号内的数字表示的是一个图元所包含的最小顶点数。

接下来，我们还需要指定几何着色器输出的图元类型，这需要在out关键字前面加一个布局修饰符。和输入布局修饰符一样，输出布局修饰符也可以接受几个图元值：

+ points
+ line_strip（线条，上面现在最大点为2点，也就代表只能输出一条线段）
+ triangle_strip

要生成一个三角形的话，我们将输出定义为triangle_strip，并输出3个顶点。

几何着色器同时希望我们设置一个它最大能够输出的顶点数量（如果你超过了这个值，OpenGL将不会绘制多出的顶点），这个也可以在out关键字的布局修饰符中设置。在这个例子中，我们将输出一个line_strip，并将最大顶点数设置为2个。

为了生成更有意义的结果，我们需要某种方式来获取前一着色器阶段的输出。GLSL提供给我们一个内建(Built-in)变量作为一个接口块，它被声明为一个数组，因为大多数的渲染图元包含多于1个的顶点，而几何着色器的输入是一个图元的所有顶点。

```glsl
in gl_Vertex
{
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];
```

有了之前顶点着色器阶段的顶点数据，我们就可以使用2个几何着色器函数，EmitVertex和EndPrimitive，来生成新的数据了。几何着色器希望你能够生成并输出至少一个定义为输出的图元。在我们的例子中，我们需要至少生成一个线条图元。

```glsl
void main() {
    gl_Position = gl_in[0].gl_Position + vec4(-0.1, 0.0, 0.0, 0.0); 
    EmitVertex();//调用后，gl_Position中的向量会被添加到图元中来

    gl_Position = gl_in[0].gl_Position + vec4( 0.1, 0.0, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();//调用后，所有发射出的(Emitted)顶点都会合成为指定的输出渲染图元
    //这里，我们发射了两个顶点，它们从原始顶点位置平移了一段距离，之后将这两个顶点合成为一个包含两个顶点的线条。
}
```

<hr>

<h3>使用几何着色器</h3>

在标准化设备坐标的z平面上绘制四个点。这些点的坐标是：

```cpp
float points[] = {
    -0.5f,  0.5f, // 左上
     0.5f,  0.5f, // 右上
     0.5f, -0.5f, // 右下
    -0.5f, -0.5f  // 左下
};
```

顶点着色器：

```glsl
#version 330 core
layout (location = 0) in vec2 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}
```

片元着色器：

```glsl
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(0.0, 1.0, 0.0, 1.0);   
}
```

场景

```cpp
shader.use();
glBindVertexArray(VAO);
glDrawArrays(GL_POINTS, 0, 4);
```

结果是在黑暗的场景中有四个（很难看见的）绿点：

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/4.9Geometry%20Shader/Reference/geometry_shader_points.png)

接下来使用几何着色器：（不对其进行任何变化）

```glsl
#version 330 core
layout (points) in;
layout (points, max_vertices = 1) out;

void main() {    
    gl_Position = gl_in[0].gl_Position; 
    EmitVertex();
    EndPrimitive();
}
```

和顶点与片段着色器一样，几何着色器也需要编译和链接，但这次在创建着色器时我们将会使用GL_GEOMETRY_SHADER作为着色器类型：

```cpp
geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
glShaderSource(geometryShader, 1, &gShaderCode, NULL);
glCompileShader(geometryShader);  
...
glAttachShader(program, geometryShader);
glLinkProgram(program);
```

这样就完成了最基本的几何着色器，有了几何着色器，你甚至**可以将最简单的图元变得十分有创意。因为这些形状是在GPU的超快硬件中动态生成的，这会比在顶点缓冲中手动定义图形要高效很多。因此，几何缓冲对简单而且经常重复的形状来说是一个很好的优化工具，比如体素(Voxel)世界中的方块和室外草地的每一根草。**

<h3>例如：我们可以通过它做出爆破效果</h3>

爆破一个物体时，我们并不是指要将宝贵的顶点集给炸掉，我们是要将每个三角形沿着法向量的方向移动一小段时间。效果就是，整个物体看起来像是沿着每个三角形的法线向量爆炸一样。

因为我们想要沿着三角形的法向量位移每个顶点，我们首先需要计算这个法向量。几何着色器函数做的正是这个(已知一个三角形的三个顶点做出这个平面的法向量）：

```glsl
vec3 GetNormal()
{
   vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
   vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
   return normalize(cross(a, b));
}
```

由此创建一个explode函数：

```glsl
vec4 explode(vec4 position, vec3 normal)
{
    float magnitude = 2.0;
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude; 
    return position + vec4(direction, 0.0);
}
```

最后顶点着色器的代码是（思想：顶点发生偏移，但是纹理还是绑定在应该在的三角面上）：

```glsl
#version 330 core
//进来的是一个三角形片的三个顶点
//出去是3个顶点的三角形块
//在第一个三角形绘制完之后，每个后续顶点将会在上一个三角形边上生成另一个三角形：每3个临近的顶点将会形成一个三角形。
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec2 texCoords;
} gs_in[];

out vec2 TexCoords; 

uniform float time;

vec4 explode(vec4 position, vec3 normal) { ... }

vec3 GetNormal() { ... }

//注意我们在发射顶点之前输出了对应的纹理坐标，不然整个效果会扭曲
void main() {    
    vec3 normal = GetNormal();

    gl_Position = explode(gl_in[0].gl_Position, normal);
    TexCoords = gs_in[0].texCoords;
    EmitVertex();
    gl_Position = explode(gl_in[1].gl_Position, normal);
    TexCoords = gs_in[1].texCoords;
    EmitVertex();
    gl_Position = explode(gl_in[2].gl_Position, normal);
    TexCoords = gs_in[2].texCoords;
    EmitVertex();
    EndPrimitive();
}
```

别忘了在OpenGL代码中设置time变量：`shader.setFloat("time", glfwGetTime());
`

<hr>

<h2>可以用来进行调试：法向量可视化</h2>

显示任意物体的法向量。当编写光照着色器时，你可能会最终会得到一些奇怪的视觉输出，但又很难确定导致问题的原因。光照错误很常见的原因就是法向量错误，这可能是由于不正确加载顶点数据、错误地将它们定义为顶点属性或在着色器中不正确地管理所导致的。我们想要的是使用某种方式来检测提供的法向量是正确的。检测法向量是否正确的一个很好的方式就是对它们进行可视化，几何着色器正是实现这一目的非常有用的工具。

**思路是这样的：我们首先不使用几何着色器正常绘制场景。然后再次绘制场景，但这次只显示通过几何着色器生成法向量。几何着色器接收一个三角形图元，并沿着法向量生成三条线——每个顶点一个法向量。**

这次在几何着色器中，我们会使用模型提供的顶点法线，而不是自己生成，为了适配（观察和模型矩阵的）缩放和旋转，我们在将法线变换到裁剪空间坐标之前，先使用法线矩阵变换一次（几何着色器接受的位置向量是剪裁空间坐标，所以我们应该将法向量变换到相同的空间中）。这可以在顶点着色器中完成：

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out VS_OUT {
    vec3 normal;//传出去的是模型上的法线在投影空间下的方向
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    mat3 normalMatrix = mat3(transpose(inverse(view * model)));//先求到法线矩阵，该把法线从模型空间变换到视角空间
    vs_out.normal = vec3(projection * vec4(normalMatrix * aNormal, 0.0));//把法线进行变换，然后转换到投影空间
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

变换后的裁剪空间法向量会以接口块的形式传递到下个着色器阶段。接下来，几何着色器会接收每一个顶点（包括一个位置向量和一个法向量），并在每个位置向量处绘制一个法线向量：

```glsl
#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

const float MAGNITUDE = 0.4;//法向量乘以了一个MAGNITUDE向量，来限制显示出的法向量大小（否则它们就有点大了）

void GenerateLine(int index)
{
    gl_Position = gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = gl_in[index].gl_Position + vec4(gs_in[index].normal, 0.0) * MAGNITUDE;//让顶点向法线方向平移，构成一个线段
    EmitVertex();
    EndPrimitive();
}

void main()
{
    GenerateLine(0); // 第一个顶点法线
    GenerateLine(1); // 第二个顶点法线
    GenerateLine(2); // 第三个顶点法线
}
```

片元着色器就直接显示单色的线进行调试即可。

这个方法也可以同时用来制作毛发等效果