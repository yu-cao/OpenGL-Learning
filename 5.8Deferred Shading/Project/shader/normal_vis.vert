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