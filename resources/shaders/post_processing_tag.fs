#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float time;
uniform vec2 resolution;
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform sampler2D texture1;
uniform sampler2D texture2;

// Tagger spotlight controls
uniform float spotlightEnabled; // 0.0 off, 1.0 on
uniform vec2 spotlightPos;      // UV position [0,1]
uniform float spotlightRadius;  // inner radius in UV units
uniform float spotlightSoftness;// edge feather in UV units
uniform float dimAmount;        // how much to dim outside [0,1]
uniform float desaturateAmount; // how much to desaturate outside [0,1]

out vec4 finalColor;

bool inBounds(vec2 p) {
    return all(greaterThanEqual(p, vec2(0.0))) && all(lessThanEqual(p, vec2(1.0)));
}

vec4 safeTex0(vec2 p) {
    if (!inBounds(p)) return vec4(0.0);
    return texture(texture0, p);
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

    float threshold = 0.72;
    vec3 bright = max(col - vec3(threshold), vec3(0.0));

    vec2 px = 1.0 / max(resolution, vec2(1.0));
    vec3 bloom = vec3(0.0);
    bloom += safeTex0(uvBarrel + vec2( 1.0,  0.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2(-1.0,  0.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2( 0.0,  1.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2( 0.0, -1.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2( 1.0,  1.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2(-1.0,  1.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2( 1.0, -1.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2(-1.0, -1.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2( 2.0,  0.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2(-2.0,  0.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2( 0.0,  2.0) * px).rgb;
    bloom += safeTex0(uvBarrel + vec2( 0.0, -2.0) * px).rgb;
    bloom /= 12.0;
    bloom = max(bloom - vec3(threshold), vec3(0.0));

    col += bloom * 0.55;

    float grain = fract(sin(dot(uv * resolution + time * 57.0, vec2(12.9898, 78.233))) * 43758.5453);
    col += (grain - 0.5) * 0.02;

    float vig = smoothstep(0.55, 0.9, dist);
    col *= mix(1.0, 0.85, vig);

    float pulse = 0.95 + 0.06 * sin(time * 1.7);

    // Apply Tagger spotlight if enabled
    if (spotlightEnabled > 0.5) {
        // Warp the spotlight position with the same barrel function
        vec2 centeredSpot = spotlightPos - 0.5;
        float r2Spot = dot(centeredSpot, centeredSpot);
        float k2 = 0.08;
        vec2 spotBarrel = 0.5 + centeredSpot * (1.0 + k2 * r2Spot);
        // Account for aspect ratio so the circle appears round on screen
        vec2 aspect = vec2(max(resolution.x / max(resolution.y, 1.0), 1.0), 1.0);
        float d = length((uvBarrel - spotBarrel) * aspect);
        float edge0 = spotlightRadius;
        float edge1 = spotlightRadius + max(spotlightSoftness, 1e-5);
        float outsideMask = smoothstep(edge0, edge1, d);

        float gray = dot(col, vec3(0.299, 0.587, 0.114));
        vec3 desat = mix(col, vec3(gray), clamp(desaturateAmount, 0.0, 1.0));
        vec3 dimmed = desat * (1.0 - clamp(dimAmount, 0.0, 1.0));
        col = mix(col, dimmed, outsideMask);
    }

    finalColor = vec4(col * pulse, 1.0) * colDiffuse;
}

