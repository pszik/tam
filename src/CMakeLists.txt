set(CMAKE_C_STANDARD 17)

add_library(tam tam.c)
target_include_directories(tam PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(tam_exe main.c)
target_include_directories(tam_exe PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(tam_exe tam)