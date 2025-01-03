#version 410 core

in vec3 fragPos; // Fragment position in world space
in vec3 fragToLight; // Vector from fragment to light

uniform vec3 lightPos; // Light position
uniform float farPlane; // Far plane distance

void main() {
    // Calculate depth relative to the light position
    float distance = length(fragPos - lightPos);
    distance = distance / farPlane;
    gl_FragDepth = distance;
}
