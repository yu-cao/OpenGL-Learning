#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

// sampler: 采样器
uniform sampler2D ourTexture;

void main()
{
    // texture(): samples the corresponding color value using the texture parameters we set earlier
    // output is the (filtered) color of the texture at the (interpolated) texture coordinate
    // 翻译：输出就是纹理的（插值）纹理坐标上的(过滤后的)颜色
    FragColor = texture(ourTexture,TexCoord) * vec4(ourColor, 1.0);// mix the vertices color and texture color
}