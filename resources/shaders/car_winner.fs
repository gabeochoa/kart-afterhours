#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float time;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 entityColor;
uniform vec2 uvMin;
uniform vec2 uvMax;
uniform vec2 contentUvMin;
uniform vec2 contentUvMax;

uniform sampler2D texture1;
uniform sampler2D texture2;

out vec4 finalColor;

vec3 rainbowColor(float t)
{
    float hue = mod(t, 6.28318); // 0..2*PI
    if (hue < 2.094) {
        float k = hue / 2.094;           // R->G
        return mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), k);
    } else if (hue < 4.188) {
        float k = (hue - 2.094) / 2.094; // G->B
        return mix(vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0), k);
    } else {
        float k = (hue - 4.188) / 2.094; // B->R
        return mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), k);
    }
}

bool inSource(vec2 uv)
{
    return uv.x >= uvMin.x && uv.x <= uvMax.x && uv.y >= uvMin.y && uv.y <= uvMax.y;
}

void main()
{
    vec2 uv = fragTexCoord;
    vec4 tex = texture(texture0, uv);

    // Keep other samplers alive (engine expects them)
    vec4 _d = texture(texture1, uv) + texture(texture2, uv);
    _d *= 0.0;

    // If outside the source frame, output transparent
    if (!inSource(uv)) {
        finalColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    // Cull padded solid-black regions while preserving black outlines on the car
    float luma = max(tex.r, max(tex.g, tex.b));
    if (luma < 0.02) {
        // Use texel size of spritesheet
        vec2 texel = 1.0 / vec2(textureSize(texture0, 0));
        float neighborMax = 0.0;
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                vec2 nUv = uv + vec2(float(i), float(j)) * texel;
                if (!inSource(nUv)) continue;
                vec3 n = texture(texture0, nUv).rgb;
                neighborMax = max(neighborMax, max(n.r, max(n.g, n.b)));
            }
        }
        // If all neighbors are also near-black, treat as transparent padding
        if (neighborMax < 0.02) {
            finalColor = vec4(0.0, 0.0, 0.0, 0.0);
            return;
        }
    }

    float pulse = sin(time * 6.0) * 0.5 + 0.5;
    vec3 rainbow = rainbowColor(time * 2.0);

    // Texel size from the actual bound texture (spritesheet)
    vec2 texel = 1.0 / vec2(textureSize(texture0, 0));

    // Edge detection on alpha or brightness change; treat out-of-frame as transparent
    float edge = 0.0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            vec2 offsetUv = uv + vec2(float(i), float(j)) * texel;
            float a = inSource(offsetUv) ? texture(texture0, offsetUv).a : 0.0;
            float l = inSource(offsetUv) ? max(texture(texture0, offsetUv).r,
                               max(texture(texture0, offsetUv).g, texture(texture0, offsetUv).b)) : 0.0;
            if (a < 0.1 || abs(l - luma) > 0.2) { edge = 1.0; }
        }
    }

    // Fallback tint: if entityColor is not set (often 0), use white
    float tintMag = entityColor.r + entityColor.g + entityColor.b;
    vec3 tint = (tintMag > 0.001) ? entityColor.rgb : vec3(1.0);

    // Base is the sprite tinted by tint
    vec3 base_color = tex.rgb * tint;

    // Subtle body glow
    base_color = mix(base_color, base_color + rainbow * 0.4, 0.15 * pulse);

    // Bright pulsing rainbow outline only on edges
    if (edge > 0.0) {
        vec3 outline = rainbow * (1.2 + pulse * 0.8);
        base_color = mix(base_color, outline, 0.8);
    }

    finalColor = vec4(base_color, tex.a) * colDiffuse;
} 