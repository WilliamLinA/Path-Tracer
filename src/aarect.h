#ifndef AARECT_H
#define AARECT_H

#include "rtweekend.h"
#include "hittable.h"

// Rectangular Axis-Aligned Planes

// XY Rectangle (perpendicular to Z axis)
class xy_rect : public hittable {
public:
    xy_rect() {}
    xy_rect(double _x0, double _x1, double _y0, double _y1, double _k, shared_ptr<material> mat)
        : x0(_x0), x1(_x1), y0(_y0), y1(_y1), k(_k), mp(mat) {}

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override;

public:
    shared_ptr<material> mp;
    double x0, x1, y0, y1, k;
};

// XZ Rectangle (perpendicular to Y axis)
class xz_rect : public hittable {
public:
    xz_rect() {}
    xz_rect(double _x0, double _x1, double _z0, double _z1, double _k, shared_ptr<material> mat)
        : x0(_x0), x1(_x1), z0(_z0), z1(_z1), k(_k), mp(mat) {}

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override;

public:
    shared_ptr<material> mp;
    double x0, x1, z0, z1, k;
};

// YZ Rectangle (perpendicular to X axis)
class yz_rect : public hittable {
public:
    yz_rect() {}
    yz_rect(double _y0, double _y1, double _z0, double _z1, double _k, shared_ptr<material> mat)
        : y0(_y0), y1(_y1), z0(_z0), z1(_z1), k(_k), mp(mat) {}

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override;

public:
    shared_ptr<material> mp;
    double y0, y1, z0, z1, k;
};

// Implementation
bool xy_rect::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
    auto t = (k - r.origin().z()) / r.direction().z();
    if (t < t_min || t > t_max)
        return false;
    
    auto x = r.origin().x() + t*r.direction().x();
    auto y = r.origin().y() + t*r.direction().y();
    if (x < x0 || x > x1 || y < y0 || y > y1)
        return false;
    
    rec.t = t;
    rec.p = r.at(t);
    auto outward_normal = vec3(0, 0, 1);
    rec.set_face_normal(r, outward_normal);
    rec.mat = mp;
    return true;
}

bool xz_rect::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
    auto t = (k - r.origin().y()) / r.direction().y();
    if (t < t_min || t > t_max)
        return false;
    
    auto x = r.origin().x() + t*r.direction().x();
    auto z = r.origin().z() + t*r.direction().z();
    if (x < x0 || x > x1 || z < z0 || z > z1)
        return false;
    
    rec.t = t;
    rec.p = r.at(t);
    auto outward_normal = vec3(0, 1, 0);
    rec.set_face_normal(r, outward_normal);
    rec.mat = mp;
    return true;
}

bool yz_rect::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
    auto t = (k - r.origin().x()) / r.direction().x();
    if (t < t_min || t > t_max)
        return false;
    
    auto y = r.origin().y() + t*r.direction().y();
    auto z = r.origin().z() + t*r.direction().z();
    if (y < y0 || y > y1 || z < z0 || z > z1)
        return false;
    
    rec.t = t;
    rec.p = r.at(t);
    auto outward_normal = vec3(1, 0, 0);
    rec.set_face_normal(r, outward_normal);
    rec.mat = mp;
    return true;
}

#endif
