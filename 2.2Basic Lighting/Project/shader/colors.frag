#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
// -------Exercise 3--------
// extra in variable, since we need the light position in view space we calculate this in the vertex shader
// in vec3 LightPos;
// -------------------------

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

void main()
{
    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);// 得到的是从object射向光源的Dir

    // -----Exercise 3------------
    // vec3 lightDir = normalize(LightPos - FragPos);
    // ---------------------------
    float diff = max(dot(norm,lightDir),0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);

    // ---------Exercise 3:--------
    // vec3 viewDir = normalize(-FragPos);// the viewer is always at (0,0,0) in view-space, so viewDir is (0,0,0) - Position => -Position
    //-----------------------------

    vec3 reflectDir = reflect(-lightDir, norm);// 改成从光源射向object的Dir进行reflect
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);// 32是高光的反光度，越高则反射越好，散射越少，高光点越小
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}

// // Exercise 4: only render the Color
// // Fragment shader:
// // ================
// #version 330 core
// out vec4 FragColor;

// in vec3 LightingColor; 

// uniform vec3 objectColor;

// void main()
// {
//    FragColor = vec4(LightingColor * objectColor, 1.0);
// }