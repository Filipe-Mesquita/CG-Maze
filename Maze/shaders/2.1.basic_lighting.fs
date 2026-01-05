#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;
in vec2 TexCoord; 

uniform float ambientS;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Inner and Outer angle of the spotlight's cone
uniform float cutOff;
uniform float outerCutOff;

uniform bool flashlightOn;

uniform sampler2D texture1;

void main()
{
     vec3 texColor = texture(texture1, TexCoord).rgb;

    // ───────────────── AMBIENT (sempre existe)
    float ambientStrength = flashlightOn ? 0.15 : 0.02;
    vec3 ambient = ambientStrength * lightColor * texColor;

    // Se a lanterna estiver desligada → só ambient
    if (!flashlightOn)
    {
        FragColor = vec4(ambient, 1.0);
        return;
    }

    // ───────────────── NORMALS
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightPos - FragPos);

    // ───────────────── SPOTLIGHT (cone)
    float theta = dot(lightDirection, normalize(-lightDir));

    float innerCutoff = cos(radians(12.0)); // cone interior
    float outerCutoff = cos(radians(18.0)); // borda suave

    float intensity = clamp(
        (theta - outerCutoff) / (innerCutoff - outerCutoff),
        0.0, 1.0
    );

    // ───────────────── DIFFUSE
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor * intensity;

    // ───────────────── SPECULAR (muito fraco → pedra)
    float specularStrength = 0.1;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = specularStrength * spec * lightColor * intensity;

    vec3 result = ambient + (diffuse + specular) * texColor;
    FragColor = vec4(result, 1.0);
} 
