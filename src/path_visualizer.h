#ifndef PATH_VISUALIZER_H
#define PATH_VISUALIZER_H

#include "vec3.h"
#include "color.h"
#include <vector>
#include <fstream>
#include <string>
#include <cmath>

// Structure to store a single vertex in a light path
struct PathVertex {
    point3 position;
    vec3 normal;
    color contribution;
    bool is_light_source;
    
    PathVertex(const point3& pos, const vec3& norm, const color& contrib, bool is_light)
        : position(pos), normal(norm), contribution(contrib), is_light_source(is_light) {}
};

// Structure to store a complete light path
struct LightPath {
    std::vector<PathVertex> vertices;
    color final_color;
    int depth;
    
    LightPath() : final_color(0, 0, 0), depth(0) {}
};

// Path recorder to capture light paths during rendering
class PathRecorder {
private:
    std::vector<LightPath> paths;
    LightPath current_path;
    int max_paths;
    bool recording;
    
public:
    PathRecorder(int max = 50) : max_paths(max), recording(false) {}
    
    void start_path() {
        if (paths.size() < max_paths) {
            current_path = LightPath();
            recording = true;
        }
    }
    
    void record_vertex(const point3& pos, const vec3& normal, const color& contrib, bool is_light) {
        if (recording) {
            current_path.vertices.push_back(PathVertex(pos, normal, contrib, is_light));
            current_path.depth++;
        }
    }
    
    void end_path(const color& final_color) {
        if (recording) {
            current_path.final_color = final_color;
            paths.push_back(current_path);
            recording = false;
        }
    }
    
    const std::vector<LightPath>& get_paths() const { return paths; }
    
    void clear() {
        paths.clear();
        recording = false;
    }
};

// OBJ exporter for visualizing paths in Unity/Blender
class PathVisualizer {
private:
    static void write_cylinder(std::ofstream& obj, const point3& start, const point3& end, 
                              double radius, int& vertex_offset, int sides = 8) {
        vec3 direction = end - start;
        double length = direction.length();
        if (length < 1e-6) return;
        
        vec3 dir = direction / length;
        
        // Create orthonormal basis
        vec3 up = fabs(dir.y()) > 0.9 ? vec3(1, 0, 0) : vec3(0, 1, 0);
        vec3 right = unit_vector(cross(up, dir));
        vec3 forward = cross(dir, right);
        
        // Generate vertices for cylinder
        for (int ring = 0; ring < 2; ++ring) {
            point3 center = ring == 0 ? start : end;
            for (int i = 0; i < sides; ++i) {
                double angle = 2.0 * 3.14159265359 * i / sides;
                vec3 offset = radius * (cos(angle) * right + sin(angle) * forward);
                point3 vertex = center + offset;
                obj << "v " << vertex.x() << " " << vertex.y() << " " << vertex.z() << "\n";
            }
        }
        
        // Generate faces
        for (int i = 0; i < sides; ++i) {
            int next = (i + 1) % sides;
            int v1 = vertex_offset + i;
            int v2 = vertex_offset + next;
            int v3 = vertex_offset + sides + next;
            int v4 = vertex_offset + sides + i;
            
            obj << "f " << v1 << " " << v2 << " " << v3 << "\n";
            obj << "f " << v1 << " " << v3 << " " << v4 << "\n";
        }
        
        vertex_offset += 2 * sides;
    }
    
