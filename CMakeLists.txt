cmake_minimum_required (VERSION 3.8)
project ("WaterfallImageGenerator")

find_package(FFTW3f CONFIG REQUIRED)
find_package(libpng CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)

add_executable (WaterfallImageGenerator "main.cpp"  "wav.h" "wav.cpp" "color_table.h" "color_table.cpp" "window.h" "window.cpp")
target_link_libraries(WaterfallImageGenerator FFTW3::fftw3f png Volk::volk)
