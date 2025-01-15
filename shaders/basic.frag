#version 410 core

in vec3 fPosition;   
in vec3 fNormal;     
in vec2 fTexCoords;  
flat in vec3 fNormalFlat;
in vec4 fragPosLightSpace;

out vec4 fColor;

uniform mat4 model;           
uniform mat4 view;            
uniform mat4 projection;      
uniform vec3 lightPosition;   
uniform vec3 lightColor;      
uniform vec3 globalLightDir;     
uniform vec3 globalLightColor;   
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform vec3 light2Position;   
uniform vec3 light2Color;      
uniform vec3 light2Direction; 
uniform bool useFlatShading; 
uniform bool isDay;
uniform sampler2D shadowMap;

vec3 ambient;
uniform float ambientStrength = 0.15f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.7f; 
float shininess = 32.0f;

uniform vec3 fogColor;      
uniform float fogStart;       
uniform float fogEnd;        

float constant = 0.5f;
float linear = 0.0025f;    
float quadratic = 0.0025f;

float computeShadow() {
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; 

	normalizedCoords = normalizedCoords * 0.5 + 0.5; 

	float closestDepth = texture(shadowMap, normalizedCoords.xy).r; 

	if (normalizedCoords.z > 1.0f) 
		return 0.0f; 
	float currentDepth = normalizedCoords.z; 

	float bias = 0.005f; 
	float shadow = currentDepth - bias > closestDepth  ? 1.0f : 0.0f; 

	return shadow;
}

void computePointLight(vec3 fragPosWorld, vec3 normalWorld) {
    vec3 lightDir = normalize(lightPosition - fragPosWorld);
    float distance = length(lightPosition - fragPosWorld);

    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

    ambient += 0.95 * lightColor * attenuation;

    float diff = max(dot(normalWorld, lightDir), 0.0);
    diffuse += 1.2 * diff * lightColor * attenuation;

    vec3 viewDir = normalize(-fragPosWorld); 
    vec3 reflectDir = reflect(-lightDir, normalWorld);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    specular += specularStrength * spec * lightColor * attenuation;
}

void computePointLight2(vec3 fragPosWorld, vec3 normalWorld) {
    vec3 lightDir = normalize(light2Position - fragPosWorld);
    float distance = length(light2Position - fragPosWorld);

    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

    float theta = dot(normalize(-lightDir), normalize(light2Direction));
    float directionFactor = max(theta, 0.0); // Ensure non-negative influence

    float effect = attenuation * directionFactor;

    ambient += 0.8 * light2Color * effect;

    float diff = max(dot(normalWorld, lightDir), 0.0);
    diffuse += diff * light2Color * effect;

    vec3 viewDir = normalize(-fragPosWorld);
    vec3 reflectDir = reflect(-lightDir, normalWorld);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    specular += specularStrength * spec * light2Color * effect;
}

void computeDirectionalLight(vec3 normalWorld) {
    vec3 lightDir = normalize(-globalLightDir); 
    ambient += ambientStrength * globalLightColor;

    float diff = max(dot(normalWorld, lightDir), 0.0);
    diffuse += 0.4 * diff * globalLightColor;

    vec3 viewDir = normalize(-fPosition); // View direction
    vec3 reflectDir = reflect(-lightDir, normalWorld);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    specular += 0.2 * specularStrength * spec * globalLightColor;
}

void main() {
    vec3 fragPosWorld = vec3(model * vec4(fPosition, 1.0));
    vec3 normalWorld = useFlatShading ? normalize(fNormalFlat) : normalize(fNormal);
    float shadow = computeShadow();

    ambient = vec3(0.0);
    diffuse = vec3(0.0);
    specular = vec3(0.0);

    if (!isDay) {
        computePointLight(fragPosWorld, normalWorld);
        computePointLight2(fragPosWorld, normalWorld);
        computeDirectionalLight(normalWorld);
        diffuse *= (1.0 - shadow); 
        specular *= (1.0 - shadow);
    } else {
        computeDirectionalLight(normalWorld);
        ambient = ambientStrength * globalLightColor;

        ambient *= mix(0.3, 1.0, 1.0 - shadow);
    }

    vec3 color = (ambient + diffuse) * texture(diffuseTexture, fTexCoords).rgb 
                 + specular * texture(specularTexture, fTexCoords).rgb;

    if (!isDay) {
        float fogFactor = clamp((fogEnd - length(fragPosWorld)) / (fogEnd - fogStart), 0.0, 1.0);
        vec3 foggedColor = mix(fogColor, color, fogFactor);
        fColor = vec4(foggedColor, 1.0);
    } else {
        fColor = vec4(color, 1.0);
    }
}
