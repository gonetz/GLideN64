cmake project files located inside src folder. To build the project with cmake, run

cmake [-DCMAKE_BUILD_TYPE=Debug] [-DVEC4_OPT=On] [-DCRC_OPT=On] [-DNEON_OPT=On] [-DNOHQ=On] [-DUSE_UNIFORMBLOCK=On] -DMUPENPLUSAPI=On ../../src/

-DCMAKE_BUILD_TYPE=Debug - optional parameter, if you want debug build. Default buid type is Release
-DVEC4_OPT=On  - optional parameter. set it if you want to enable additional VEC4 optimization (can cause additional bugs).
-DCRC_OPT=On  - optional parameter. set it if you want to enable additional CRC optimization (can cause additional bugs).
-DNEON_OPT=On - optional parameter. set it if you want to enable additional ARM NEON optimization (can cause additional bugs).
-DNOHQ=On - build without realtime texture enhancer library (GLideNHQ).
-DUSE_UNIFORMBLOCK=On - Use uniform blocks in shaders. May help to improve performance. Not supported by GLES2 hardware.
-DMUPENPLUSAPI=On - currently cmake build works only for mupen64plus version of the plugin.
