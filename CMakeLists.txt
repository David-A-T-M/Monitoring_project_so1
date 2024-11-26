cmake_minimum_required(VERSION 3.10)
project(monitoring_project)

# Añadir las carpetas include y lib
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/prom/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/promhttp/include)

find_package(libmicrohttpd REQUIRED)
find_package(cJSON REQUIRED)

add_subdirectory(lib/prom)
add_subdirectory(lib/promhttp)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wno-error)
endif()

# Añadir los archivos fuente de tu proyecto
add_executable(monitoring_project
        src/main.c
        src/metrics.c
        src/expose_metrics.c
)

# Linkear bibliotecas dinámicas desde la carpeta lib
#[[link_directories(${CMAKE_SOURCE_DIR}/lib)

# Vincular bibliotecas prom y promhttp
target_link_libraries(monitoring_project
        ${CMAKE_SOURCE_DIR}/lib/prom/libprom.so
        ${CMAKE_SOURCE_DIR}/lib/promhttp/libpromhttp.so
)]]

target_link_libraries(monitoring_project
        prom
        promhttp
)

# Incluir los directorios de cJSON
include_directories(/usr/include)

# Vincular la biblioteca cJSON
target_link_libraries(monitoring_project libmicrohttpd::libmicrohttpd cjson::cjson)