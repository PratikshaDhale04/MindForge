cmake_minimum_required(VERSION 3.16)

project(QuizMaster
    VERSION 1.0.0
    DESCRIPTION "QuizMaster - a simple animated quiz game built with C++ and SFML"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Build settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The finished program lands in build/bin so it is easy to find.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Finding SFML
#
# SFML is not part of Windows, so its location has to be told to CMake.
# If you installed SFML somewhere CMake cannot guess, run:
#
#   cmake -S . -B build -DSFML_DIR=C:/path/to/SFML
#
# On MSYS2 you can instead point at the whole toolchain folder:
#   cmake -S . -B build -DQUIZMASTER_DEP_PREFIX=C:/msys64/ucrt64
# ---------------------------------------------------------------------------
set(QUIZMASTER_DEP_PREFIX "" CACHE STRING "Folder that contains SFML (include/ and lib/)")

if (QUIZMASTER_DEP_PREFIX)
    list(APPEND CMAKE_PREFIX_PATH "${QUIZMASTER_DEP_PREFIX}")
endif()

message(STATUS "Searching for SFML in: ${CMAKE_PREFIX_PATH}")

# SFML 3 is required (SFML 2 changed too much to build against it unchanged).
find_package(SFML 3 COMPONENTS System Window Graphics Audio)

if (NOT SFML_FOUND)
    message(FATAL_ERROR
        "SFML 3 was not found.\n"
        "Install SFML 3 and tell CMake where it is, for example:\n"
        "    cmake -S . -B build -DSFML_DIR=C:/SFML\n"
        "or on MSYS2:\n"
        "    cmake -S . -B build -DQUIZMASTER_DEP_PREFIX=C:/msys64/ucrt64")
endif()

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
add_executable(QuizMaster
    src/main.cpp
    src/Game.cpp
    src/Button.cpp
    src/Quiz.cpp
    src/Question.cpp
    src/Notification.cpp
    src/Audio.cpp
)


target_include_directories(QuizMaster PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(QuizMaster PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

# SFML headers are noisy about a few deprecated typedefs; we do not use them.
if (MSVC)
    target_compile_options(QuizMaster PRIVATE /W4)
else()
    target_compile_options(QuizMaster PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Copy the fonts, sounds and the empty data files next to the .exe, so the game
# can be started by double-clicking it. The data files are created on first run
# if they are missing, so an existing copy is never overwritten.
# ---------------------------------------------------------------------------
add_custom_command(TARGET QuizMaster POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:QuizMaster>/assets"
    COMMENT "Copying fonts and sounds next to QuizMaster.exe")

# SFML and the MinGW runtime DLLs have to be reachable at run time. On MSVC they
# come from the system; with a MinGW toolchain they are copied next to the .exe
# so the game can be started by double-clicking it.
if (NOT MSVC AND TARGET SFML::Graphics)
    set(QUIZMASTER_DLLS
        # The four SFML libraries themselves.
        libsfml-graphics-3
        libsfml-window-3
        libsfml-system-3
        libsfml-audio-3
        # The compiler's own runtime, which is not part of Windows.
        libstdc++-6
        libgcc_s_seh-1
        libwinpthread-1
        # What the SFML libraries need in turn: fonts, images and sound.
        libfreetype-6
        libFLAC
        libogg-0
        libvorbis-0
        libvorbisenc-2
        libvorbisfile-3
        # OpenAL does the actual playing. It is listed here rather than left to
        # the automatic scan because it sits one level further down.
        libopenal-1
        # FreeType's own dependencies. It links these directly, so without them
        # the .exe refuses to start with 0xC0000135 even though every DLL named
        # above is sitting right next to it.
        libbrotlidec
        libbrotlicommon
        libbz2-1
        libharfbuzz-0
        libpng16-16
        zlib1
        # HarfBuzz's dependencies, one level further down again.
        libgraphite2
        libglib-2.0-0
        libintl-8
        libiconv-2
        libpcre2-8-0
        #
        # libzstd.dll is deliberately NOT listed. It belongs to the compiler
        # tools, not to the game, and it imports api-ms-win-crt-private-l1-1-0
        # - a private Windows API set that is missing on an ordinary system.
        # Shipping it makes the game fail to start with 0xC0000135.
    )

    foreach (_dll IN LISTS QUIZMASTER_DLLS)
        get_filename_component(_dll_path "${QUIZMASTER_DEP_PREFIX}/bin/${_dll}.dll" ABSOLUTE)
        if (EXISTS "${_dll_path}")
            add_custom_command(TARGET QuizMaster POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_dll_path}" "$<TARGET_FILE_DIR:QuizMaster>"
                COMMENT "Copying ${_dll}.dll next to QuizMaster.exe")
        else()
            message(WARNING "${_dll}.dll was not found in ${QUIZMASTER_DEP_PREFIX}/bin.\n"
                            "The game will still build, but start it through "
                            "run_quizmaster.bat so Windows can find the libraries.")
        endif()
    endforeach()
endif()