#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

//不改变
void main()
{
   vec3 col = texture(screenTexture, TexCoords).rgb;
   FragColor = vec4(col, 1.0);
}

//反相：从屏幕纹理中取颜色值，然后用1.0减去它，对它进行反相
// void main()
// {
//     FragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoords)), 1.0);
// }

//灰度：移除场景中除了黑白灰以外所有的颜色，让整个图像灰度化(Grayscale)
//很简单的实现方式是，取所有的颜色分量，将它们平均化：
// void main()
// {
//     FragColor = texture(screenTexture, TexCoords);
//     float average = (FragColor.r + FragColor.g + FragColor.b) / 3.0;
//     FragColor = vec4(average, average, average, 1.0);
// }
//人眼会对绿色更加敏感一些，而对蓝色不那么敏感，所以为了获取物理上更精确的效果，我们需要使用加权的(Weighted)通道：
// void main()
// {
//     FragColor = texture(screenTexture, TexCoords);
//     float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
//     FragColor = vec4(average, average, average, 1.0);
// }

//核效果：使用卷积核
//它的中心为当前的像素，它会用它的核值乘以周围的像素值，并将结果相加变成一个值
//所以，基本上我们是在对当前像素周围的纹理坐标添加一个小的偏移量，并根据核将结果合并
//大部分核将所有的权重加起来之后都应该会等于1，如果它们加起来不等于1，这就意味着最终的纹理颜色将会比原纹理值更亮或者更暗了。
//需要稍微修改一下片段着色器，让它能够支持核。我们假设使用的核都是3x3核:
//这个例子中是一个锐化(Sharpen)核，这能创建一些很有趣的效果，比如说玩家打了麻醉剂所感受到的效果。
// const float offset = 1.0 / 300.0;  

// void main()
// {
//     vec2 offsets[9] = vec2[](
//         vec2(-offset,  offset), // 左上
//         vec2( 0.0f,    offset), // 正上
//         vec2( offset,  offset), // 右上
//         vec2(-offset,  0.0f),   // 左
//         vec2( 0.0f,    0.0f),   // 中
//         vec2( offset,  0.0f),   // 右
//         vec2(-offset, -offset), // 左下
//         vec2( 0.0f,   -offset), // 正下
//         vec2( offset, -offset)  // 右下
//     );

//     // //锐化核
//     // float kernel[9] = float[](
//     //     -1, -1, -1,
//     //     -1,  9, -1,
//     //     -1, -1, -1
//     // );
//     //模糊核
//     // float kernel[9] = float[](
//     // 1.0 / 16, 2.0 / 16, 1.0 / 16,
//     // 2.0 / 16, 4.0 / 16, 2.0 / 16,
//     // 1.0 / 16, 2.0 / 16, 1.0 / 16  
//     // );
//     //边缘检测(Edge-detection)核
//     float kernel[9] = float[](
//         1,  1, 1,
//         1, -8, 1,
//         1,  1, 1
//     );

//     vec3 sampleTex[9];
//     for(int i = 0; i < 9; i++)
//     {
//         sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
//     }
//     vec3 col = vec3(0.0);
//     for(int i = 0; i < 9; i++)
//         col += sampleTex[i] * kernel[i];

//     FragColor = vec4(col, 1.0);
// }