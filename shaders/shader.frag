#version 410 core

struct Camera {
    vec3 position;
    vec3 direction;
    float fov;
};

out vec4 fragColor;

uniform vec2 iResolution;
uniform float iTime;
uniform Camera u_camera;

#define M_PI 3.14159265358979323846
#define FLT_MAX 3.402823466e+38

struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
};

struct Plane {
    vec3 point;
    vec3 normal;
    vec3 color;
};

struct HitRecord {
    vec3 point;
    vec3 normal;
    float t;
    vec3 color;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

const int NUM_SPHERES = 3;

Sphere spheres[NUM_SPHERES] = Sphere[](
        Sphere(vec3(0.0, 1.0, -3.0), 1.0, vec3(1.0, 0.2, 0.2)),
        Sphere(vec3(2.0, 1.0, -4.0), 1.0, vec3(0.2, 1.0, 0.2)),
        Sphere(vec3(-2.0, 1.0, -4.0), 1.0, vec3(0.2, 0.2, 1.0))
    );

Plane plane = Plane(vec3(0, 0, 0), normalize(vec3(0, 1, 0)), vec3(0.9));

bool hit_sphere(Sphere s, Ray ray, float t_min, float t_max, out float t_hit, out HitRecord record) {
    vec3 oc = ray.origin - s.center;
    float a = dot(ray.direction, ray.direction);
    float b = dot(oc, ray.direction);
    float c = dot(oc, oc) - s.radius * s.radius;
    float d = b * b - a * c;

    if (d < 0.0) return false;

    float sqrtd = sqrt(d);
    float t = (-b - sqrtd) / a;
    if (t < t_min || t > t_max) {
        t = (-b + sqrtd) / a;
        if (t < t_min || t > t_max) return false;
    }

    record.t = t;
    record.point = ray.origin + t * ray.direction;
    record.normal = (record.point - s.center) * (1.0f / s.radius);
    record.color = s.color;

    t_hit = t;
    return true;
}

bool hit_plane(Plane p, Ray ray, float t_min, float t_max, out float t_hit, out HitRecord record) {
    float denom = dot(p.normal, ray.direction);
    if (abs(denom) < 1e-6) return false;
    float t = dot(p.point - ray.origin, p.normal) / denom;
    if (t < t_min || t > t_max) return false;
    record.t = t;
    record.point = ray.origin + t * ray.direction;
    record.normal = p.normal;
    record.color = p.color;

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
    vec3 base = vec3(0.80);
    vec3 lines = vec3(0.0);
    return mix(lines, base, mask);
}

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec3 random_in_unit_sphere(vec2 rnd_state) {
    vec3 p;

    for (int i = 0; i < 4; ++i) {
        p = vec3(
                random(rnd_state + vec2(1.0, 0.0)),
                random(rnd_state + vec2(0.0, 1.0)),
                random(rnd_state + vec2(1.0, 1.0))
            ) * 2.0 - 1.0;

        if (dot(p, p) < 1.0)
            return p;

        rnd_state += 13.37; // decorrelate on retry
    }

    // fallback to guarantee return
    return normalize(p) * random(rnd_state + 42.0);
}

bool scatter(Ray ray_in, HitRecord record, out vec3 attenuation, out Ray scattered,
    vec2 rnd_state) {
    vec3 target = record.point + record.normal + random_in_unit_sphere(rnd_state);
    scattered = Ray(record.point, target - record.point);
    attenuation = record.color;
    return true;
}

vec3 trace(Ray ray, vec2 rnd_state) {
    Ray cur_ray = ray;
    vec3 cur_attenuation = vec3(1.0, 1.0, 1.0);

    for (int i = 0; i < 50; i++) {
        HitRecord record;
        HitRecord temp_record;

        // hit anything
        float t;
        bool hit_anything = false;
        float closest_t = FLT_MAX;

        for (int i = 0; i < NUM_SPHERES; ++i) {
            if (hit_sphere(spheres[i], cur_ray, 0.001, closest_t, t, temp_record)) {
                closest_t = t;
                hit_anything = true;
                record = temp_record;
            }
        }
        if (hit_plane(plane, cur_ray, 0.001, closest_t, t, temp_record)) {
            vec3 hit_pos = ray.origin + t * ray.direction;
            hit_anything = true;
            record = temp_record;
        }

        if (hit_anything) {
            Ray scattered;
            vec3 attenuation;
            if (scatter(cur_ray, record, attenuation, scattered, rnd_state)) {
                cur_attenuation *= attenuation;
                cur_ray = scattered;
            } else {
                return vec3(0.0, 0.0, 0.0);
            }
        } else {
            vec3 unit_direction = normalize(cur_ray.direction);
            float t = 0.5f * (unit_direction.y + 1.0f);
            vec3 c = (1.0f - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
            return cur_attenuation * c;
        }
    }
    return vec3(0.0, 0.0, 0.0); // exceeded recursion
}

void main() {
    vec2 rnd_state = gl_FragCoord.xy / iResolution.xy * iTime;

    vec2 uv = (gl_FragCoord.xy / iResolution) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    // setup camera basis (rotation)
    vec3 world_up = vec3(0.0, 1.0, 0.0);
    vec3 fwd = normalize(u_camera.direction);
    vec3 right = normalize(cross(fwd, world_up));
    vec3 up = normalize(cross(right, fwd));
    mat3 camera_rotation = mat3(right, up, -fwd);

    // get ray dir
    // Use the struct's fov
    float z = -1.0 / tan(radians(u_camera.fov) * 0.5);
    vec3 local_ray_dir = normalize(vec3(uv, z));
    // rotate into world space
    vec3 ray_dir = camera_rotation * local_ray_dir;

    // finally, trace ray
    vec3 col = trace(Ray(u_camera.position, ray_dir), rnd_state);
    fragColor = vec4(col, 1.0);
}
