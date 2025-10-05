cmake_minimum_required(VERSION 3.15)
project(ludum-dare-58 VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find raylib (install via brew/apt/vcpkg first)
find_package(raylib REQUIRED)

# Executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/game.cpp
    src/collectible.cpp
    src/map.cpp
    src/enemy.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Platform-specific libraries
if(APPLE)
    # macOS frameworks
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        "-framework IOKit" 
        "-framework Cocoa" 
        "-framework OpenGL"
    )
elseif(UNIX)
    # Linux libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE m pthread dl)
elseif(WIN32)
    # Windows libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm gdi32)
endif()

