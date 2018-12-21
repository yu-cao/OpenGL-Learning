#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

// sampler: 采样器
uniform sampler2D texture1;
uniform sampler2D texture2;

// Exercise 4: Use the up and down arrow keys to change how much the container or the smiley face is visible
uniform float mixValue;

void main()
{
    // texture(): samples the corresponding color value using the texture parameters we set earlier
    // output is the (filtered) color of the texture at the (interpolated) texture coordinate
    // 翻译：输出就是纹理的（插值）纹理坐标上的(过滤后的)颜色
    //FragColor = texture(ourTexture,TexCoord) * vec4(ourColor, 1.0);// mix the vertices color and texture color
    // mix():linear interpolate by third param.
    // FragColor = mix(texture(texture1,TexCoord), texture(texture2,TexCoord),0.2);

    // Exercise 1: change the smile direction
    //FragColor = mix(texture(texture1,TexCoord), texture(texture2,vec2(1.0-TexCoord.x,TexCoord.y)),0.2);

    // Exercise 4:
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), mixValue);
}