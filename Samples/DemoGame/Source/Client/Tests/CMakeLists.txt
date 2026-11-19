lemuria_add_tests(demo_client_tests
    SOURCES DemoStyleTest.cpp GroundSceneTest.cpp ProofSceneTest.cpp SmoothnessTest.cpp
    DEPS demo_client_lib)

# Needs a GPU or a software renderer, so it only runs when asked for (py tools/dev.py gpu-test).
lemuria_add_tests(demo_client_gpu_tests
    SOURCES ProofImageTest.cpp
    DEPS demo_client_lib
    LABELS gpu)
