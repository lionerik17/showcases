#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;

out vec3 fragPos;       
out vec3 viewDirection; 
out vec3 normal;        

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos; 

void main() 
{
    vec4 worldPos = model * vec4(vPosition, 1.0);
    fragPos = vec3(worldPos);

    viewDirection = cameraPos - fragPos;

    normal = mat3(transpose(inverse(model))) * vNormal;

    gl_Position = projection * view * worldPos;
}
