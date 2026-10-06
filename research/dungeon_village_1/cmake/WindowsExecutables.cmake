# Windows可能将带update等名称的32位测试误判为安装器；明确使用调用者权限。
# 三个研究包共用同一资源，不改变CTest名称、断言或游戏规则。
if(WIN32)
    enable_language(RC)
    configure_file("${CMAKE_CURRENT_LIST_DIR}/windows.rc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/research-windows.rc" @ONLY)
    get_property(research_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(research_target IN LISTS research_targets)
        get_target_property(research_target_type "${research_target}" TYPE)
        if(research_target_type STREQUAL "EXECUTABLE")
            target_sources("${research_target}" PRIVATE
                           "${CMAKE_CURRENT_BINARY_DIR}/research-windows.rc")
        endif()
    endforeach()
endif()
