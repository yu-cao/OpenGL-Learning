// #version 330 core
// out vec4 FragColor;

// struct Material{
//     // sampler2D是不透明类型（Opaque Type），不能将其实例化，只能通过uniform进行定义，否则可能会出现奇怪的错误
//     sampler2D diffuse;// replace the ambient and diffuse using the diffuse map
//     sampler2D specular;// replace the vector become the specular map to show more reality
    
//     float shininess;
// };

// struct Light{
//     //vec3 position;// point light

//     //vec3 direction;// directional light

//     vec3 ambient;
//     vec3 diffuse;
//     vec3 specular;

//     // point light
//     float constant;
//     float linear;
//     float quadratic;
// };

// in vec3 Normal;
// in vec3 FragPos;
// in vec2 TexCoords;

// uniform Material material;
// uniform Light light;
// uniform vec3 viewPos;

// void main()
// {
//     float theta = dot(lightDir, normalize(-light.direction));

//     // ambient
//     // 环境光的材质颜色与漫反射得到的应该相同
//     vec3 ambient = texture(material.diffuse, TexCoords).rgb * light.ambient;

//     // diffuse
//     vec3 norm = normalize(Normal);
//     vec3 lightDir = normalize(light.position - FragPos);// point light
//     //vec3 lightDir = normalize(-light.direction);// directional light
//     float diff = max(dot(norm,lightDir),0.0);
//     vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

//     // specular
//     vec3 viewDir = normalize(viewPos - FragPos);
//     vec3 reflectDir = reflect(-lightDir, norm);
//     float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
//     vec3 specular = texture(material.specular, TexCoords).rgb * spec * light.specular;

//     // point light
//     float distance = length(light.position - FragPos);
//     float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

//     ambient  *= attenuation; 
//     diffuse  *= attenuation;
//     specular *= attenuation;

//     vec3 result = ambient + diffuse + specular;
//     FragColor = vec4(result, 1.0);
// }

// spot light
#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;    
    float shininess;
}; 

struct Light {
    vec3 position;  
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    // ambient
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;
    
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;  
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;  
    
    // spotlight (soft edges)
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    diffuse  *= intensity;
    specular *= intensity;
    
    // attenuation
    float distance    = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    ambient  *= attenuation; 
    diffuse   *= attenuation;
    specular *= attenuation;   
        
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
} 