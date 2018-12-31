#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

// ----- Exercise 3: ------
// we now define the uniform in the vertex shader and pass the 'view space' lightpos to the fragment shader.
// lightPos is currently in world space.
// out vec3 LightPos;
// uniform vec3 lightPos;
// -------------------

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
     gl_Position = projection * view * model * vec4(aPos, 1.0f);
     FragPos = vec3(model * vec4(aPos,1.0));
     Normal = aNormal;

     // -----Exercise 3: 在观察空间中计算光照-----
    //  gl_Position = projection * view * model * vec4(aPos, 1.0);
    //  FragPos = vec3(view * model * vec4(aPos, 1.0));
    //  Normal = mat3(transpose(inverse(view * model))) * aNormal;
    //  LightPos = vec3(view * vec4(lightPos, 1.0)); // Transform world-space light position to view-space light position
     // ---------------------------------------
}

//// Exercise 4: Vertex shader:
//// ================
//#version 330 core
//layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec3 aNormal;
//
//out vec3 LightingColor; // resulting color from lighting calculations
//
//uniform vec3 lightPos;
//uniform vec3 viewPos;
//uniform vec3 lightColor;
//
//uniform mat4 model;
//uniform mat4 view;
//uniform mat4 projection;
//
//void main()
//{
//    gl_Position = projection * view * model * vec4(aPos, 1.0);
//
//    // gouraud shading
//    // ------------------------
//    vec3 Position = vec3(model * vec4(aPos, 1.0));
//    vec3 Normal = mat3(transpose(inverse(model))) * aNormal;
//
//    // ambient
//    float ambientStrength = 0.1;
//    vec3 ambient = ambientStrength * lightColor;
//
//    // diffuse
//    vec3 norm = normalize(Normal);
//    vec3 lightDir = normalize(lightPos - Position);
//    float diff = max(dot(norm, lightDir), 0.0);
//    vec3 diffuse = diff * lightColor;
//
//    // specular
//    float specularStrength = 1.0; // this is set higher to better show the effect of Gouraud shading
//    vec3 viewDir = normalize(viewPos - Position);
//    vec3 reflectDir = reflect(-lightDir, norm);
//    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
//    vec3 specular = specularStrength * spec * lightColor;
//
//    LightingColor = ambient + diffuse + specular;
//}
