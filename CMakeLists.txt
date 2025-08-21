cmake_minimum_required(VERSION 3.31)
project(Game_3D_C C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Znajdź OpenGL i GLUT
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(Game_3D_C main.c)

# Linkuj biblioteki
target_link_libraries(Game_3D_C PRIVATE
        ${OPENGL_LIBRARIES}
        ${GLUT_LIBRARIES}
        gdi32
        user32
)

# Dodaj include directories
target_include_directories(Game_3D_C PRIVATE ${GLUT_INCLUDE_DIRS})

# Opcjonalne: dodatkowe flagi kompilacji
target_compile_options(Game_3D_C PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Wextra>
        $<$<C_COMPILER_ID:MSVC>:/W4>
)