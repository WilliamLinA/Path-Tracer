# Path Tracer - Cornell Box Renderer

A simple yet complete Monte Carlo path tracer implemented from scratch in C++ for rendering the classic Cornell Box scene with physically-based global illumination.

![Cornell Box](cornell_box_preview.png)

## Features

- **Monte Carlo Path Tracing** - Physically accurate light transport simulation
- **Lambertian Materials** - Ideal diffuse reflection (BRDF)
- **Area Light Sources** - Self-emissive surfaces for realistic lighting
- **Multi-sampling** - 200+ samples per pixel for noise reduction
- **Gamma Correction** - Proper linear-to-sRGB color space conversion
- **Cornell Box Scene** - Classic test scene with colored walls and boxes

## Implementation Highlights

### Core Components

- **Ray-Geometry Intersection** - Axis-aligned rectangle primitives
- **Material System** - Lambertian diffuse and emissive materials
- **Camera Model** - Configurable position, FOV, and aspect ratio
- **Recursive Ray Tracing** - Up to 50 bounces for global illumination
- **Random Sampling** - Uniform hemisphere sampling for diffuse reflection

### Technical Details

- **Language**: C++17
- **Build System**: CMake
- **Dependencies**: Standard library only (no external libraries)
- **Output Format**: PPM (Portable Pixmap)
- **Resolution**: 600×600 pixels (configurable)
- **Samples per Pixel**: 200 (configurable)

## Building

```bash
# Create build directory
mkdir build
cd build

# Generate build files
cmake ..

# Compile
cmake --build . --config Release
```

## Running

```bash
# Render Cornell Box
cd build
.\bin\Release\ImageRenderer.exe > cornell_box.ppm

# Convert to PNG (optional, requires ImageMagick)
magick cornell_box.ppm cornell_box.png
```

## Project Structure

```
Project/
├── src/
│   ├── main.cpp           # Main rendering loop
│   ├── vec3.h             # 3D vector math
│   ├── ray.h              # Ray class
│   ├── color.h            # Color utilities and gamma correction
│   ├── camera.h           # Camera with configurable parameters
│   ├── hittable.h         # Hittable object interface
│   ├── hittable_list.h    # Scene container
│   ├── aarect.h           # Axis-aligned rectangles
│   ├── material.h         # Material system (Lambertian, Light)
│   └── rtweekend.h        # Utility functions
├── CMakeLists.txt         # Build configuration
├── .gitignore
└── README.md
```

## Scene Configuration

The Cornell Box scene consists of:
- **Left Wall**: Green Lambertian (0.12, 0.45, 0.15)
- **Right Wall**: Red Lambertian (0.65, 0.05, 0.05)
- **Other Walls**: White Lambertian (0.73, 0.73, 0.73)
- **Light**: Area light on ceiling (15, 15, 15)
- **Two Boxes**: White diffuse boxes (tall and short)

## Rendering Parameters

```cpp
const int image_width = 600;
const int samples_per_pixel = 200;
const int max_depth = 50;
```

Adjust these in `main.cpp` for different quality/performance tradeoffs.

## Theory

This renderer implements the **Rendering Equation**:

$$L_o(p, \omega_o) = L_e(p, \omega_o) + \int_{\Omega} f_r(p, \omega_i, \omega_o) L_i(p, \omega_i) (\omega_i \cdot n) d\omega_i$$

Solved using **Monte Carlo integration** with importance sampling.

### Key Algorithms

1. **Path Tracing**: Recursive ray bouncing through the scene
2. **Lambertian Reflection**: Cosine-weighted hemisphere sampling
3. **Russian Roulette**: Probabilistic path termination (optional)
4. **Multi-sampling**: Average multiple random samples per pixel
5. **Gamma Correction**: Convert linear RGB to sRGB (γ=2.0)

## Performance

Rendering a 600×600 image with 200 samples takes approximately **2-5 minutes** on modern CPUs (single-threaded).

## Future Extensions

Potential improvements for extending to Metropolis Light Transport (MLT):
- [ ] Bidirectional path tracing
- [ ] Path space representation
- [ ] Metropolis-Hastings sampling
- [ ] Multiple importance sampling
- [ ] BVH acceleration structure

## Educational Purpose

This project is designed for learning and understanding:
- Monte Carlo path tracing fundamentals
- Global illumination algorithms
- BRDF and light transport theory
- Preparing for MLT implementation
- Debugging and validating rendering algorithms

## References

- [Ray Tracing in One Weekend Series](https://raytracing.github.io/)
- [PBRT: Physically Based Rendering](https://www.pbr-book.org/)
- [Cornell Box Original Data](https://www.graphics.cornell.edu/online/box/data.html)
- [Metropolis Light Transport (Veach & Guibas, 1997)](https://graphics.stanford.edu/papers/metro/)

## License

MIT License - Free for educational and research purposes.

## Author

Created as part of research into probabilistic computing and light transport algorithms.
