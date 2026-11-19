add_subdirectory(custom_framework)
add_subdirectory(frame_allocations)

if(LEMURIA_SANITIZE)
    add_subdirectory(sanitizer_canary)
endif()
