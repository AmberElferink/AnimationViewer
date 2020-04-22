# AnimationViewer

*by Sandy Carter and Anne van Ede*

## Building
Building the project requires CMake and a package manager.
There are many options for package managers.

### vcpkg

Follow the installation steps at https://github.com/Microsoft/vcpkg

Make sure to set the global environment variable `VCPKG_DEFAULT_TRIPLET=x64-windows`

Install the dependencies
```
vcpkg install sdl2 glslang spirv-cross
```

When generating the cmake project use the vcpkg toolchain file.

### msys2

Install the following packages

```bash
pacman -S mingw-w64-x86_64-sdl2 mingw-w64-x86_64-glslang mingw-w64-x86_64-spirv-cross
```
