#version 410 core

layout(location = 0) in vec3 aPosition; 
layout(location = 1) in vec3 aVelocity; 

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float deltaTime;

out vec3 vColor;

void main() {
    vec3 currentPosition = aPosition + aVelocity * deltaTime; // Update position based on velocity
    if (currentPosition.y < -50.0) { // Reset raindrop if it falls below the scene
        currentPosition.y = 50.0 + mod(currentPosition.x + currentPosition.z, 10.0);
    }
    vColor = vec3(0.5, 0.5, 1.0); // Raindrop color
    gl_Position = projection * view * model * vec4(currentPosition, 1.0);
    gl_PointSize = 2.0; // Size of each raindrop
}
