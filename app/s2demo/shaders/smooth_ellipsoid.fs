#version 330

in vec3 fragPosition;
in vec3 fragNormal;
in vec4 fragColor;

// Center position of the ellipsoid in World Space
uniform vec3 uCenter;
// Quadric matrix M (upper-left 3x3) of the ellipsoid: p^T M p = 1 in World
// Space, with M including any shear/rotation of the base surface
uniform mat4 uQuadric;
// Directional light vector in World Space pointing towards the light source
uniform vec3 uLightDir;
// Camera/viewer position in World Space for specular highlight calculation
uniform vec3 uViewPos;
// Surface opacity (1.0 opaque, < 1.0 translucent)
uniform float uAlpha;

out vec4 finalColor;

void main()
{
    vec3 N;
    vec3 p = fragPosition - uCenter;

    // General ellipsoid: gradient of p^T M p is 2 M p, whose direction is the
    // surface normal. This covers spheres (M = (1/r^2) I) and sheared ellipsoids.
    vec3 Mp = vec3(dot(uQuadric[0].xyz, p),
                   dot(uQuadric[1].xyz, p),
                   dot(uQuadric[2].xyz, p));
    N = normalize(Mp);

    // Directional light vector (pointing towards light source in World Space)
    vec3 lightDir = normalize(uLightDir);
    float diff = max(dot(N, lightDir), 0.0);
    float ambient = 0.25;

    // View direction towards camera position in World Space
    vec3 viewDir = normalize(uViewPos - fragPosition);
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0) * 0.35;

    vec3 baseColor = fragColor.rgb;
    finalColor = vec4((ambient + diff * 0.75 + spec) * baseColor, fragColor.a * uAlpha);
}
