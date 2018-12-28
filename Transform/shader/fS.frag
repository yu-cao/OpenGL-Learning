#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// sampler: 采样器
uniform sampler2D texture1;
uniform sampler2D texture2;

// Exercise 4: Use the up and down arrow keys to change how much the container or the smiley face is visible
uniform float mixValue;

void main()
{
	// linearly interpolate between both textures (80% container, 20% awesomeface)
    FragColor = mix(texture(texture1,TexCoord), texture(texture2,TexCoord),0.2);

}