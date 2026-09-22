cmake_minimum_required(VERSION 3.5)

project(gpvkfr) # graphics programming vulkan Franciszek Rakowiecki

set (CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

add_executable(gpvkfr src/main.cpp
        src/Engine.cpp
        src/Engine.h
        src/Window.cpp
        src/Window.h
        src/structs.h
        src/Singleton.h
        src/Logger.cpp
        src/Logger.h
        src/Mesh.cpp
        src/Mesh.h
        src/GameObject.cpp
        src/GameObject.h
        src/Scene.cpp
        src/Scene.h
        src/Texture2D.cpp
        src/Texture2D.h
        src/TextureCube.cpp
        src/TextureCube.h
        src/AllocatedBuffer.cpp
        src/AllocatedBuffer.h
        src/AllocatedImage.cpp
        src/AllocatedImage.h
        src/Camera.cpp
        src/Camera.h
        src/ResourceLoader.cpp
        src/ResourceLoader.h
        src/ResourceManager.cpp
        src/ResourceManager.h
        src/CommandPool.cpp
        src/CommandPool.h
        src/Sampler.cpp
        src/Sampler.h
        src/DepthBuffer.cpp
        src/DepthBuffer.h
        src/ShaderProgram.cpp
        src/ShaderProgram.h
        src/SwapChain.cpp
        src/SwapChain.h
        src/Renderer.cpp
        src/Renderer.h
        src/DescriptorSet.cpp
        src/DescriptorSet.h
        src/GBuffer.cpp
        src/GBuffer.h
        src/Shader.cpp
        src/Shader.h
        src/Material.cpp
        src/Material.h
        src/ImageTransitionHelper.cpp
        src/ImageTransitionHelper.h
        src/Fence.cpp
        src/Fence.h
        src/UploadPool.cpp
        src/UploadPool.h
)

target_include_directories(gpvkfr
        PRIVATE
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(gpvkfr
        PRIVATE
        ${Vulkan_LIBRARIES}
)

include(FetchContent)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.3
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG v3.3.0
)
FetchContent_MakeAvailable(vma)

FetchContent_Declare(
        tinyglTF
        GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
        GIT_TAG v3.0.0
)
FetchContent_MakeAvailable(tinyglTF)

FetchContent_Declare(
        tinyexr
        GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
        GIT_TAG v1.0.8
)
set(TINYEXR_BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tinyexr)

target_include_directories(gpvkfr PRIVATE
        ${tinyexr_SOURCE_DIR}
        ${tinyexr_SOURCE_DIR}/deps/miniz
)

target_link_libraries(gpvkfr PRIVATE Vulkan::Vulkan glfw glm GPUOpen::VulkanMemoryAllocator tinygltf tinyexr miniz)

file(GLOB_RECURSE SHADERS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/resources/shaders/*.frag"
    "${CMAKE_SOURCE_DIR}/resources/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/resources/shaders/*.comp"
)

message("Found shaders ${SHADERS}")

set(SPIRV_FILES)

foreach (shader ${SHADERS})
    file(RELATIVE_PATH relPath ${CMAKE_SOURCE_DIR}/resources/shaders ${shader})
    set(outputfile "${CMAKE_BINARY_DIR}/resources/shaders/${relPath}.spv")

    get_filename_component(outputDir ${outputfile} DIRECTORY)

    file(MAKE_DIRECTORY ${outputDir})

    add_custom_command(
            OUTPUT ${outputfile}
            COMMAND glslangValidator -V -g ${shader} -I${CMAKE_SOURCE_DIR}/resources/shaders/ -o ${outputfile}
            DEPENDS ${shader}
            COMMENT "Compiling shader ${relPath} to SPV"
            VERBATIM
    )

    list(APPEND SPIRV_FILES ${outputfile})
endforeach ()

add_custom_target(CompileShaders ALL DEPENDS ${SPIRV_FILES})
add_dependencies(gpvkfr CompileShaders)

add_custom_target(CopyResources ALL DEPENDS CompileShaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
        COMMENT "Copying resources to build"
)

file(GLOB_RECURSE PNG_FILES
        "${CMAKE_SOURCE_DIR}/resources/images/*.png"
        "${CMAKE_SOURCE_DIR}/resources/images/*.jpg"
        "${CMAKE_SOURCE_DIR}/resources/images/*.exr"
)

message("Found pngs ${PNG_FILES}")

set(COPIED_PNG_FILES)

foreach (png ${PNG_FILES})
    file(RELATIVE_PATH relPath ${CMAKE_SOURCE_DIR}/resources/images ${png})
    set(outputfile "${CMAKE_BINARY_DIR}/resources/images/${relPath}")

    get_filename_component(outputDir ${outputfile} DIRECTORY)
    file(MAKE_DIRECTORY ${outputDir})

    add_custom_command(
            OUTPUT ${outputfile}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${png}
            ${outputfile}
            DEPENDS ${png}
            COMMENT "Copying image ${relPath}"
            VERBATIM
    )

    list(APPEND COPIED_PNG_FILES ${outputfile})
endforeach()

add_custom_target(CopyPngs ALL DEPENDS ${COPIED_PNG_FILES})
add_dependencies(gpvkfr CopyPngs)

add_custom_target(CopyImages ALL DEPENDS CopyPngs
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
        COMMENT "Copying resources to build"
)

file(GLOB_RECURSE GLTF_FILES
        "${CMAKE_SOURCE_DIR}/resources/models/*.gltf"
        "${CMAKE_SOURCE_DIR}/resources/models/*.glb"
        "${CMAKE_SOURCE_DIR}/resources/models/*.bin"
)

message("Found gltf files ${GLTF_FILES}")

set(COPIED_GLTF_FILES)

foreach (gltf ${GLTF_FILES})
    file(RELATIVE_PATH relPath ${CMAKE_SOURCE_DIR}/resources/models ${gltf})
    set(outputfile "${CMAKE_BINARY_DIR}/resources/models/${relPath}")

    get_filename_component(outputDir ${outputfile} DIRECTORY)
    file(MAKE_DIRECTORY ${outputDir})

    add_custom_command(
            OUTPUT ${outputfile}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${gltf}
            ${outputfile}
            DEPENDS ${gltf}
            COMMENT "Copying model ${relPath}"
            VERBATIM
    )

    list(APPEND COPIED_GLTF_FILES ${outputfile})
endforeach()

add_custom_target(CopyGltf ALL DEPENDS ${COPIED_GLTF_FILES})
add_dependencies(gpvkfr CopyGltf)

add_custom_target(CopyModels ALL DEPENDS CopyGltf
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
        COMMENT "Copying resources to build"
)
