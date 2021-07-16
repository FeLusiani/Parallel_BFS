cmake_minimum_required(VERSION 3.0.0)

set(PROJECT_NAME
    Parallel_BFS
)
set(ParBFS_SOURCE_DIR
    src
)
set(ParBFS_SOURCE
    ${ParBFS_SOURCE_DIR}/main.cpp
)
set(ParBFS_LIBRARIES_DIR
    libs
)
set(ParBFS_LIBRARIES 
     fastflow
)

project(${PROJECT_NAME})

#########
# GTest #
#########
# enable_testing()
# add_subdirectory(libs/gtest-1.7.0)
# include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

###########
# Project #
###########
add_executable(${ParBFS_PROJECT_NAME} ${ParBFS_SOURCE})

foreach(LIBRARY ${ParBFS_LIBRARIES})
    add_subdirectory("${ParBFS_LIBRARIES_DIR}/${LIBRARY}")
endforeach(LIBRARY)
target_link_libraries(${ParBFS_PROJECT_NAME} ${ParBFS_LIBRARIES})