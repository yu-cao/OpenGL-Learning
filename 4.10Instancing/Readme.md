<h2>实例化</h2>

**假设你有一个绘制了很多模型的场景，而大部分的模型包含的是同一组顶点数据，只不过进行的是不同的世界空间变换。想象一个充满草的场景：每根草都是一个包含几个三角形的小模型。你可能会需要绘制很多根草，最终在每帧中你可能会需要渲染上千或者上万根草。因为每一根草仅仅是由几个三角形构成，渲染几乎是瞬间完成的，但上千个渲染函数调用却会极大地影响性能。**

也就是说，大量的实例会导致性能很快耗尽，而且使用glDrawArrays或glDrawElements函数告诉GPU去绘制你的顶点数据会消耗更多的性能，因为OpenGL在绘制顶点数据之前需要做很多准备工作（比如告诉GPU该从哪个缓冲读取数据，从哪寻找顶点属性，而且这些都是在相对缓慢的CPU到GPU总线(CPU to GPU Bus)上进行的）。所以，即便渲染顶点非常快，命令GPU去渲染却未必。

<h3>如果我们能够将数据一次性发送给GPU，然后使用一个绘制函数让OpenGL利用这些数据绘制多个物体，就会更方便了。这就是实例化(Instancing)。</h3>

**想使用实例化渲染，我们只需要将`glDrawArrays`和`glDrawElements`的渲染调用分别改为`glDrawArraysInstanced`和`glDrawElementsInstanced`就可以了。**这些渲染函数的实例化版本需要一个额外的参数，叫做实例数量(Instance Count)，它能够设置我们需要渲染的实例个数。

这个函数本身并没有什么用。渲染同一个物体一千次对我们并没有什么用处，每个物体都是完全相同的，而且还在同一个位置。我们只能看见一个物体！处于这个原因，GLSL在顶点着色器中嵌入了另一个内建变量，`gl_InstanceID`。

使用实例化渲染调用时，`gl_InstanceID`会从0开始，在每个实例被渲染时递增1。比如说，我们正在渲染第43个实例，那么顶点着色器中它的gl_InstanceID将会是42。因为每个实例都有唯一的ID，我们可以建立一个数组，将ID与位置值对应起来，将每个实例放置在世界的不同位置。

```cpp
//一个简单的四边形参数
float quadVertices[] = {
    // 位置          // 颜色
    -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
     0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
    -0.05f, -0.05f,  0.0f, 0.0f, 1.0f,

    -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
     0.05f, -0.05f,  0.0f, 1.0f, 0.0f,   
     0.05f,  0.05f,  0.0f, 1.0f, 1.0f                   
}; 
```

片元着色器：

```glsl
#version 330 core
out vec4 FragColor;

in vec3 fColor;

void main()
{
    FragColor = vec4(fColor, 1.0);
}
```

主要的改动出现在顶点着色器上：

```glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out vec3 fColor;

uniform vec2 offsets[100];//定义了一个叫做offsets的数组，它包含100个偏移向量

void main()
{
    vec2 offset = offsets[gl_InstanceID];//顶点着色器中，我们会使用gl_InstanceID来索引offsets数组，获取每个实例的偏移向量
    gl_Position = vec4(aPos + offset, 0.0, 1.0);
    fColor = aColor;
}
```

如果我们要实例化绘制100个四边形，仅使用这个顶点着色器我们就能得到100个位于不同位置的四边形。但是偏移的位置需要我们进行自行在main中设定

```cpp
glm::vec2 translations[100];
int index = 0;
float offset = 0.1f;
for(int y = -10; y < 10; y += 2)
{
    for(int x = -10; x < 10; x += 2)
    {
        glm::vec2 translation;
        translation.x = (float)x / 10.0f + offset;
        translation.y = (float)y / 10.0f + offset;
        translations[index++] = translation;
    }
}
```

创建100个位移向量，表示10x10网格上的所有位置。除了生成translations数组之外，我们还需要将数据转移到顶点着色器的uniform数组中

```cpp
shader.use();
for(unsigned int i = 0; i < 100; i++)
{
    stringstream ss;
    string index;
    ss << i; 
    index = ss.str(); 
    shader.setVec2(("offsets[" + index + "]").c_str(), translations[i]);
}
```

现在所有的准备工作都做完了，我们可以开始渲染四边形了。对于实例化渲染，我们使用glDrawArraysInstanced或glDrawElementsInstanced。因为我们使用的不是索引缓冲，我们会调用glDrawArrays版本的函数：

```cpp
glBindVertexArray(quadVAO);
glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);//多了个参数用来设置需要绘制的实例数量
```

<h3>实例化数组</h3>

如果我们要渲染远超过100个实例的时候（这其实非常普遍），我们最终会超过最大能够发送至着色器的uniform数据大小的上限。

一个代替方案是实例化数组(Instanced Array)，它被定义为一个顶点属性（能够让我们储存更多的数据），仅在顶点着色器渲染一个新的实例时才会更新。

