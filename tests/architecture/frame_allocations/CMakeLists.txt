# Proves that the warm frame asks for no memory (INV-13). The counter replaces `new` for the whole test program,
# which only this folder may do (tools/checks, cpp-rules).
lemuria_add_tests(frame_allocation_tests
    SOURCES AllocationCounter.cpp AllocationCounterTest.cpp FrameAllocationsTest.cpp
    DEPS demo_client_lib demo_test_support test_server_lib)

# Draws with the GPU, or a software renderer, so it only runs when asked for (py tools/dev.py gpu-test).
lemuria_add_tests(frame_draw_allocation_tests
    SOURCES AllocationCounter.cpp DrawAllocationsTest.cpp
    DEPS demo_client_lib demo_test_support test_server_lib
    LABELS gpu)
