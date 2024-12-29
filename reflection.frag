#version 410 core

in vec3 fragPos;       // Position of the fragment in world space
in vec3 viewDirection; // Direction from the camera to the fragment
in vec3 normal;        // Transformed normal vector

out vec4 fragColor;

// Uniforms
uniform samplerCube skybox; // Cubemap texture representing the environment

void main() 
{
    // Normalize the view direction and the normal vector
    vec3 viewDirectionN = normalize(viewDirection);
    vec3 normalN = normalize(normal);

    // Calculate the reflection vector
    vec3 reflection = reflect(viewDirectionN, normalN);

    // Sample the cubemap texture using the reflection vector
    vec3 colorFromSkybox = vec3(texture(skybox, reflection));

    // Set the fragment color
    fragColor = vec4(colorFromSkybox, 1.0);
}
