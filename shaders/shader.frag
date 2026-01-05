#version 410 core

out vec4 fragColor;

uniform vec2 iResolution;
uniform float iTime;
uniform float fov;

#define M_PI 3.14159265358979323846
#define FLT_MAX 3.402823466e+38

vec3 camera = vec3(0.0, 1.0, 0.0);
vec3 sphere_center = vec3(0.0, 0.0, -3.0);
float sphere_radius = 1.0;

struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
};

struct Plane {
    vec3 point;
    vec3 normal;
};

const int NUM_SPHERES = 3;

Sphere spheres[NUM_SPHERES] = Sphere[](
        Sphere(vec3(0.0, 0.0, -3.0), 1.0, vec3(1.0, 0.2, 0.2)),
        Sphere(vec3(2.0, 0.0, -4.0), 1.0, vec3(0.2, 1.0, 0.2)),
        Sphere(vec3(-2.0, 0.0, -4.0), 1.0, vec3(0.2, 0.2, 1.0))
    );

Plane plane = Plane(vec3(0, 0, 0), normalize(vec3(0, 1, 0)));

bool hit_sphere(
    Sphere s,
    vec3 ro,
    vec3 rd,
    float t_min,
    float t_max,
    out float t_hit
) {
    vec3 oc = ro - s.center;

    float a = dot(rd, rd);
    float b = dot(oc, rd);
    float c = dot(oc, oc) - s.radius * s.radius;
    float d = b * b - a * c;

    if (d < 0.0) return false;

    float sqrtd = sqrt(d);
    float t = (-b - sqrtd) / a;
    if (t < t_min || t > t_max) {
        t = (-b + sqrtd) / a;
        if (t < t_min || t > t_max) return false;
    }

    t_hit = t;
    return true;
}

bool hit_plane(
    Plane p,
    vec3 ro,
    vec3 rd,
    float t_min,
    float t_max,
    out float t_hit
) {
    float denom = dot(p.normal, rd);
    if (abs(denom) < 1e-6) return false;

    float t = dot(p.point - ro, p.normal) / denom;
    if (t < t_min || t > t_max) return false;

    t_hit = t;
    return true;
}

vec3 plane_grid_color(vec3 hit_pos) {
    float scale = 1.0;
    vec2 p = hit_pos.xz * scale;

    vec2 g = abs(fract(p) - 0.5);
    float line = min(g.x, g.y);

    float line_width = 0.04;
    float mask = step(line_width, line);

    vec3 base = vec3(0.30);
    vec3 lines = vec3(0.05);
    return mix(lines, base, mask);
}

vec3 trace(vec3 ro, vec3 rd) {
    float closest_t = FLT_MAX;
    vec3 color = vec3(0.05);

    for (int i = 0; i < NUM_SPHERES; ++i) {
        float t;
        if (hit_sphere(spheres[i], ro, rd, 0.001, closest_t, t)) {
            closest_t = t;
            color = spheres[i].color;
        }
    }

    float t_plane;
    if (hit_plane(plane, ro, rd, 0.001, closest_t, t_plane)) {
        vec3 hit_pos = ro + t_plane * rd;
        color = plane_grid_color(hit_pos);
    }

    return color;
}

void main() {
    vec2 uv = (gl_FragCoord.xy / iResolution) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    float z = -1.0 / tan(radians(fov) * 0.5);
    vec3 ray_dir = normalize(vec3(uv, z));

    vec3 col = trace(camera, ray_dir);
    fragColor = vec4(col, 1.0);
}
