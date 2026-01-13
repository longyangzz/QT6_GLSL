#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uProjection;
uniform mat4 uView;
uniform int uRenderMode;
uniform float uMinZ;
uniform float uMaxZ;

out vec3 vColor;

vec3 elevationColor(float t) {
    vec3 c0 = vec3(0.0, 0.0, 0.5);
    vec3 c1 = vec3(0.0, 0.5, 0.0);
    vec3 c2 = vec3(0.8, 0.8, 0.0);
    vec3 c3 = vec3(1.0, 1.0, 1.0);
    if (t < 0.25) return mix(c0, c1, t * 4.0);
    else if (t < 0.5) return mix(c1, c2, (t - 0.25) * 4.0);
    else if (t < 0.75) return mix(c2, c3, (t - 0.5) * 4.0);
    else return c3;
}

void main()
{
    gl_Position = uProjection * uView * vec4(aPos, 1.0);
    gl_PointSize = 3.0;

    if (uRenderMode == 0) {
        float t = (aPos.z - uMinZ) / (uMaxZ - uMinZ + 1e-6);
        t = clamp(t, 0.0, 1.0);
        vColor = elevationColor(t);
    } else {
        vColor = aColor;
    }
}
