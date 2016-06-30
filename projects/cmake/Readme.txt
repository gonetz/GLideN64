cmake project files located inside src folder. To build the project with cmake, run

cmake [-DCMAKE_BUILD_TYPE=Debug] [-DOPT=On] [-DNOHQ=On] [-DUSE_UNIFORMBLOCK=On] -DMUPENPLUSAPI=On ../../src/

-DCMAKE_BUILD_TYPE=Debug - optional parameter, if you want debug build. Default buid type is Release
-DOPT=On - optional parameter. set it if you want to enable additional optimizations (can cause additional bugs).
-DNOHQ=On - build without realtime texture enhancer library (GLideNHQ)
-DUSE_UNIFORMBLOCK=On - Use uniform blocks in shaders. May help to improve performance. Not supported by GLES2 hardware.
-DMUPENPLUSAPI=On - currently cmake build works only for mupen64plus version of the plugin.
