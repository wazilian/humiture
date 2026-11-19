
cmake_minimum_required(VERSION 3.5)

file(READ "${CMAKE_CURRENT_LIST_DIR}/version.txt" TMP_PROJECT_VER)
string(STRIP "${TMP_PROJECT_VER}" PROJECT_VER)
set(PROJECT_VER "${PROJECT_VER}")
string(REPLACE "." "_" PROJECT_VER_CLEAN "${PROJECT_VER}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project("Humiture-${PROJECT_VER_CLEAN}")
