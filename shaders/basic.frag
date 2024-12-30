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
uniform vec3 globalLightDir;     // Direction of the global light
uniform vec3 globalLightColor;   // Color of the global light
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

// Lighting parameters
vec3 ambient;
float ambientStrength = 0.2f; // Reduced ambient strength for the global light
vec3 diffuse;
vec3 specular;
float specularStrength = 0.3f; // Dimmer specular highlight
float shininess = 32.0f;

uniform vec3 fogColor;        // Fog color
uniform float fogStart;       // Start distance for fog
uniform float fogEnd;         // End distance for fog

// Attenuation factors for the point light
float constant = 0.5f;
float linear = 0.0025f;    
float quadratic = 0.0025f; 

void computePointLight(vec3 fragPosWorld, vec3 normalWorld) {
    vec3 lightDir = normalize(lightPosition - fragPosWorld);
    float distance = length(lightPosition - fragPosWorld);

    // Calculate attenuation
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

    // Compute ambient light for the point light
    ambient += 0.95 * lightColor * attenuation;

    // Compute diffuse light for the point light
    float diff = max(dot(normalWorld, lightDir), 0.0);
    diffuse += 1.2 * diff * lightColor * attenuation;

    // Compute specular light for the point light
    vec3 viewDir = normalize(-fragPosWorld); 
    vec3 reflectDir = reflect(-lightDir, normalWorld);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    specular += specularStrength * spec * lightColor * attenuation;
}

void computeDirectionalLight(vec3 normalWorld) {
    vec3 lightDir = normalize(-globalLightDir); // Directional light direction
    ambient += ambientStrength * globalLightColor;

    // Compute diffuse light for the global light
    float diff = max(dot(normalWorld, lightDir), 0.0);
    diffuse += 0.4 * diff * globalLightColor; // Reduce diffuse contribution for the global light

    // Compute specular light for the global light
    vec3 viewDir = normalize(-fPosition); // View direction
    vec3 reflectDir = reflect(-lightDir, normalWorld);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    specular += 0.2 * specularStrength * spec * globalLightColor; // Reduce specular contribution
}

void main() {
    vec3 fragPosWorld = vec3(model * vec4(fPosition, 1.0));
    vec3 normalWorld = normalize(mat3(transpose(inverse(model))) * fNormal);

    // Initialize lighting components
    ambient = vec3(0.0);
    diffuse = vec3(0.0);
    specular = vec3(0.0);

    // Compute lighting
    computePointLight(fragPosWorld, normalWorld);
    computeDirectionalLight(normalWorld);

    // Combine texture colors with lighting
    vec3 color = (ambient + diffuse) * texture(diffuseTexture, fTexCoords).rgb 
                 + specular * texture(specularTexture, fTexCoords).rgb;

    // Fog calculation
    float fogFactor = clamp((fogEnd - length(fragPosWorld)) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 foggedColor = mix(fogColor, color, fogFactor);

    fColor = vec4(foggedColor, 1.0);
}
