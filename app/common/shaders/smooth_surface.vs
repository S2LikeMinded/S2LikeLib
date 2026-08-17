#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec4 vertexColor;

out vec3 fragPosition;
out vec3 fragNormal;
out vec4 fragColor;

// Model-View-Projection matrix supplied automatically by Raylib (View * Projection)
uniform mat4 mvp;
// Custom Model transformation matrix (position, rotation, scale) supplied from app
uniform mat4 uModel;

void main()
{
    vec4 worldPos = uModel * vec4(vertexPosition, 1.0);
    fragPosition = vec3(worldPos);

    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    fragNormal = normalize(normalMatrix * vertexNormal);
    fragColor = vertexColor;

    gl_Position = mvp * worldPos;
}
