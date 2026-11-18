lemuria_add_tests(platform_sdl_tests
    SOURCES PlatformInfoTest.cpp
    DEPS lemuria_platform_sdl)

# Needs a GPU or a software renderer, so it only runs when asked for (py tools/dev.py gpu-test).
lemuria_add_tests(platform_gpu_tests
    SOURCES GpuProbeTest.cpp
    DEPS lemuria_platform_sdl
    LABELS gpu)
