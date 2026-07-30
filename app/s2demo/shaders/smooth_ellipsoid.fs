#version 330

in vec3 fragPosition;
in vec3 fragNormal;
in vec4 fragColor;

// Center position of the ellipsoid in World Space
uniform vec3 uCenter;
// Semi-principal axes lengths (a, b, c) of the ellipsoid
uniform vec3 uSemiAxes;
// Flag: 1 if the ellipsoid is a sphere (a == b == c), 0 otherwise
uniform int uIsSphere;
// Directional light vector in World Space pointing towards the light source
uniform vec3 uLightDir;
// Camera/viewer position in World Space for specular highlight calculation
uniform vec3 uViewPos;

out vec4 finalColor;

void main()
{
    vec3 N;
    vec3 p = fragPosition - uCenter;

    if (uIsSphere != 0)
    {
        // Special case for sphere: Smooth per-pixel radial normal
        N = normalize(p);
    }
    else
    {
        // Triaxial ellipsoid: Gradient of x^2/a^2 + y^2/b^2 + z^2/c^2 = 1
        // Normal N = normalize(x/a^2, y/b^2, z/c^2)
        vec3 invAxesSq = vec3(1.0 / (uSemiAxes.x * uSemiAxes.x),
                              1.0 / (uSemiAxes.y * uSemiAxes.y),
                              1.0 / (uSemiAxes.z * uSemiAxes.z));
        N = normalize(p * invAxesSq);
    }

    // Directional light vector (pointing towards light source in World Space)
    vec3 lightDir = normalize(uLightDir);
    float diff = max(dot(N, lightDir), 0.0);
    float ambient = 0.25;

    // View direction towards camera position in World Space
    vec3 viewDir = normalize(uViewPos - fragPosition);
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0) * 0.35;

    vec3 baseColor = fragColor.rgb;
    finalColor = vec4((ambient + diff * 0.75 + spec) * baseColor, fragColor.a);
}
