cmake_minimum_required(VERSION 3.16)
project(MasterDuelSwitcher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

add_executable(MasterDuelSwitcher
    src/main.cpp
)

target_link_libraries(MasterDuelSwitcher PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if (WIN32)
    set_target_properties(MasterDuelSwitcher PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

install(TARGETS MasterDuelSwitcher DESTINATION bin)
