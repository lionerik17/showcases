#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;

out vec3 fragPos;       // Position of the fragment in world space
out vec3 viewDirection; // Direction from the camera to the fragment
out vec3 normal;        // Transformed normal vector

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos; // Position of the camera in world space

void main() 
{
    // Compute fragment position in world space
    vec4 worldPos = model * vec4(vPosition, 1.0);
    fragPos = vec3(worldPos);

    // Compute view direction (from the camera to the fragment)
    viewDirection = cameraPos - fragPos;

    // Transform the normal vector to world space
    normal = mat3(transpose(inverse(model))) * vNormal;

    // Output the transformed vertex position
    gl_Position = projection * view * worldPos;
}
