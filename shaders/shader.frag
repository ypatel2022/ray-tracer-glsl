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
uniform sampler2D u_prev_frame;
uniform int u_frame_index;
uniform bool u_use_prev;
uniform vec3 u_sun_direction;
uniform vec3 u_sun_color;
uniform float u_sun_intensity;
uniform vec3 u_sky_color;
uniform float u_sky_intensity;

#define M_PI 3.14159265358979323846
#define FLT_MAX 3.402823466e+38

struct Sphere {
    vec3 center;
    float radius;
    vec3 color;
};

struct Material {
    int type;
    vec3 albedo;
    float roughness;
    float ior;
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
    Material material;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

const int NUM_SPHERES = 4;

const int MAT_LAMBERT = 0;
const int MAT_METAL = 1;
const int MAT_DIELECTRIC = 2;

Material materials[NUM_SPHERES] = Material[](
        Material(MAT_METAL, vec3(1.0, 0.0, 0.2), 0.0, 1.0),
        Material(MAT_METAL, vec3(0.0, 1.0, 0.2), 0.0, 1.0),
        Material(MAT_LAMBERT, vec3(0.2, 0.2, 1.0), 0.0, 1.0),
        Material(MAT_DIELECTRIC, vec3(1.0), 0.0, 1.5)
    );

Sphere spheres[NUM_SPHERES] = Sphere[](
        Sphere(vec3(0.0, 1.0, -3.0), 1.0, vec3(1.0)),
        Sphere(vec3(2.0, 1.0, -4.0), 1.0, vec3(1.0)),
        Sphere(vec3(-2.0, 1.0, -4.0), 1.0, vec3(1.0)),
        Sphere(vec3(0.0, 1.0, -6.0), 1.0, vec3(1.0))
    );

Plane plane = Plane(vec3(0, 0, 0), normalize(vec3(0, 1, 0)), vec3(0.9));
Material plane_material = Material(MAT_LAMBERT, vec3(1.0), 0.0, 1.0);


bool hit_sphere(Sphere s, Material material, Ray ray, float t_min, float t_max,
    out float t_hit, out HitRecord record) {
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
    record.material = material;

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
    record.material = plane_material;

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

float schlick(float cosine, float ref_idx) {
    float r0 = (1.0 - ref_idx) / (1.0 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow(1.0 - cosine, 5.0);
}

bool scatter_lambert(HitRecord record, out vec3 attenuation, out Ray scattered,
    vec2 rnd_state) {
    vec3 target = record.point + record.normal + random_in_unit_sphere(rnd_state);
    scattered = Ray(record.point, target - record.point);
    attenuation = record.material.albedo;
    return true;
}

bool scatter_metal(Ray ray_in, HitRecord record, out vec3 attenuation,
    out Ray scattered, vec2 rnd_state) {
    vec3 reflected = reflect(normalize(ray_in.direction), record.normal);
    vec3 roughness_dir = record.material.roughness * random_in_unit_sphere(rnd_state);
    scattered = Ray(record.point, reflected + roughness_dir);
    attenuation = record.material.albedo;
    return dot(scattered.direction, record.normal) > 0.0;
}

bool scatter_dielectric(Ray ray_in, HitRecord record, out vec3 attenuation,
    out Ray scattered, vec2 rnd_state) {
    attenuation = vec3(1.0);
    vec3 unit_dir = normalize(ray_in.direction);

    float cos_theta = min(dot(-unit_dir, record.normal), 1.0);
    float sin_theta = sqrt(max(0.0, 1.0 - cos_theta * cos_theta));

    float eta = record.material.ior;
    vec3 outward_normal = record.normal;
    float refraction_ratio = 1.0 / eta;
    if (dot(unit_dir, record.normal) > 0.0) {
        outward_normal = -record.normal;
        refraction_ratio = eta;
        cos_theta = min(dot(-unit_dir, outward_normal), 1.0);
    }

    bool cannot_refract = refraction_ratio * sin_theta > 1.0;
    float reflect_prob = schlick(cos_theta, refraction_ratio);

    if (cannot_refract || random(rnd_state) < reflect_prob) {
        vec3 reflected = reflect(unit_dir, record.normal);
        scattered = Ray(record.point, reflected);
    } else {
        vec3 refracted = refract(unit_dir, outward_normal, refraction_ratio);
        scattered = Ray(record.point, refracted);
    }

    return true;
}

bool scatter(Ray ray_in, HitRecord record, out vec3 attenuation, out Ray scattered,
    vec2 rnd_state) {
    if (record.material.type == MAT_METAL) {
        return scatter_metal(ray_in, record, attenuation, scattered, rnd_state);
    }
    if (record.material.type == MAT_DIELECTRIC) {
        return scatter_dielectric(ray_in, record, attenuation, scattered,
            rnd_state);
    }

    return scatter_lambert(record, attenuation, scattered, rnd_state);
}

vec3 trace(Ray ray, vec2 rnd_state) {
    Ray cur_ray = ray;
    vec3 cur_attenuation = vec3(1.0, 1.0, 1.0);
    vec3 radiance = vec3(0.0);

    for (int i = 0; i < 50; i++) {
        HitRecord record;
        HitRecord temp_record;

        // hit anything
        float t;
        bool hit_anything = false;
        float closest_t = FLT_MAX;

        for (int i = 0; i < NUM_SPHERES; ++i) {
            if (hit_sphere(spheres[i], materials[i], cur_ray, 0.001, closest_t,
                t, temp_record)) {
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
            vec3 direct = vec3(0.0);
            vec3 sun_dir = normalize(u_sun_direction);
            Ray shadow_ray = Ray(record.point + record.normal * 0.001, sun_dir);
            bool in_shadow = false;

            for (int i = 0; i < NUM_SPHERES; ++i) {
                if (hit_sphere(spheres[i], materials[i], shadow_ray, 0.001,
                    FLT_MAX, t, temp_record)) {
                    in_shadow = true;
                    break;
                }
            }
            if (!in_shadow &&
                hit_plane(plane, shadow_ray, 0.001, FLT_MAX, t, temp_record)) {
                in_shadow = true;
            }

            if (!in_shadow) {
                float n_dot_l = max(dot(record.normal, sun_dir), 0.0);
                direct = record.material.albedo * u_sun_color * u_sun_intensity *
                    n_dot_l;
                radiance += cur_attenuation * direct;
            }

            Ray scattered;
            vec3 attenuation;
            if (scatter(cur_ray, record, attenuation, scattered, rnd_state)) {
                cur_attenuation *= attenuation;
                cur_ray = scattered;
            } else {
                return radiance;
            }
        } else {
            vec3 unit_direction = normalize(cur_ray.direction);
            float t = 0.5f * (unit_direction.y + 1.0f);
            vec3 sky_bottom = vec3(1.0, 1.0, 1.0);
            vec3 sky_top = u_sky_color;
            vec3 c = mix(sky_bottom, sky_top, t) * u_sky_intensity;
            return radiance + cur_attenuation * c;
        }
    }
    return radiance; // exceeded "recursion"
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
    if (u_use_prev && u_frame_index > 1) {
        vec2 prev_uv = gl_FragCoord.xy / iResolution.xy;
        vec3 prev = texture(u_prev_frame, prev_uv).rgb;
        float frame = float(u_frame_index);
        col = (prev * (frame - 1.0) + col) / frame;
    }
    fragColor = vec4(col, 1.0);
}
