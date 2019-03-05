//几何着色器：负责将所有世界空间的顶点变换到6个不同的光空间
//输入一个三角形，输出总共6个三角形（6个面 * 3个顶点，所以总共18个顶点）
#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 shadowMatrices[6];

out vec4 FragPos;//还要将最后的FragPos变量发送给片元着色器，我们需要计算一个深度值

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face;//内建变量，指定发散出基本图形送到立方体贴图的哪个面
        for(int i = 0; i < 3; ++i)//把面（3个顶点）的光源空间变换矩阵乘以FragPos，将每个世界空间顶点变换到相关的光空间，生成每个三角形。
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = shadowMatrices[face] * FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}