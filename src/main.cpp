#include "rtweekend.h"
#include "color.h"
#include "camera.h"
#include "hittable_list.h"
#include "aarect.h"
#include "material.h"
#include "path_visualizer.h"
#include <iostream>

// Recursive ray bouncing with optional path recording
color ray_color(const ray& r, const hittable& world, int depth, PathRecorder* recorder = nullptr, int path_id = -1) {
    // If we've exceeded the ray bounce limit, no more light is gathered
    if (depth <= 0)
        return color(0, 0, 0);

    hit_record rec;
    
    // Debug for first few paths
    bool should_debug = (recorder != nullptr && path_id >= 0 && path_id < 3);
    
    // If the ray hits nothing, return black
    if (!world.hit(r, 0.001, infinity, rec)) {
        if (should_debug) {
            std::clog << "    No hit! Ray origin: (" << r.origin().x() << ", " << r.origin().y() << ", " << r.origin().z() << ")\n";
            std::clog << "             direction: (" << r.direction().x() << ", " << r.direction().y() << ", " << r.direction().z() << ")\n";
        }
        return color(0, 0, 0); // Background is black in Cornell Box
    }
    
    if (should_debug) {
        std::clog << "    HIT at (" << rec.p.x() << ", " << rec.p.y() << ", " << rec.p.z() << ")\n";
    }

    ray scattered;
    color attenuation;
    color emitted = rec.mat->emitted();

    // If light hit the diffuse surface, scatter the ray
    if (rec.mat->scatter(r, rec, attenuation, scattered)) {
        // Record this vertex after scatter (so attenuation is valid)
        if (recorder) {
            recorder->record_vertex(rec.p, rec.normal, attenuation, false);
        }
        return emitted + attenuation * ray_color(scattered, world, depth-1, recorder, path_id);
    }
    
    // Otherwise, hit the light source - record it
    if (recorder) {
        if (emitted.length_squared() > 0.01) {
            recorder->record_vertex(rec.p, rec.normal, emitted, true);
        }
    }
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
    // Camera at Z=-800 to see full box view
    point3 lookfrom(278, 278, -800);  // Original position for full view
    point3 lookat(278, 278, 0);       // Look at center of box
    vec3 vup(0, 1, 0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.0;
    auto vfov = 35.0; // Narrower FOV to keep rays within bounds at Z=-800

    camera cam(lookfrom, lookat, vup, vfov, aspect_ratio);

    // Create path recorder for visualization
    PathRecorder path_recorder(20); // Record 20 sample paths
    int paths_recorded = 0;
    
    // Pre-generate random sampling positions for better angle distribution
    std::vector<std::pair<int, int>> path_sample_positions;
    for (int k = 0; k < 20; ++k) {
        int rand_i = static_cast<int>(random_double() * image_width);
        int rand_j = static_cast<int>(random_double() * image_height);
        path_sample_positions.push_back({rand_i, rand_j});
    }

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
                
                // Record sample paths from randomly distributed positions
                PathRecorder* rec = nullptr;
                bool should_record = false;
                int position_index = -1;
                
                // Check if current pixel matches any pre-selected random position
                if (paths_recorded < 20) {
                    for (size_t k = 0; k < path_sample_positions.size(); ++k) {
                        if (path_sample_positions[k].first == i && 
                            path_sample_positions[k].second == j) {
                            should_record = true;
                            position_index = k;
                            path_sample_positions.erase(path_sample_positions.begin() + k); // Remove to avoid duplicate
                            break;
                        }
                    }
                }
                
                if (should_record) {
                    path_recorder.start_path();
                    // Record camera position as starting point
                    point3 cam_pos = r.origin();
                    path_recorder.record_vertex(cam_pos, vec3(0,0,1), color(1,1,1), false);
                    rec = &path_recorder;
                    
                    // Debug: print first few paths info
                    if (paths_recorded < 3) {
                        std::clog << "\nPath " << paths_recorded << " - Camera: (" 
                                  << cam_pos.x() << ", " << cam_pos.y() << ", " << cam_pos.z() << ")\n";
                        std::clog << "  Pixel: (" << i << ", " << j << ")\n";
                        std::clog << "  Ray direction: (" << r.direction().x() << ", " 
                                  << r.direction().y() << ", " << r.direction().z() << ")\n";
                    }
                    
                    paths_recorded++;
                }
                
                color ray_contrib = ray_color(r, world, max_depth, rec, paths_recorded-1);
                pixel_color += ray_contrib;
                
                if (rec) {
                    path_recorder.end_path(ray_contrib);
                    
                    // Debug: print path depth
                    if (paths_recorded <= 3) {
                        auto& last_path = path_recorder.get_paths().back();
                        std::clog << "  Path depth: " << last_path.depth 
                                  << ", vertices: " << last_path.vertices.size() << "\n";
                    }
                }
            }
            
            write_color(std::cout, pixel_color, samples_per_pixel);
        }
    }

    std::clog << "\rDone.                 \n";
    
    // Export paths to OBJ file
    std::clog << "Exporting " << paths_recorded << " paths to OBJ file...\n";
    if (PathVisualizer::export_paths_to_obj("cornell_box_paths.obj", path_recorder.get_paths(), true)) {
        std::clog << "Successfully exported to cornell_box_paths.obj\n";
    } else {
        std::clog << "Failed to export OBJ file\n";
    }
}