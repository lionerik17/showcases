#version 410 core

in vec3 fragPos;       
in vec3 viewDirection; 
in vec3 normal;        

out vec4 fragColor;

uniform samplerCube skybox;

void main() 
{
    vec3 viewDirectionN = normalize(viewDirection);
    vec3 normalN = normalize(normal);

    vec3 reflection = reflect(viewDirectionN, normalN);

    vec3 colorFromSkybox = vec3(texture(skybox, reflection));

    fragColor = vec4(colorFromSkybox, 1.0);
}
