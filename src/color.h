#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"
#include "rtweekend.h"
#include <iostream>

using color = vec3; // Color is an alias for vec3

// Write Color to Output Stream with sampling support
inline void write_color(std::ostream &out, color pixel_color, int samples_per_pixel) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Divide the color by the number of samples
    auto scale = 1.0 / samples_per_pixel;
    r *= scale;
    g *= scale;
    b *= scale;

    // Apply gamma correction (gamma=2.0 means raising to 1/2 power, i.e., sqrt)
    r = sqrt(r);
    g = sqrt(g);
    b = sqrt(b);

    // Write the translated [0,255] value of each color component
    out << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';
}

// Original version without sampling (for compatibility)
inline void write_color(std::ostream &out, color pixel_color) {
    // Scale color components to [0,255]
    int ir = static_cast<int>(255.999 * pixel_color.x());
    int ig = static_cast<int>(255.999 * pixel_color.y());
    int ib = static_cast<int>(255.999 * pixel_color.z());

    out << ir << ' ' << ig << ' ' << ib << '\n';
}

#endif // COLOR_H