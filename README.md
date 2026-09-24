# c_simulations

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
[![raylib](https://img.shields.io/badge/raylib-000000?style=for-the-badge&logo=raylib&logoColor=white)](https://www.raylib.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)](LICENSE)

A collection of C projects focused on 2D and 3D simulations.
Each project builds and runs independently.

## Projects

### [Spinning Cube](spinning_cube/README.md)

A spinning 3D cube rendered in the terminal with ASCII characters. Uses 3D
rotation, perspective projection, and a depth buffer to draw the visible faces.

![Spinning ASCII cube in the terminal](spinning_cube/cube_img.png)

Build and run on macOS or Linux with a C compiler, starting from the repository root:

```sh
cd spinning_cube
cc cube.c -o cube -lm
./cube
```

Use a terminal that supports ANSI escape sequences with room for 160 columns
and 44 rows. Press `Ctrl+C` to stop.

### [Particle Simulation](particle_simulation/README.md)

A 3D particle simulation built with C and raylib. Particles rain into a dotted
sphere, collide, and settle under gravity. Supports up to 350 particles, with
click spawning and an interactive camera.

![Purple particles inside a dotted sphere](particle_simulation/docs/images/particle-simulation.png)

Build and run on macOS with Homebrew and a C compiler, starting from the repository root:

```sh
brew install raylib pkgconf
cd particle_simulation
cc -O2 -std=c11 -Wall -Wextra main.c $(pkg-config --cflags --libs raylib) -o particle_simulation
./particle_simulation
```

- Left-click an empty spot inside the sphere to add a particle.
- Right-drag to orbit the camera; scroll to zoom.
- Press `Space` to pause or resume, `R` to reset, and `Escape` to close.

See the [project README](particle_simulation/README.md) for physics details and limitations.

## License

[MIT](LICENSE)
