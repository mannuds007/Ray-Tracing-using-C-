#include <tuple>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>

// Vector class definition
struct vec3 {
    float x = 0, y = 0, z = 0;
    float& operator[](const int i) { return i == 0 ? x : (1 == i ? y : z); }
    const float& operator[](const int i) const { return i == 0 ? x : (1 == i ? y : z); }
    vec3 operator*(const float v) const { return { x * v, y * v, z * v }; }
    float operator*(const vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    vec3 operator+(const vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    vec3 operator-(const vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    vec3 operator-() const { return { -x, -y, -z }; }
    float norm() const { return std::sqrt(x * x + y * y + z * z); }
    vec3 normalized() const { return (*this) * (1.f / norm()); }
};

// Cross product function
vec3 cross(const vec3 v1, const vec3 v2) {
    return { v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x };
}

// Material structure definition
struct Material {
    float refractive_index = 1;
    float albedo[4] = { 2, 0, 0, 0 };
    vec3 diffuse_color = { 0, 0, 0 };
    float specular_exponent = 0;
};

// Sphere structure definition
struct Sphere {
    vec3 center;
    float radius;
    Material material;
};

// Cube structure definition
struct Cube {
    vec3 center;
    float size;
    Material material;
};

// Material definitions
constexpr Material marble = { 1.0, {0.8, 0.2, 0.0, 0.0}, {0.5, 0.5, 0.5}, 30. };
constexpr Material water = { 1.3, {0.1, 0.4, 0.7, 0.5}, {0.2, 0.5, 0.8}, 100. };
constexpr Material shiny_red = { 1.0, {1.2, 0.3, 0.0, 0.1}, {0.7, 0.1, 0.1}, 200. };
constexpr Material bronze = { 1.0, {0.4, 0.3, 0.2, 0.1}, {0.8, 0.7, 0.5}, 500. };

// Scene objects
constexpr Sphere spheres[] = {
    {{-2, 1, -15}, 1.5, marble},
    {{ 0, 4, -12}, 2.0, water},
    {{ 2, 0, -18}, 2.5, shiny_red},
    {{ 5, 3, -20}, 3.5, bronze}
};

constexpr Cube cube = { {0, -1, -10}, 2.0, water};

// Light positions
constexpr vec3 lights[] = {
    {-15, 10, 25},
    { 20, 30, -30},
    { 10, 10, 15}
};

// Reflect and refract functions
vec3 reflect(const vec3& I, const vec3& N) {
    return I - N * 2.f * (I * N);
}

vec3 refract(const vec3& I, const vec3& N, const float eta_t, const float eta_i = 1.f) {
    float cosi = -std::max(-1.f, std::min(1.f, I * N));
    if (cosi < 0) return refract(I, -N, eta_i, eta_t);
    float eta = eta_i / eta_t;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? vec3{1, 0, 0} : I * eta + N * (eta * cosi - std::sqrt(k));
}

// Ray-sphere intersection
std::tuple<bool, float> ray_sphere_intersect(const vec3& orig, const vec3& dir, const Sphere& s) {
    vec3 L = s.center - orig;
    float tca = L * dir;
    float d2 = L * L - tca * tca;
    if (d2 > s.radius * s.radius) return { false, 0 };
    float thc = std::sqrt(s.radius * s.radius - d2);
    float t0 = tca - thc, t1 = tca + thc;
    if (t0 > .001) return { true, t0 };
    if (t1 > .001) return { true, t1 };
    return { false, 0 };
}

// Ray-cube intersection
std::tuple<bool, float, vec3> ray_cube_intersect(const vec3& orig, const vec3& dir, const Cube& c) {
    float tmin = (c.center.x - c.size / 2 - orig.x) / dir.x;
    float tmax = (c.center.x + c.size / 2 - orig.x) / dir.x;
    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (c.center.y - c.size / 2 - orig.y) / dir.y;
    float tymax = (c.center.y + c.size / 2 - orig.y) / dir.y;
    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return { false, 0, vec3{0, 0, 0} };

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (c.center.z - c.size / 2 - orig.z) / dir.z;
    float tzmax = (c.center.z + c.size / 2 - orig.z) / dir.z;
    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return { false, 0, vec3{0, 0, 0} };

    if (tzmin > tmin)
        tmin = tzmin;

    if (tzmax < tmax)
        tmax = tzmax;

    vec3 N;
    if (tmin > 0) {
        N = vec3{
            (tmin == (c.center.x - c.size / 2 - orig.x) / dir.x || tmin == (c.center.x + c.size / 2 - orig.x) / dir.x) ? 1.0f : 0.0f,
            (tmin == (c.center.y - c.size / 2 - orig.y) / dir.y || tmin == (c.center.y + c.size / 2 - orig.y) / dir.y) ? 1.0f : 0.0f,
            (tmin == (c.center.z - c.size / 2 - orig.z) / dir.z || tmin == (c.center.z + c.size / 2 - orig.z) / dir.z) ? 1.0f : 0.0f
        };
        return { true, tmin, N };
    }
    return { false, 0, vec3{0, 0, 0} };
}

// Scene intersection function
std::tuple<bool, vec3, vec3, Material> scene_intersect(const vec3& orig, const vec3& dir) {
    vec3 pt, N;
    Material material;

    float nearest_dist = 1e10;
    if (std::abs(dir.y) > .001) {
        float d = -(orig.y + 3) / dir.y;
        vec3 p = orig + dir * d;
        if (d > .001 && d < nearest_dist && std::abs(p.x) < 12 && p.z < -12 && p.z > -28) {
            nearest_dist = d;
            pt = p;
            N = { 0, 1, 0 };
            material.diffuse_color = (int(.5 * pt.x + 1000) + int(.5 * pt.z)) & 1 ? vec3{.4, .4, .4} : vec3{.4, .3, .2};
        }
    }

    for (const Sphere& s : spheres) {
        auto [intersection, d] = ray_sphere_intersect(orig, dir, s);
        if (!intersection || d > nearest_dist) continue;
        nearest_dist = d;
        pt = orig + dir * nearest_dist;
        N = (pt - s.center).normalized();
        material = s.material;
    }

    auto [cube_hit, cube_dist, cube_norm] = ray_cube_intersect(orig, dir, cube);
    if (cube_hit && cube_dist < nearest_dist) {
        nearest_dist = cube_dist;
        pt = orig + dir * nearest_dist;
        N = cube_norm;
        material = cube.material;
    }

    return { nearest_dist < 1000, pt, N, material };
}

// Cast ray function
vec3 cast_ray(const vec3& orig, const vec3& dir, const int depth = 0) {
    auto [hit, point, N, material] = scene_intersect(orig, dir);
    if (depth > 4 || !hit)
        return { 0.2, 0.7, 0.8 };

    vec3 reflect_dir = reflect(dir, N).normalized();
    vec3 refract_dir = refract(dir, N, material.refractive_index).normalized();
    vec3 reflect_color = cast_ray(point, reflect_dir, depth + 1);
    vec3 refract_color = cast_ray(point, refract_dir, depth + 1);

    float diffuse_light_intensity = 0, specular_light_intensity = 0;
    for (const vec3& light : lights) {
        vec3 light_dir = (light - point).normalized();
        auto [hit, shadow_pt, trashnrm, trashmat] = scene_intersect(point, light_dir);
        if (hit && (shadow_pt - point).norm() < (light - point).norm()) continue;
        diffuse_light_intensity += std::max(0.f, light_dir * N);
        specular_light_intensity += std::pow(std::max(0.f, -reflect(-light_dir, N) * dir), material.specular_exponent);
    }
    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + vec3{1., 1., 1.} * specular_light_intensity * material.albedo[1] + reflect_color * material.albedo[2] + refract_color * material.albedo[3];
}

int main() {
    constexpr int width = 1024;
    constexpr int height = 768;
    constexpr float fov = 1.05;
    std::vector<vec3> framebuffer(width * height);
#pragma omp parallel for
    for (int pix = 0; pix < width * height; pix++) {
        float dir_x = (pix % width + 0.5) - width / 2.;
        float dir_y = -(pix / width + 0.5) + height / 2.;
        float dir_z = -height / (2. * tan(fov / 2.));
        framebuffer[pix] = cast_ray(vec3{ 0, 0, 0 }, vec3{ dir_x, dir_y, dir_z }.normalized());
    }

    std::ofstream ofs;
    ofs.open("out.ppm", std::ofstream::out | std::ofstream::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (vec3& color : framebuffer) {
        float max = std::max(1.f, std::max(color[0], std::max(color[1], color[2])));
        for (int chan : { 0, 1, 2 })
            ofs << (char)(255 * color[chan] / max);
    }
    return 0;
}
