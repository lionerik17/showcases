#version 410 core

layout(location = 0) in vec3 position;

uniform mat4 lightMatrices[6];
uniform vec3 lightPosition;
uniform float farPlane;

out vec3 fragPos; // Fragment position in world space
out vec3 fragToLight; // Vector from fragment to light

void main() {
    fragPos = position; // Pass the vertex position in world space
    fragToLight = position - lightPosition; // Vector from fragment to the light source
    gl_Position = lightMatrices[gl_InstanceID] * vec4(position, 1.0); // Calculate position in light space
}
