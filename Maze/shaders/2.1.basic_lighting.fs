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

uniform sampler2D texture1;

void main()
{
    // --- TEXTURE COLOR ---
    vec3 texColor = texture(texture1, TexCoord).rgb;

    // ambient 
    float ambientStrength = ambientS;
    vec3 ambient = ambientStrength * lightColor * texColor;

    // -------- DIREÇÕES --------
    vec3 norm = normalize(Normal);
    vec3 toLight = normalize(lightPos - FragPos);

    // Spotlight intensity
    float theta = dot(toLight, normalize(-lightDir));

    float epsilon = cutOff - outerCutOff;
    float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

    // Se estiver fora do cone → só ambient
    if (theta <= outerCutOff)
    {
        FragColor = vec4(ambient, 1.0);
        return;
    }

    // diffuse 
    float diff = max(dot(norm, toLight), 0.0);
    vec3 diffuse = diff * lightColor * texColor;

    // -------- SPECULAR --------
    float specularStrength = 0.15;   
    float shininess = 6.0;           

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-toLight, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;


    // phong
    vec3 result = ambient + (diffuse + specular) * intensity;
    FragColor = vec4(result, 1.0);
} 