    static void write_sphere(std::ofstream& obj, const point3& center, double radius, 
                            int& vertex_offset, int stacks = 6, int slices = 8) {
        // Generate sphere vertices
        for (int i = 0; i <= stacks; ++i) {
            double phi = 3.14159265359 * i / stacks;
            for (int j = 0; j < slices; ++j) {
                double theta = 2.0 * 3.14159265359 * j / slices;
                
                double x = center.x() + radius * sin(phi) * cos(theta);
                double y = center.y() + radius * cos(phi);
                double z = center.z() + radius * sin(phi) * sin(theta);
                
                obj << "v " << x << " " << y << " " << z << "\n";
            }
        }
        
        // Generate sphere faces
        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                int next_j = (j + 1) % slices;
                int v1 = vertex_offset + i * slices + j;
                int v2 = vertex_offset + i * slices + next_j;
                int v3 = vertex_offset + (i + 1) * slices + next_j;
                int v4 = vertex_offset + (i + 1) * slices + j;
                
                if (i != stacks - 1) {
                    obj << "f " << v1 << " " << v2 << " " << v3 << "\n";
                    obj << "f " << v1 << " " << v3 << " " << v4 << "\n";
                }
            }
        }
        
        vertex_offset += (stacks + 1) * slices;
    }
    
    static void write_cornell_box_geometry(std::ofstream& obj, int& vertex_offset) {
        // Cornell Box walls as simple quads
        // Note: Camera is at Z=-800 looking at Z=0, box is 0-555 in all axes
        // We omit the front wall (Z=0) so camera can see inside
        
        // Left wall (green) - x=555, facing inward (-X)
        obj << "v 555 0 0\nv 555 0 555\nv 555 555 555\nv 555 555 0\n";
        obj << "f " << vertex_offset << " " << vertex_offset+1 << " " 
            << vertex_offset+2 << " " << vertex_offset+3 << "\n";
        vertex_offset += 4;
        
        // Right wall (red) - x=0, facing inward (+X)
        obj << "v 0 0 555\nv 0 0 0\nv 0 555 0\nv 0 555 555\n";
        obj << "f " << vertex_offset << " " << vertex_offset+1 << " " 
            << vertex_offset+2 << " " << vertex_offset+3 << "\n";
        vertex_offset += 4;
        
        // Floor (white) - y=0, facing inward (+Y)
        obj << "v 0 0 555\nv 555 0 555\nv 555 0 0\nv 0 0 0\n";
        obj << "f " << vertex_offset << " " << vertex_offset+1 << " " 
            << vertex_offset+2 << " " << vertex_offset+3 << "\n";
        vertex_offset += 4;
        
        // Ceiling (white) - y=555, facing inward (-Y)
        obj << "v 0 555 0\nv 555 555 0\nv 555 555 555\nv 0 555 555\n";
        obj << "f " << vertex_offset << " " << vertex_offset+1 << " " 
            << vertex_offset+2 << " " << vertex_offset+3 << "\n";
        vertex_offset += 4;
        
        // Back wall (white) - z=555, facing inward (-Z)
        obj << "v 0 0 555\nv 0 555 555\nv 555 555 555\nv 555 0 555\n";
        obj << "f " << vertex_offset << " " << vertex_offset+1 << " " 
            << vertex_offset+2 << " " << vertex_offset+3 << "\n";
        vertex_offset += 4;
        
        // Tall box (approximate)
        double tall_box_verts[][3] = {
            {265, 0, 295}, {430, 0, 295}, {430, 0, 460}, {265, 0, 460},  // Bottom
            {265, 330, 295}, {430, 330, 295}, {430, 330, 460}, {265, 330, 460}  // Top
        };
        for (int i = 0; i < 8; ++i) {
            obj << "v " << tall_box_verts[i][0] << " " << tall_box_verts[i][1] 
                << " " << tall_box_verts[i][2] << "\n";
        }
        // Tall box faces (outward facing)
        obj << "f " << vertex_offset+7 << " " << vertex_offset+6 << " " 
            << vertex_offset+5 << " " << vertex_offset+4 << "\n"; // Top
        obj << "f " << vertex_offset+1 << " " << vertex_offset+2 << " " 
            << vertex_offset+3 << " " << vertex_offset+0 << "\n"; // Bottom
        obj << "f " << vertex_offset+4 << " " << vertex_offset+5 << " " 
            << vertex_offset+1 << " " << vertex_offset+0 << "\n"; // Front
        obj << "f " << vertex_offset+6 << " " << vertex_offset+7 << " " 
            << vertex_offset+3 << " " << vertex_offset+2 << "\n"; // Back
        obj << "f " << vertex_offset+3 << " " << vertex_offset+7 << " " 
            << vertex_offset+4 << " " << vertex_offset+0 << "\n"; // Left
        obj << "f " << vertex_offset+5 << " " << vertex_offset+6 << " " 
            << vertex_offset+2 << " " << vertex_offset+1 << "\n"; // Right
        vertex_offset += 8;
        
        // Short box (approximate)
        double short_box_verts[][3] = {
            {130, 0, 65}, {295, 0, 65}, {295, 0, 230}, {130, 0, 230},  // Bottom
            {130, 165, 65}, {295, 165, 65}, {295, 165, 230}, {130, 165, 230}  // Top
        };
        for (int i = 0; i < 8; ++i) {
            obj << "v " << short_box_verts[i][0] << " " << short_box_verts[i][1] 
                << " " << short_box_verts[i][2] << "\n";
        }
        // Short box faces (outward facing)
        obj << "f " << vertex_offset+7 << " " << vertex_offset+6 << " " 
            << vertex_offset+5 << " " << vertex_offset+4 << "\n"; // Top
        obj << "f " << vertex_offset+1 << " " << vertex_offset+2 << " " 
            << vertex_offset+3 << " " << vertex_offset+0 << "\n"; // Bottom
        obj << "f " << vertex_offset+4 << " " << vertex_offset+5 << " " 
            << vertex_offset+1 << " " << vertex_offset+0 << "\n"; // Front
        obj << "f " << vertex_offset+6 << " " << vertex_offset+7 << " " 
            << vertex_offset+3 << " " << vertex_offset+2 << "\n"; // Back
        obj << "f " << vertex_offset+3 << " " << vertex_offset+7 << " " 
            << vertex_offset+4 << " " << vertex_offset+0 << "\n"; // Left
        obj << "f " << vertex_offset+5 << " " << vertex_offset+6 << " " 
            << vertex_offset+2 << " " << vertex_offset+1 << "\n"; // Right
        vertex_offset += 8;
    }
    
