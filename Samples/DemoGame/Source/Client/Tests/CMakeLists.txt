lemuria_add_tests(demo_client_tests
    SOURCES
        DemoAdapterTest.cpp
        DemoGameTest.cpp
        DemoStyleTest.cpp
        GroundSceneTest.cpp
        ProofSceneTest.cpp
        SmoothnessTest.cpp
        TwoPlayersTest.cpp
    DEPS demo_client_lib demo_test_support test_server_lib)

# Needs a GPU or a software renderer, so it only runs when asked for (py tools/dev.py gpu-test).
lemuria_add_tests(demo_client_gpu_tests
    SOURCES ProofImageTest.cpp
    DEPS demo_client_lib
    LABELS gpu)
