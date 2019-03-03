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

// float ShadowCalculation(vec4 fragPosLightSpace)
// {
//     //进行透视除法
//     vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

//     //上面的projCoords的xyz分量都是[-1,1]（下面会指出这对于远平面之类的点才成立）
//     //而为了和深度贴图的深度相比较，z分量需要变换到[0,1]
//     //为了作为从深度贴图中采样的坐标，xy分量也需要变换到[0,1]；所以整个projCoords向量都需要变换到[0,1]范围
//     projCoords = projCoords * 0.5 + 0.5;

//     //得到光的位置视野下最近的深度
//     float closestDepth = texture(shadowMap, projCoords.xy).r;

//     // 得到片元的当前深度，我们简单获取投影向量的z坐标，它等于来自光源视角的片元的深度
//     float currentDepth = projCoords.z;

//     //简单检查currentDepth是否高于closetDepth，如果是，那么片元就在阴影中。
//     float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

//     return shadow;
// }

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
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
    //float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

    //进行阴影校正
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    //对于远平面保持光照
    if(projCoords.z > 1.0)
        shadow = 0.0;

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
    //float shadow = ShadowCalculation(fs_in.FragPosLightSpace);//计算出一个shadow值，当fragment在阴影中时是1.0，在阴影外是0.0
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, normal, lightDir);//进行阴影失真校正
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

    FragColor = vec4(lighting, 1.0);
}