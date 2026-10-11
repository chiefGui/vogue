# Vogue

Renderer-independent UI behavior in C11. Caller-owned state, no heap allocations.
Currently: contexts and buttons.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Use with CMake

```cmake
add_subdirectory(vendor/vogue)
target_link_libraries(your_app PRIVATE vogue::vogue)
```

Include `<vogue.h>`. See [the public header](include/vogue.h) for the API.
