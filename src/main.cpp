#include "rtweekend.h"
#include "color.h"
#include "camera.h"
#include "hittable_list.h"
#include "aarect.h"
#include "material.h"
#include <iostream>

// Recursive ray bouncing
color ray_color(const ray& r, const hittable& world, int depth) {
    // If we've exceeded the ray bounce limit, no more light is gathered
    if (depth <= 0)
        return color(0, 0, 0);

    hit_record rec;
    
    // If the ray hits nothing, return black
    if (!world.hit(r, 0.001, infinity, rec))
        return color(0, 0, 0); // Background is black in Cornell Box

    ray scattered;
    color attenuation;
    color emitted = rec.mat->emitted();

    // If light hit the diffuse surface, scatter the ray
    if (rec.mat->scatter(r, rec, attenuation, scattered)) {
        return emitted + attenuation * ray_color(scattered, world, depth-1);
    }
    
    // Otherwise, hit the light source and return emitted light
    return emitted;
}

int main() {
    // Image
    const auto aspect_ratio = 1.0;
    const int image_width = 600;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 200;
    const int max_depth = 10;

    // World - Cornell Box
    hittable_list world;
    
    // Materials
    auto red   = make_shared<lambertian>(color(0.65, 0.05, 0.05));
    auto white = make_shared<lambertian>(color(0.73, 0.73, 0.73));
    auto green = make_shared<lambertian>(color(0.12, 0.45, 0.15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    // Cornell Box: 555 units cube
    // Left wall (green)
    world.add(make_shared<yz_rect>(0, 555, 0, 555, 555, green));
    // Right wall (red)
    world.add(make_shared<yz_rect>(0, 555, 0, 555, 0, red));
    // Light (centered on ceiling, smaller than ceiling)
    world.add(make_shared<xz_rect>(213, 343, 227, 332, 554, light));
    // Floor (white)
    world.add(make_shared<xz_rect>(0, 555, 0, 555, 0, white));
    // Ceiling (white)
    world.add(make_shared<xz_rect>(0, 555, 0, 555, 555, white));
    // Back wall (white)
    world.add(make_shared<xy_rect>(0, 555, 0, 555, 555, white));

    // Two boxes (tall and short)
    // Tall box (right side)
    world.add(make_shared<xz_rect>(265, 430, 295, 460, 330, white));  // Top
    world.add(make_shared<xy_rect>(265, 430, 0, 330, 460, white));    // Front
    world.add(make_shared<xy_rect>(265, 430, 0, 330, 295, white));    // Back
    world.add(make_shared<yz_rect>(0, 330, 295, 460, 265, white));    // Left
    world.add(make_shared<yz_rect>(0, 330, 295, 460, 430, white));    // Right

    // Short box (left side)
    world.add(make_shared<xz_rect>(130, 295, 65, 230, 165, white));   // Top
    world.add(make_shared<xy_rect>(130, 295, 0, 165, 230, white));    // Front
    world.add(make_shared<xy_rect>(130, 295, 0, 165, 65, white));     // Back
    world.add(make_shared<yz_rect>(0, 165, 65, 230, 130, white));     // Left
    world.add(make_shared<yz_rect>(0, 165, 65, 230, 295, white));     // Right

    // Camera positioned to view the Cornell Box
    point3 lookfrom(278, 278, -800);
    point3 lookat(278, 278, 0);
    vec3 vup(0, 1, 0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.0;
    auto vfov = 40.0;

    camera cam(lookfrom, lookat, vup, vfov, aspect_ratio);

    // Render
    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height-1; j >= 0; --j) {
        std::clog << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            color pixel_color(0, 0, 0);
            
            // Multiple samples per pixel for antialiasing and noise reduction
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width-1);
                auto v = (j + random_double()) / (image_height-1);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_color(r, world, max_depth);
            }
            
            write_color(std::cout, pixel_color, samples_per_pixel);
        }
    }

    std::clog << "\rDone.                 \n";
}