#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform vec2 resolution;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 entityColor;
uniform float speed;
uniform float winnerRainbow; // 0.0 = off (normal car), 1.0 = on (winner)
uniform float time;
uniform vec2 uvMin;
uniform vec2 uvMax;
uniform vec2 contentUvMin; // may be unused
uniform vec2 contentUvMax; // may be unused

uniform sampler2D texture1;
uniform sampler2D texture2;

out vec4 finalColor;

bool inSource(vec2 uv)
{
    return uv.x >= uvMin.x && uv.x <= uvMax.x && uv.y >= uvMin.y && uv.y <= uvMax.y;
}

vec3 rainbowColor(float t)
{
    float hue = mod(t, 6.28318);
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

void main()
{
    vec2 uv = fragTexCoord;
    vec4 texColor = texture(texture0, uv);

    // Keep additional samplers alive (engine expects them)
    vec4 _d = texture(texture1, uv) + texture(texture2, uv);
    _d *= 0.0;

    // Discard outside source frame
    if (!inSource(uv)) {
        finalColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    // Cull padded black regions while preserving real black outline of the car
    float luma = max(texColor.r, max(texColor.g, texColor.b));
    if (luma < 0.02) {
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
        if (neighborMax < 0.02) {
            finalColor = vec4(0.0, 0.0, 0.0, 0.0);
            return;
        }
    }

    // Base effect: chromatic aberration by speed (original car shader)
    float speedPercent = clamp(speed, 0.0, 1.0);
    float aberrationStrength = speedPercent * 0.03;
    vec2 shift = (resolution.x > 0.0 && resolution.y > 0.0) ? vec2(aberrationStrength) / resolution : vec2(0.0);
    vec4 colR = texture(texture0, uv + shift);
    vec4 colG = texColor;
    vec4 colB = texture(texture0, uv - shift);
    vec3 combined = vec3(colR.r, colG.g, colB.b);

    // Start with base color tinted by entity and diffuse
    vec3 base_color = combined * entityColor.rgb;

    if (winnerRainbow > 0.5) {
        // Optional pulsing rainbow glow + outline for winners
        float pulse = sin(time * 6.0) * 0.5 + 0.5;
        vec3 rainbow = rainbowColor(time * 2.0);

        // Edge detection
        vec2 texel = 1.0 / vec2(textureSize(texture0, 0));
        float edge = 0.0;
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                vec2 o = uv + vec2(float(i), float(j)) * texel;
                float a = inSource(o) ? texture(texture0, o).a : 0.0;
                if (a < 0.1) { edge = 1.0; }
            }
        }

        // Subtle body glow
        base_color = mix(base_color, base_color + rainbow * 0.4, 0.15 * pulse);

        // Bright rainbow outline
        if (edge > 0.0) {
            vec3 outline = rainbow * (1.2 + pulse * 0.8);
            base_color = mix(base_color, outline, 0.8);
        }
    }

    finalColor = vec4(base_color, texColor.a) * colDiffuse;
}