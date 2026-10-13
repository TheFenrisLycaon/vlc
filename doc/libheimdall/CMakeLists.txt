CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

# configure with your own path
#  -DLIBHEIMDALL_SDK_INC:STRING=S:/sources/build/win64/win64/heimdall-4.0.0-dev/sdk/include
#  -DLIBHEIMDALL_SDK_LIB:STRING=S:/sources/build/win64/win64/heimdall-4.0.0-dev/sdk/lib
#
# or set them in your VSCode settings
# {
#     "cmake.configureSettings": {
#         "LIBHEIMDALL_SDK_INC": "S:/sources/heimdall/include",
#         "LIBHEIMDALL_SDK_LIB": "S:/sources/build/win64/win64/lib/.libs"
#     }
# }

set("LIBHEIMDALL_SDK_INC" "" CACHE PATH "libheimdall include folder, containing the heimdall/ includes")
set("LIBHEIMDALL_SDK_LIB" "" CACHE PATH "libheimdall library folder, containing the libheimdall libraries")

project("libheimdall samples")

# define the libheimdall external build
add_library(libheimdall SHARED IMPORTED GLOBAL)
target_include_directories(libheimdall INTERFACE ${LIBHEIMDALL_SDK_INC})
if (MSVC)
    set_target_properties(libheimdall PROPERTIES IMPORTED_IMPLIB "${LIBHEIMDALL_SDK_LIB}/libheimdall.lib")
else ()
    set_target_properties(libheimdall PROPERTIES IMPORTED_IMPLIB "${LIBHEIMDALL_SDK_LIB}/libheimdall.dll.a")
endif ()

if(WIN32)

    add_executable(d3d9_player WIN32 d3d9_player.c)
    target_link_libraries(d3d9_player libheimdall d3d9)

    add_executable(d3d11_player WIN32 d3d11_player.cpp)
    target_compile_definitions(d3d11_player PRIVATE _WIN32_WINNT=0x0601)
    target_link_libraries(d3d11_player libheimdall d3d11 d3dcompiler uuid)

    add_executable(win_player WIN32 win_player.c)
    target_link_libraries(win_player libheimdall)

endif()