public:
    static bool export_paths_to_obj(const std::string& filename, 
                                    const std::vector<LightPath>& paths,
                                    bool include_scene = true) {
        std::ofstream obj(filename);
        if (!obj.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }
        
        // Generate MTL file for materials
        std::string mtl_filename = filename.substr(0, filename.find_last_of('.')) + ".mtl";
        std::ofstream mtl(mtl_filename);
        if (mtl.is_open()) {
            mtl << "# Material file for Cornell Box paths\n\n";
            
            // Green material for light paths
            mtl << "newmtl GreenPath\n";
            mtl << "Ka 0.0 0.5 0.0\n";  // Ambient
            mtl << "Kd 0.0 1.0 0.0\n";  // Diffuse - bright green
            mtl << "Ks 0.0 1.0 0.0\n";  // Specular
            mtl << "Ns 10.0\n";
            mtl << "d 0.8\n";  // Slightly transparent
            mtl << "illum 2\n\n";
            
            // White material for Cornell Box
            mtl << "newmtl BoxWhite\n";
            mtl << "Ka 0.7 0.7 0.7\n";
            mtl << "Kd 0.73 0.73 0.73\n";
            mtl << "Ks 0.0 0.0 0.0\n";
            mtl << "d 0.5\n";  // Semi-transparent
            mtl << "illum 1\n";
            
            mtl.close();
        }
        
        obj << "# Cornell Box with Light Paths\n";
        obj << "# Generated for Unity/Blender visualization\n";
        obj << "mtllib " << mtl_filename.substr(mtl_filename.find_last_of("/\\") + 1) << "\n\n";
        
        int vertex_offset = 1; // OBJ indices start at 1
        
        // Export Cornell Box geometry
        if (include_scene) {
            obj << "# Cornell Box Geometry\n";
            obj << "usemtl BoxWhite\n";  // Use white/semi-transparent material for box
            write_cornell_box_geometry(obj, vertex_offset);
            obj << "\n";
        }
        
        // Export light paths
        obj << "# Light Paths\n";
        obj << "usemtl GreenPath\n";  // Use green material for paths
        double path_radius = 0.5;  // Thickness of path lines (thinner for cleaner visualization)
        double vertex_radius = 1.0; // Size of path vertices
        
        int path_num = 0;
        for (const auto& path : paths) {
            obj << "# Path " << path_num++ << " (depth: " << path.depth << ")\n";
            
            // Draw line segments
            for (size_t i = 0; i + 1 < path.vertices.size(); ++i) {
                const auto& v1 = path.vertices[i];
                const auto& v2 = path.vertices[i + 1];
                
                // Color code by depth: green->yellow->red
                write_cylinder(obj, v1.position, v2.position, path_radius, vertex_offset);
            }
            
            // Draw spheres at vertices
            for (const auto& vertex : path.vertices) {
                write_sphere(obj, vertex.position, vertex_radius, vertex_offset);
            }
            
            obj << "\n";
        }
        
        obj.close();
        return true;
    }
};

#endif // PATH_VISUALIZER_H
