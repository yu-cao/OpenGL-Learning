#version 330 core
out vec4 FragColor;

struct Material{
    // sampler2D是不透明类型（Opaque Type），不能将其实例化，只能通过uniform进行定义，否则可能会出现奇怪的错误
    sampler2D diffuse;// replace the ambient and diffuse using the diffuse map
    sampler2D specular;// replace the vector become the specular map to show more reality
    
    // // -----Exercise 3:-----
    // sampler2D emission;
    // // ---------------------

    float shininess;
};

struct Light{
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

void main()
{
    // ambient
    // 环境光的材质颜色与漫反射得到的应该相同
    vec3 ambient = texture(material.diffuse, TexCoords).rgb * light.ambient;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm,lightDir),0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = texture(material.specular, TexCoords).rgb * spec * light.specular;
    // ----Exercise 1: ------
    //vec3 specular = light.specular * spec * (vec3(1.0) - vec3(texture(material.specular, TexCoords))); // here we inverse the sampled specular color. Black becomes white and white becomes black.
    // ----------------------

    // // -----Exercise 3:-----
    // // emission
    // vec3 emission = texture(material.emission, TexCoords).rgb;

    // vec3 result = ambient + diffuse + specular + emission;
    // // ---------------------

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}