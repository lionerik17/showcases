#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
flat out vec3 fNormalFlat;
out vec4 fragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform	mat3 normalMatrix;
uniform bool useFlatShading;

uniform mat4 lightSpaceTrMatrix;

void main() 
{
    fragPosLightSpace = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f);
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);
	fPosition = vPosition;
    if (useFlatShading) {
        fNormalFlat = normalize(normalMatrix * vNormal);
    } else {
        fNormal = normalize(normalMatrix * vNormal);
    }
	fTexCoords = vTexCoords;
}
