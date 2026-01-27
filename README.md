# Brioche Analytic Rendering Interface  

[Khronos ANARI: glTF To ANARI](https://github.com/KhronosGroup/ANARI-SDK/blob/next_release/src/anari_test_scenes/scenes/file/gltf2anari.h)  
[Khronos ANARI: Hydra ANARI Render Delegate](https://github.com/KhronosGroup/ANARI-SDK/blob/next_release/src/hdanari/renderDelegate.h)  
[Pxiar OpenUSD: Hydra Storm Render Delegate](https://github.com/PixarAnimationStudios/OpenUSD/blob/dev/pxr/imaging/hdSt/renderDelegate.h)  

- [ ] Geometry  
    - [ ] Triangle  
        - [x] Morphing Deformation  
        - [x] [Skinning Deformation](https://github.com/HanetakaChou/Dual-Quaternion-Linear-Blending)  
        - [ ] Ray Tracing Acceleration Structure  
- [ ] Light  
    - [ ] [Quad](https://github.com/HanetakaChou/Linearly-Transformed-Cosine)  
    - [ ] [HDRI](https://github.com/HanetakaChou/Spherical-Harmonic)  
        - [x] [Equirectangular (Latitude-Longitude) Map](https://www.pbr-book.org/3ed-2018/Light_Sources/Infinite_Area_Lights)  
        - [x] [Octahedral Map](https://www.pbr-book.org/4ed/Light_Sources/Infinite_Area_Lights#ImageInfiniteLights)  
        - [ ] [~~Cube Map~~](https://dev.epicgames.com/documentation/en-us/unreal-engine/creating-cubemaps?application_version=4.27)  
- [ ] Material  
    - [x] [PBR (Microfacet Model | Trowbridge Reitz)](https://pharr.org/matt/blog/2022/05/06/trowbridge-reitz)  
