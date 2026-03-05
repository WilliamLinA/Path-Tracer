#include "rtweekend.h"
#include "color.h"
#include "camera.h"
#include "hittable_list.h"
#include "aarect.h"
#include "material.h"
#include "path_visualizer.h"
#include "mlt_sampler.h"
#include <iostream>

// Forward declaration
void render_mlt(const hittable_list& world, const camera& cam, int image_width, int image_height, 
                int max_depth, int num_chains, int mutations_per_chain, double p_large);

// Recursive ray bouncing with optional path recording and MLT sampler
color ray_color(const ray& r, const hittable& world, int depth, PathRecorder* recorder = nullptr, int path_id = -1, MLTSampler* mlt_sampler = nullptr) {
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
    if (rec.mat->scatter(r, rec, attenuation, scattered, mlt_sampler)) {
        // Record this vertex after scatter (so attenuation is valid)
        if (recorder) {
            recorder->record_vertex(rec.p, rec.normal, attenuation, false);
        }
        return emitted + attenuation * ray_color(scattered, world, depth-1, recorder, path_id, mlt_sampler);
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
    // === Rendering Mode Selection ===
    // Set to true to use MLT, false for standard path tracing
    const bool use_mlt = true;  // Enable MLT rendering
    
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

    // === Choose rendering method ===
    if (use_mlt) {
        // MLT Parameters
        int num_chains = image_width * image_height;  // 360,000 chains (full coverage)
        int mutations_per_chain = 200;  // 200 mutations per chain
        double p_large = 0.3;  // 30% large steps
        
        render_mlt(world, cam, image_width, image_height, max_depth, 
                   num_chains, mutations_per_chain, p_large);
        return 0;
    }
    
    // === Standard Path Tracing ===
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

// MLT rendering function
// Fixed PSSMLT: seed phase for b, f/L contribution weighting, MIS weights, global normalization
void render_mlt(const hittable_list& world, const camera& cam, int image_width, int image_height, 
                int max_depth, int num_chains, int mutations_per_chain, double p_large) {
    
    // Output PPM header FIRST (to stdout)
    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
    
    // Image buffer for accumulation (no per-pixel sample count - we use global normalization)
    std::vector<std::vector<color>> image(image_height, std::vector<color>(image_width, color(0,0,0)));
    
    // === Phase 1: Estimate normalization constant b ===
    // b = E[L(x)] where L is luminance and x is a uniform random path in primary sample space
    const int num_seed_samples = 100000;
    double b = 0.0;
    std::clog << "Phase 1: Estimating normalization constant b (" << num_seed_samples << " seed samples)...\n";
    for (int i = 0; i < num_seed_samples; ++i) {
        double u = random_double();
        double v = random_double();
        ray r = cam.get_ray(u, v);
        color c = ray_color(r, world, max_depth);
        b += c.length();
    }
    b /= num_seed_samples;
    std::clog << "Normalization constant b = " << b << "\n";
    
    if (b < 1e-10) {
        std::clog << "Scene is essentially black, outputting black image.\n";
        for (int j = image_height-1; j >= 0; --j)
            for (int i = 0; i < image_width; ++i)
                write_color(std::cout, color(0, 0, 0), 1);
        return;
    }
    
    // === Phase 2: MLT rendering with Markov chains ===
    // MLT Path Recorder - capture first few chains for mutation visualization
    MLTPathRecorder mlt_recorder(5, 30);
    
    // Statistics
    size_t total_mutations = 0;
    size_t accepted_mutations = 0;
    size_t large_steps = 0;
    size_t accepted_large_steps = 0;
    
    std::clog << "Phase 2: Running " << num_chains << " Markov chains ("
              << mutations_per_chain << " mutations each, p_large=" << p_large << ")...\n";
    
    // For each Markov chain
    for (int chain = 0; chain < num_chains; ++chain) {
        MLTSampler sampler;
        
        // Generate initial path (seed)
        sampler.start_iteration();
        double u = sampler.next();
        double v = sampler.next();
        ray r = cam.get_ray(u, v);
        
        // Always record seed path geometry (cheap; only kept if chain is selected)
        PathRecorder temp_seed_recorder(1);
        temp_seed_recorder.start_path();
        temp_seed_recorder.record_vertex(r.origin(), vec3(0,0,1), color(1,1,1), false);
        
        color current_color = ray_color(r, world, max_depth, &temp_seed_recorder, -1, &sampler);
        double current_luminance = current_color.length();
        temp_seed_recorder.end_path(current_color);
        
        int current_x = static_cast<int>(u * image_width);
        int current_y = static_cast<int>(v * image_height);
        if (current_x >= image_width) current_x = image_width - 1;
        if (current_y >= image_height) current_y = image_height - 1;
        if (current_x < 0) current_x = 0;
        if (current_y < 0) current_y = 0;
        
        sampler.accept();
        
        // Check if we should record this chain for visualization
        // Only record chains with non-zero seed luminance (interesting chains)
        bool record_chain = false;
        if (mlt_recorder.should_record_chain() && current_luminance > 0) {
            record_chain = true;
            mlt_recorder.start_chain(chain);
        }
        
        // Track current path geometry for recorder
        LightPath current_path_geom;
        if (record_chain && !temp_seed_recorder.get_paths().empty()) {
            current_path_geom = temp_seed_recorder.get_paths().back();
            mlt_recorder.record_seed(current_path_geom, current_color, current_luminance);
        }
        
        // Progress reporting (only to stderr, less frequently)
        if (chain % 5000 == 0 && chain > 0) {
            std::clog << "\rProgress: " << (100 * chain / num_chains) << "% | Acceptance: " 
                      << (total_mutations > 0 ? 100.0 * accepted_mutations / total_mutations : 0) 
                      << "%" << std::flush;
        }
        
        // MLT mutation loop
        for (int mutation = 0; mutation < mutations_per_chain; ++mutation) {
            total_mutations++;
            
            // Decide large or small step
            bool large_step = (random_double() < p_large);
            if (large_step) large_steps++;
            
            sampler.set_large_step(large_step);
            sampler.start_iteration();
            
            // Generate proposed path with mutated random numbers
            double u_prop = sampler.next();
            double v_prop = sampler.next();
            ray r_prop = cam.get_ray(u_prop, v_prop);
            
            // Optionally record proposed path geometry
            PathRecorder temp_prop_recorder(1);
            PathRecorder* prop_rec = nullptr;
            if (record_chain && mlt_recorder.is_recording()) {
                temp_prop_recorder.start_path();
                temp_prop_recorder.record_vertex(r_prop.origin(), vec3(0,0,1), color(1,1,1), false);
                prop_rec = &temp_prop_recorder;
            }
            
            color proposed_color = ray_color(r_prop, world, max_depth, prop_rec, -1, &sampler);
            double proposed_luminance = proposed_color.length();
            
            if (prop_rec) {
                temp_prop_recorder.end_path(proposed_color);
            }
            
            int proposed_x = static_cast<int>(u_prop * image_width);
            int proposed_y = static_cast<int>(v_prop * image_height);
            if (proposed_x >= image_width) proposed_x = image_width - 1;
            if (proposed_y >= image_height) proposed_y = image_height - 1;
            if (proposed_x < 0) proposed_x = 0;
            if (proposed_y < 0) proposed_y = 0;
            
            // Metropolis-Hastings acceptance probability
            double acceptance_prob = (current_luminance > 0) ? 
                std::min(1.0, proposed_luminance / current_luminance) : 1.0;
            
            // === KEY FIX: MIS-weighted f/L contribution ===
            // The Markov chain samples proportional to luminance L(x).
            // Each deposit must be f(x)/L(x) to cancel the luminance bias.
            // MIS weights: proposed gets weight a, current gets weight (1-a).
            if (proposed_luminance > 0) {
                color prop_contrib = proposed_color * (acceptance_prob / proposed_luminance);
                image[proposed_y][proposed_x] = image[proposed_y][proposed_x] + prop_contrib;
            }
            if (current_luminance > 0) {
                color curr_contrib = current_color * ((1.0 - acceptance_prob) / current_luminance);
                image[current_y][current_x] = image[current_y][current_x] + curr_contrib;
            }
            
            // Accept or reject
            bool accepted = (random_double() < acceptance_prob);
            
            // Record mutation for visualization
            if (record_chain && mlt_recorder.is_recording()) {
                MLTMutationRecord mut_rec;
                if (!temp_prop_recorder.get_paths().empty()) {
                    mut_rec.proposed_path = temp_prop_recorder.get_paths().back();
                }
                mut_rec.current_path = current_path_geom;
                mut_rec.proposed_color = proposed_color;
                mut_rec.current_color = current_color;
                mut_rec.proposed_luminance = proposed_luminance;
                mut_rec.current_luminance = current_luminance;
                mut_rec.acceptance_prob = acceptance_prob;
                mut_rec.accepted = accepted;
                mut_rec.large_step = large_step;
                mut_rec.mutation_index = mutation;
                mut_rec.proposed_x = proposed_x;
                mut_rec.proposed_y = proposed_y;
                mut_rec.current_x = current_x;
                mut_rec.current_y = current_y;
                mlt_recorder.record_mutation(mut_rec);
            }
            
            if (accepted) {
                accepted_mutations++;
                if (large_step) accepted_large_steps++;
                
                // Move to proposed state
                current_color = proposed_color;
                current_luminance = proposed_luminance;
                current_x = proposed_x;
                current_y = proposed_y;
                
                // Update current path geometry for recorder
                if (record_chain && !temp_prop_recorder.get_paths().empty()) {
                    current_path_geom = temp_prop_recorder.get_paths().back();
                }
                
                sampler.accept();
            } else {
                sampler.reject();
            }
        }
        
        if (record_chain) mlt_recorder.end_chain();
    }
    
    // === Output: Global normalization (KEY FIX) ===
    // PSSMLT image estimate: I_pixel = (b * W * H / N_total) * sum(f/L contributions)
    // This replaces the incorrect per-pixel normalization that caused overexposure.
    // Derivation: MCMC samples from p(x) = L(x)/b. The IS estimator is
    //   I = (1/N) * sum[ f(x)/p(x) ] = (b/N) * sum[ f(x)/L(x) ]
    // Multiplying by W*H converts from integral to per-pixel average (matching PT output).
    double normalization = b * image_width * image_height / static_cast<double>(total_mutations);
    std::clog << "\nNormalization: b=" << b << ", N=" << total_mutations 
              << ", scale=" << normalization << "\n";
    
    for (int j = image_height-1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            color final_color = image[j][i] * normalization;
            write_color(std::cout, final_color, 1);
        }
    }
    
    // Export MLT mutation paths for visualization/debugging
    if (!mlt_recorder.get_chains().empty()) {
        std::clog << "Exporting MLT mutation paths to OBJ...\n";
        PathVisualizer::export_mlt_paths_to_obj("cornell_box_mlt_paths.obj",
                                                 mlt_recorder.get_chains(), true);
    }
    
    // Output statistics to stderr (after PPM is complete)
    std::clog << "\n=== MLT Rendering Statistics ===\n";
    std::clog << "Normalization constant b: " << b << "\n";
    std::clog << "Total mutations: " << total_mutations << "\n";
    std::clog << "Accepted: " << accepted_mutations << " (" 
              << (100.0 * accepted_mutations / total_mutations) << "%)\n";
    std::clog << "Large steps: " << large_steps << " (accepted: " << accepted_large_steps 
              << ", " << (large_steps > 0 ? 100.0 * accepted_large_steps / large_steps : 0) << "%)\n";
    std::clog << "Small steps: " << (total_mutations - large_steps) 
              << " (accepted: " << (accepted_mutations - accepted_large_steps) 
              << ", " << ((total_mutations - large_steps) > 0 ? 
                  100.0 * (accepted_mutations - accepted_large_steps) / (total_mutations - large_steps) : 0) << "%)\n";
    std::clog << "Chains: " << num_chains << ", mutations/chain: " << mutations_per_chain << "\n";
    std::clog << "Image: " << image_width << "x" << image_height << "\n";
    std::clog << "===========================\n";
}