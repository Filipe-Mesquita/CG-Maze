#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform bool flashlightMode;
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
    vec3 norm = normalize(Normal);

    // ─────────────────────────────
    // MODO 1 — JOGO SEM LANTERNA
    // ─────────────────────────────
    if (!flashlightMode)
    {
        vec3 lightDir = normalize(vec3(-0.3, -1.0, -0.2)); // luz global
        float diff = max(dot(norm, -lightDir), 0.0);

        vec3 ambient = 0.25 * lightColor * texColor;
        vec3 diffuse = diff * lightColor * texColor;

        FragColor = vec4(ambient + diffuse, 1.0);
        return;
    }

    // ─────────────────────────────
    // MODO 2 — JOGO COM LANTERNA
    // ─────────────────────────────

    // Ambient base (sempre)
    vec3 ambient = 0.12 * lightColor * texColor;

    // Lanterna desligada
    if (!flashlightOn)
    {
        FragColor = vec4(ambient, 1.0);
        return;
    }

    // Lanterna ligada (spotlight)
    vec3 lightDirection = normalize(lightPos - FragPos);
    float theta = dot(lightDirection, normalize(-lightDir));

    float intensity = clamp(
        (theta - outerCutOff) / (cutOff - outerCutOff),
        0.0, 1.0
    );

    float distance = length(lightPos - FragPos);
    float attenuation =
        1.0 / (constant + linear * distance + quadratic * distance * distance);

    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = 0.08 * spec * lightColor;

    vec3 lighting =
        (diffuse + specular) * intensity * attenuation;

    FragColor = vec4(ambient + lighting * texColor, 1.0);
}