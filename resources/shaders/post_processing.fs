#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float time;
uniform vec2 resolution;
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform sampler2D texture1;
uniform sampler2D texture2;

// Bloom controls
uniform float bloomThreshold = 0.5;    // Brightness threshold for bloom
uniform float bloomIntensity = 1.2;    // Overall bloom strength
uniform float bloomRadius = 4.0;       // Bloom spread radius
uniform vec3 bloomTint = vec3(1.0);    // Color tint for bloom

out vec4 finalColor;

bool inBounds(vec2 p) {
    return all(greaterThanEqual(p, vec2(0.0))) && all(lessThanEqual(p, vec2(1.0)));
}

vec4 safeTex0(vec2 p) {
    if (!inBounds(p)) return vec4(0.0);
    return texture(texture0, p);
}

vec3 sampleBloom(vec2 uv, vec2 px, float radius) {
    vec3 bloom = vec3(0.0);
    float weight = 0.0;
    float sigma = radius * 0.5;
    
    for(float y = -radius; y <= radius; y += 1.0) {
        for(float x = -radius; x <= radius; x += 1.0) {
            vec2 offset = vec2(x, y) * px;
            float dist = length(offset);
            float gauss = exp(-(dist * dist) / (2.0 * sigma * sigma));
            bloom += safeTex0(uv + offset).rgb * gauss;
            weight += gauss;
        }
    }
    
    return bloom / max(weight, 0.00001);
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 centered = uv - 0.5;
    float r2 = dot(centered, centered);

    float k = 0.08;
    vec2 uvBarrel = 0.5 + centered * (1.0 + k * r2);

    float aberrBase = 0.0018;
    float dist = length(centered);
    float edgeAtten = 1.0 - smoothstep(0.6, 0.95, dist);
    vec2 ca = centered * (aberrBase + 0.004 * r2) * edgeAtten;

    vec2 uvR = uvBarrel + ca;
    vec2 uvG = uvBarrel;
    vec2 uvB = uvBarrel - ca;
    vec4 cR = safeTex0(uvR);
    vec4 cG = safeTex0(uvG);
    vec4 cB = safeTex0(uvB);
    vec3 col = vec3(cR.r, cG.g, cB.b);

    // Extract bright areas
    vec3 bright = max(col - vec3(bloomThreshold), vec3(0.0));
    
    // Sample bloom with gaussian blur
    vec2 px = 1.0 / max(resolution, vec2(1.0));
    vec3 bloom = sampleBloom(uvBarrel, px, bloomRadius);
    
    // Apply threshold and tint
    bloom = max(bloom - vec3(bloomThreshold), vec3(0.0)) * bloomTint;
    
    // Add bloom to original color
    col += bloom * bloomIntensity;

    float grain = fract(sin(dot(uv * resolution + time * 57.0, vec2(12.9898, 78.233))) * 43758.5453);
    col += (grain - 0.5) * 0.02;

    float vig = smoothstep(0.55, 0.9, dist);
    col *= mix(1.0, 0.85, vig);

    float pulse = 0.95 + 0.06 * sin(time * 1.7);

    finalColor = vec4(col * pulse, 1.0) * colDiffuse;
}