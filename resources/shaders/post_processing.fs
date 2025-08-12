#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float time;
uniform vec2 resolution;
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform sampler2D texture1;
uniform sampler2D texture2;

out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord;
    vec2 centered = uv - 0.5;
    float r2 = dot(centered, centered);

    float k = 0.08;
    vec2 uvBarrel = 0.5 + centered * (1.0 + k * r2);
    uvBarrel = clamp(uvBarrel, vec2(0.0), vec2(1.0));

    float aberrBase = 0.0018;
    float dist = length(centered);
    float edgeAtten = 1.0 - smoothstep(0.6, 0.95, dist);
    vec2 ca = centered * (aberrBase + 0.004 * r2) * edgeAtten;

    vec2 uvR = clamp(uvBarrel + ca, vec2(0.0), vec2(1.0));
    vec2 uvG = uvBarrel;
    vec2 uvB = clamp(uvBarrel - ca, vec2(0.0), vec2(1.0));
    vec4 cR = texture(texture0, uvR);
    vec4 cG = texture(texture0, uvG);
    vec4 cB = texture(texture0, uvB);
    vec3 col = vec3(cR.r, cG.g, cB.b);

    float threshold = 0.72;
    vec3 bright = max(col - vec3(threshold), vec3(0.0));

    vec2 px = 1.0 / max(resolution, vec2(1.0));
    vec3 bloom = vec3(0.0);
    bloom += texture(texture0, clamp(uvBarrel + vec2( 1.0,  0.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2(-1.0,  0.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2( 0.0,  1.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2( 0.0, -1.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2( 1.0,  1.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2(-1.0,  1.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2( 1.0, -1.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2(-1.0, -1.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2( 2.0,  0.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2(-2.0,  0.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2( 0.0,  2.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom += texture(texture0, clamp(uvBarrel + vec2( 0.0, -2.0) * px, vec2(0.0), vec2(1.0))).rgb;
    bloom /= 12.0;
    bloom = max(bloom - vec3(threshold), vec3(0.0));

    col += bloom * 0.55;

    float grain = fract(sin(dot(uv * resolution + time * 57.0, vec2(12.9898, 78.233))) * 43758.5453);
    col += (grain - 0.5) * 0.02;

    float vig = smoothstep(0.55, 0.9, dist);
    col *= mix(1.0, 0.85, vig);

    float pulse = 0.95 + 0.06 * sin(time * 1.7);
    finalColor = vec4(col * pulse, 1.0) * colDiffuse;
}