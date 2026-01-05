#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D sceneTex;
uniform float time;
uniform float intensity; // 0..1

void main()
{
    vec2 uv = TexCoord;

    // wobble geral (ondas)
    float w1 = sin(uv.y * 12.0 + time * 2.3) * 0.006;
    float w2 = sin(uv.x * 10.0 + time * 1.7) * 0.006;
    uv += vec2(w1, w2) * intensity;

    // distorção radial (tipo lente)
    vec2 center = vec2(0.5);
    vec2 d = uv - center;
    float r = dot(d, d);
    uv += d * r * 0.25 * intensity;

    // ligeira aberração cromática (RGB separado)
    vec2 chroma = d * 0.012 * intensity;
    float rr = texture(sceneTex, uv + chroma).r;
    float gg = texture(sceneTex, uv).g;
    float bb = texture(sceneTex, uv - chroma).b;

    vec3 col = vec3(rr, gg, bb);

    // vinheta suave
    float vig = smoothstep(0.9, 0.2, length(d));
    col *= mix(1.0, vig, 0.35 * intensity);

    FragColor = vec4(col, 1.0);
}
