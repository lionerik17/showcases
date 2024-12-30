#version 410 core

in vec3 fPosition;   // Vertex position in object space
in vec3 fNormal;     // Vertex normal in object space
in vec2 fTexCoords;  // Texture coordinates

out vec4 fColor;

// Uniforms
uniform mat4 model;           // Model matrix
uniform mat4 view;            // View matrix
uniform mat4 projection;      // Projection matrix
uniform vec3 lightPosition;   // Lamp's position in world space
uniform vec3 lightColor;      // Lamp's color
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

// Lighting parameters
vec3 ambient;
float ambientStrength = 0.5f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

// Attenuation factors
float constant = 1.0f;
float linear = 0.0045f;    // You may need to adjust these values for your scene
float quadratic = 0.0075f; // You may need to adjust these values for your scene

void computePointLight(vec3 fragPosWorld, vec3 normalWorld) {
    // Compute light direction in world space
    vec3 lightDir = normalize(lightPosition - fragPosWorld);
    float distance = length(lightPosition - fragPosWorld);

    // Calculate attenuation
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

    // Compute ambient light
    ambient = ambientStrength * lightColor * attenuation;

    // Compute diffuse light
    float diff = max(dot(normalWorld, lightDir), 0.0);
    diffuse = diff * lightColor * attenuation;

    // Compute specular light
    vec3 viewDir = normalize(-fragPosWorld); // View direction is opposite of fragment position
    vec3 reflectDir = reflect(-lightDir, normalWorld);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    specular = specularStrength * spec * lightColor * attenuation;
}

void main() {
    // Transform fragment position and normal to world space
    vec3 fragPosWorld = vec3(model * vec4(fPosition, 1.0));
    vec3 normalWorld = normalize(mat3(transpose(inverse(model))) * fNormal);

    // Compute lighting
    computePointLight(fragPosWorld, normalWorld);

    // Combine texture colors with lighting
    vec3 color = (ambient + diffuse) * texture(diffuseTexture, fTexCoords).rgb 
                 + specular * texture(specularTexture, fTexCoords).rgb;

    fColor = vec4(color, 1.0);
}
