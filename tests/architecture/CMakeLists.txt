add_subdirectory(custom_framework)

if(LEMURIA_SANITIZE)
    add_subdirectory(sanitizer_canary)
endif()