使用顶点属性时，顶点着色器的每次运行都会让GLSL获取新一组适用于当前顶点的属性。而当我们将顶点属性定义为一个实例化数组时，顶点着色器就只需要对每个实例，而不是每个顶点，更新顶点属性的内容了。这允许我们对逐顶点的数据使用普通的顶点属性，而对逐实例的数据使用实例化数组。

```glsl
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aOffset;//顶点属性

out vec3 fColor;

void main()
{
    gl_Position = vec4(aPos + aOffset, 0.0, 1.0);
    fColor = aColor;
}
```

```cpp
//把translations存进一个缓冲对象
unsigned int instanceVBO;
glGenBuffers(1, &instanceVBO);
glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 100, &translations[0], GL_STATIC_DRAW);
glBindBuffer(GL_ARRAY_BUFFER, 0);

//设置它的顶点属性指针并启用
glEnableVertexAttribArray(2);
glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
glBindBuffer(GL_ARRAY_BUFFER, 0);   
glVertexAttribDivisor(2, 1);//这个函数告诉OpenGL，处于位置值2的顶点属性是一个实例化数组
```

**`glVertexAttribDivisor`告诉了OpenGL该什么时候更新顶点属性的内容至新一组数据。**它的第一个参数是需要的顶点属性，第二个参数是属性除数。默认情况下，属性除数是0，告诉OpenGL我们需要在顶点着色器的每次迭代时更新顶点属性。**将它设置为1时，我们告诉OpenGL我们希望在渲染一个新实例的时候更新顶点属性。**而设置为2时，我们希望每2个实例更新一次属性，以此类推。

<hr>

<h3>实例：小行星带</h3>

宇宙中有一个大的行星，它位于小行星带的中央。这样的小行星带可能包含成千上万的岩块（想象：土星环）实例化渲染正是适用于这样的场景，因为所有的小行星都可以使用一个模型来表示。每个小行星可以再使用不同的变换矩阵来进行少许的变化。

为这个矩阵预留4个顶点属性(mat4 = 4 * vec4）。因为我们将它的位置值设置为3，矩阵每一列的顶点属性位置值就是3、4、5和6。（因为没有`GL_MAT4`这种类型，只有`GL_FLOAT`这种）

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in mat4 instanceMatrix;//改成mat4的顶点属性存储一个实例化的数组的变换矩阵

out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    gl_Position = projection * view * instanceMatrix * vec4(aPos, 1.0); 
    TexCoords = aTexCoords;
}
```

然后为这4个顶点属性设置属性指针，并将它们设置为实例化数组：

```cpp
//产生大量半随机化的模型转换矩阵
// ------------------------------------------------------------------
unsigned int amount = 100000;
glm::mat4* modelMatrices;
modelMatrices = new glm::mat4[amount];
srand(glfwGetTime());//随机种子
float radius = 150.0;
float offset = 25.0f;
for (unsigned int i = 0; i < amount; i++)
{
    glm::mat4 model = glm::mat4(1.0f);
    // 1. translation: displace along circle with 'radius' in range [-offset, offset]
    float angle = (float)i / (float)amount * 360.0f;
    float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float x = sin(angle) * radius + displacement;
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float y = displacement * 0.4f; // keep height of asteroid field smaller compared to width of x and z
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float z = cos(angle) * radius + displacement;
    model = glm::translate(model, glm::vec3(x, y, z));

    // 2. scale: Scale between 0.05 and 0.25f
    float scale = (rand() % 20) / 100.0f + 0.05;
    model = glm::scale(model, glm::vec3(scale));

    // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
    //相同的模型应该要有一点旋转凸显变化
    float rotAngle = (rand() % 360);
    model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

    // 4. now add to list of matrices
    modelMatrices[i] = model;
}


// 定义了额外的顶点缓冲对象存储model矩阵
unsigned int buffer;
glGenBuffers(1, &buffer);
glBindBuffer(GL_ARRAY_BUFFER, buffer);
glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

// 读取上面的buffer用作layout=3那边的mat4（也就是3~6）
//（因为没有`GL_MAT4`这种类型，只有`GL_FLOAT`这种）
for(unsigned int i = 0; i < rock.meshes.size(); i++)
{
    unsigned int VAO = rock.meshes[i].VAO;
    glBindVertexArray(VAO);
    // 顶点属性
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}
```

最后画出小行星带：

```cpp
// draw meteorites
asteroidShader.use();
asteroidShader.setInt("texture_diffuse1", 0);
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, rock.textures_loaded[0].id); // note: we also made the textures_loaded vector public (instead of private) from the model class.
for (unsigned int i = 0; i < rock.meshes.size(); i++)//这里meshes只有1，indices为576，GPU一次性就绘制完所有的岩石
{
	glBindVertexArray(rock.meshes[i].VAO);
	glDrawElementsInstanced(GL_TRIANGLES, rock.meshes[i].indices.size(), GL_UNSIGNED_INT, 0, amount);
	glBindVertexArray(0);
}
```

