cmake_minimum_required(VERSION 3.16)

project(dungeon_village_research LANGUAGES CXX)

# 先在根作用域初始化共享策略，让全部子包继承同一套bin/lib与运行库设置。
# prototype内部纳入example；这里不重复添加领域库或领域测试。
include(cmake/ResearchLibraries.cmake)
include(CTest)

add_subdirectory(prototype)
add_subdirectory(tools)
