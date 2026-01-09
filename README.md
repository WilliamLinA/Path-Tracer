# Path Tracer - Cornell Box Renderer

A simple yet complete Monte Carlo path tracer implemented from scratch in C++ for rendering the classic Cornell Box scene with physically-based global illumination.

### Technical Details

- **Language**: C++17
- **Build System**: CMake
- **Output Format**: .ppm
- **Resolution**: 600×600 pixels (configurable)
- **Samples per Pixel**: 200 (configurable)

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Running

```bash
cd build
./bin/Release/ImageRenderer.exe | Out-File -Encoding ascii image.ppm
magick cornell_box.ppm cornell_box.png
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
const int max_depth = 10;
```

Adjust these in `main.cpp` for different quality/performance tradeoffs.

## References

- [Ray Tracing in One Weekend Series](https://raytracing.github.io/)

## License

MIT License - Free for educational and research purposes.

## Author

Yi Lin
