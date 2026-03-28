# HW3 – 3D Rural Farmstead Scene

OpenGL 3.3 Core Profile scene viewer with Assimp model loading, Phong/Blinn-Phong
lighting, interactive FPS camera, texturing, and distance fog.

## Build Instructions

```bash
cd Hmk3_Skeleton
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The binary is placed at `build/bin/HW3`. Assets and shaders are automatically
copied into `build/bin/` during the build.

## Running

From any directory:

```bash
cd build
./bin/HW3
```

The executable locates its assets relative to its own path, so it works
regardless of the current working directory.

## Controls

| Input              | Action                                    |
|--------------------|-------------------------------------------|
| W / A / S / D      | Move forward / left / backward / right    |
| Q / E              | Move down / up                            |
| Right-click + drag | Look around (yaw and pitch)               |
| Scroll wheel       | Zoom (adjust FOV)                         |
| R                  | Reset camera to default position          |
| F                  | Toggle distance fog on/off                |
| ESC                | Quit                                      |

## Scene Objects (7 distinct OBJ models)

| Object      | File                           | Position        | Rotation | Scale           |
|-------------|--------------------------------|-----------------|----------|-----------------|
| Farmhouse   | farmhouse/Farm_house.obj       | (0, 0, -10)     | 0°       | uniform 1.0     |
| Barrel ×2   | barrel/Barrel_OBJ.obj          | (3.5,0,-8) etc. | 0°, 15°  | non-uniform     |
| Pine Tree ×4| pinetree/Pine_Tree.obj         | corners ±12     | 10–45°   | varied          |
| Park Bench  | bench/wooden_bench.obj         | (-6, 0, 2)      | -30°     | uniform 1.0     |
| Street Lamp ×2 | lamp/objLamp.obj            | (5,0,0) (-5,0,5)| 0°       | uniform 0.5     |
| Robot       | robot/Robot.obj                | (-3, 0, -4)     | 180°     | uniform 0.8     |
| Farmer      | farmer/farmer.obj              | (1, 0, -6)      | 160°     | uniform 1.0     |

Ground plane: 100×100 quad textured with tiled grass (pinetree/Texture/grass.jpg).

## Lighting

- **Directional light** (sun): warm late-afternoon orange, direction (-0.5, -1, -0.5)
- **Point light 0**: cool white at (5, 4, 0) with quadratic attenuation
- **Point light 1**: cool white at (-5, 4, 5) with quadratic attenuation
- Blinn-Phong shading with per-fragment specular highlights

## Bonus Features

- **Distance fog**: exponential fog blending scene into the sky color. Toggle with F key.

## Assets

All 3D models were provided with the assignment skeleton. Textures are loaded
via stb_image. The loader handles inline MTL comments and extension mismatches
automatically.

## Dependencies

- OpenGL 3.3+ (Core Profile)
- GLFW 3.x
- GLEW 2.x
- Assimp 5.x
- GLM
- stb_image (bundled)
