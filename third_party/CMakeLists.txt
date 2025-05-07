#find_package(Vulkan REQUIRED)

add_library(glm INTERFACE)

#add_library(stb_image INTERFACE)


# glm  header only libs so we only need the include path
target_include_directories(glm INTERFACE glm)
#target_include_directories(stb_image INTERFACE stb_image)

option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(glfw)

#add_library(imgui STATIC)

#target_include_directories(imgui PUBLIC imgui)

#target_sources(imgui PRIVATE 
 #   imgui/imgui.h
 #   imgui/imgui.cpp

#    imgui/imgui_demo.cpp
#    imgui/imgui_draw.cpp
 #   imgui/imgui_widgets.cpp

#    imgui/imgui_impl_vulkan.cpp
    #imgui/imgui_impl_sdl.cpp
 #   )

#target_link_libraries(imgui PUBLIC Vulkan::Vulkan )


