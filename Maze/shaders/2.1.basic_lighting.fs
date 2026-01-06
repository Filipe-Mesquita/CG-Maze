#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform bool flashlightOn;

// Spotlight cone
uniform float cutOff;
uniform float outerCutOff;

// Attenuation
uniform float constant;
uniform float linear;
uniform float quadratic;

uniform sampler2D texture1;

void main()
{
    vec3 texColor = texture(texture1, TexCoord).rgb;

    // ─────────────── AMBIENT (muito fraco)
    float ambientStrength = flashlightOn ? 0.12 : 0.02;
    vec3 ambient = ambientStrength * lightColor * texColor;

    if (!flashlightOn)
    {
        FragColor = vec4(ambient, 1.0);
        return;
    }

    // ─────────────── NORMALS
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightPos - FragPos);

    // ─────────────── SPOTLIGHT INTENSITY
    float theta = dot(lightDirection, normalize(-lightDir));

    float intensity = clamp((theta - outerCutOff) / (cutOff - outerCutOff), 0.0, 1.0);

    // ─────────────── DISTANCE ATTENUATION
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);

    // ─────────────── DIFFUSE
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;

    // ─────────────── SPECULAR (pedra → fraco)
    float specularStrength = 0.08;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = specularStrength * spec * lightColor;

    // ─────────────── FINAL COLOR
    vec3 lighting = (diffuse + specular) * intensity * attenuation;

    vec3 result = ambient + lighting * texColor;
    FragColor = vec4(result, 1.0);
}
