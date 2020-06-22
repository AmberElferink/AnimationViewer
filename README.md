# AnimationViewer

*by Sandy Carter, Thijs Hendrickx, Amber Elferink and Anne van Ede*

## Building
Building the project requires CMake and a package manager.
There are many options for package managers.

### vcpkg

Follow the installation steps at https://github.com/Microsoft/vcpkg

Make sure to set the global environment variable `VCPKG_DEFAULT_TRIPLET=x64-windows`

Install the dependencies
```
vcpkg install --triplet x64-windows entt sdl2 glslang spirv-cross glm assimp
```

When generating the cmake project use the vcpkg toolchain file.

### msys2

Install the following packages

```bash
pacman -S mingw-w64-x86_64-sdl2 mingw-w64-x86_64-glslang mingw-w64-x86_64-spirv-cross mingw-w64-x86_64-glm  mingw-w64-x86_64-assimp
```

##The Project


To  learn  more  about  animation  engines,  we  implemented  our  own.   The  goal  was  to  bothimplement key frame animation and motion capture animation.  We will discuss the programfrom a user perspective and explain what is behind what you see.Because this project is meant to give us an insight in using animation and MoCap files,we chose to focus a large portion of our time on supporting multiple different file formats.  Itis important to work with many file formats because this gives a broader insight into usinganimation files as a whole.  Because some file formats like FBX are closed and documentationis scarce for almost all parsers, this was a difficult task.We support L3D, ANM, C3D, FBX, GLTF files.  BVH is also partially supported, howeverfor  this  the  animations  do  not  show  perfectly.   We  implemented  a  skeleton  structure  andkeyframe  blending  for  smooth  animations.   We  implemented  skinning  and  a  easy  to  useinterface that allows for maximum flexibility and easy use for an animator.