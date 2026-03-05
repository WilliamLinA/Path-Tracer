#ifndef MATERIAL_H
#define MATERIAL_H

#include "rtweekend.h"
#include "hittable.h"
#include "mlt_sampler.h"

class material {
public:
    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, MLTSampler* sampler = nullptr) const = 0;
    virtual color emitted() const {
        return color(0, 0, 0);
    }
};

// Diffuse Material
class lambertian : public material {
public:
    lambertian(const color& a) : albedo(a) {}

    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, MLTSampler* sampler = nullptr) const override {
        vec3 scatter_direction;
        
        if (sampler) {
            // Use MLT sampler for deterministic sampling
            scatter_direction = rec.normal + random_unit_vector_mlt(sampler);
        } else {
            // Use standard random sampling
            scatter_direction = rec.normal + random_unit_vector();
        }
        
        // Catch degenerate scatter direction
        if (near_zero(scatter_direction))
            scatter_direction = rec.normal;
        
        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo;
        return true;
    }

public:
    color albedo;

private:
    static vec3 random_unit_vector() {
        auto a = random_double(0, 2*pi);
        auto z = random_double(-1, 1);
        auto r = sqrt(1 - z*z);
        return vec3(r*cos(a), r*sin(a), z);
    }
    
    static vec3 random_unit_vector_mlt(MLTSampler* sampler) {
        auto a = sampler->next() * 2 * pi;
        auto z = sampler->next() * 2 - 1;
        auto r = sqrt(1 - z*z);
        return vec3(r*cos(a), r*sin(a), z);
    }

    static bool near_zero(const vec3& v) {
        const auto s = 1e-8;
        return (fabs(v[0]) < s) && (fabs(v[1]) < s) && (fabs(v[2]) < s);
    }
};

// Emissive Material (Light Source)
class diffuse_light : public material {
public:
    diffuse_light(const color& c) : emit_color(c) {}

    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, MLTSampler* sampler = nullptr) const override {
        return false;
    }

    virtual color emitted() const override {
        return emit_color;
    }

public:
    color emit_color;
};

#endif
