# Ray casting shadows

Real-time 2D shadow casting in C with SDL2. A light source emits 300 rays; every ray
stops at the first obstacle it hits (a circle or the window border), so the area behind
the obstacle stays dark. Drag the mouse to move the light.

## Controls

| Input | Action |
|-------|--------|
| Hold mouse button + move | move the light source |
| Close window | quit |

The current FPS is shown in the window title.

## Build and run

Requires a C compiler and SDL2.

- **Windows (MSYS2 UCRT64):** `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2 make`
- **Linux:** `sudo apt install build-essential libsdl2-dev`

```
git clone https://github.com/manneriadev/ray-shadows
cd ray-shadows
make
./ray        # ray.exe on Windows
```

## How it works

Each ray is a parametric line `P(t) = O + t * (cos θ, sin θ)`, `t > 0`, where `O` is the
center of the light and `θ = 2πi / N`.

- **Ray vs circle.** Substituting the ray into `|P - C|² = r²` gives a quadratic
  `t² + B·t + C₀ = 0`, with discriminant `D = B² - 4C₀`. `D < 0` means a miss; otherwise the
  smallest positive root is the hit distance.
- **Ray vs window border.** Intersecting with `x = 0`, `x = W`, `y = 0`, `y = H` gives
  `t = (border - O) / direction`; the hit is valid if `t > 0` and the other coordinate is inside the window.
- **Nearest hit wins.** The smallest valid `t` across all candidates is the end of the ray.

All intersections are closed-form, so no ray marching or per-pixel stepping is needed.

## Optimization notes

The project was developed on a low-memory virtual machine, so performance was a focus.
What is used in `src/ray.c`:

- sin/cos of all ray angles are **precomputed once** into lookup tables
- everything that does not depend on the ray (obstacle center, `C₀`, light position) is **hoisted out of the loop**
- `sqrt` is evaluated only when the discriminant is positive
- circles are drawn **once** into textures, not every frame
- vsync is enabled, so the render loop does not burn 100% CPU

## Project layout

| Path | Contents |
|------|----------|
| `src/ray.c` | the program |

## License

MIT, see the LICENSE file.
