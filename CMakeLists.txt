cmake_minimum_required(VERSION 3.20)


include("arm-none-eabi-gcc.cmake")


message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")


###################### MACROS #########################################
MACRO(FILE_DIRECTORIES target_path return_list extention)
    set(FILE_DIRECTORIES_new_list "")
    FILE(GLOB_RECURSE FILE_DIRECTORIES_new_list "${target_path}/*.${extention}")
    SET(FILE_DIRECTORIES_dir_list "")
    FOREACH(file_path ${FILE_DIRECTORIES_new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(FILE_DIRECTORIES_dir_list "${FILE_DIRECTORIES_dir_list};${dir_path}")
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES FILE_DIRECTORIES_dir_list)
    SET(${return_list} ${FILE_DIRECTORIES_dir_list})
ENDMACRO()

MACRO(FILE_PATHS target_path return_list extention)
    set(files_list "")
    FILE(GLOB_RECURSE files_list "${target_path}/*.${extention}")
    LIST(REMOVE_DUPLICATES files_list)
    SET(${return_list} ${files_list})
ENDMACRO()

MACRO(EXCLUDE_PATHS target_list regexp_path_name)
    SET(EXCLUDE_PATHS_new_list "")
    message(STATUS "Regexp: \"^${CMAKE_CURRENT_SOURCE_DIR}.+${regexp_path_name}.*\"")
    FOREACH(curr_path ${${target_list}})
        if(NOT ${curr_path} MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}.+${regexp_path_name}.*$")
            message(STATUS "PATH ADDED: ${curr_path}")
            SET(EXCLUDE_PATHS_new_list "${EXCLUDE_PATHS_new_list};${curr_path}")
        else()
            message(STATUS "PATH REMOVED: ${curr_path}")
        endif()
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES EXCLUDE_PATHS_new_list)
    SET(${target_list} ${EXCLUDE_PATHS_new_list})
ENDMACRO()
#######################################################################


# Модификация устройства
# if(MODE)
#     message(STATUS "Set mode")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMODE")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMODE")
# endif()

# Название проекта
project(monitoring_module VERSION 0.1.0)
message(STATUS "Project version: ${CMAKE_PROJECT_VERSION}")
message(STATUS "Building ${PROJECT_NAME}")
message(STATUS "C FLAGS:   ${CMAKE_C_FLAGS}")
message(STATUS "C++ FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "ASM FLAGS: ${CMAKE_ASM_FLAGS}")

# Включаем Assembler
enable_language(ASM)

# Указываем исходные файлы проекта
FILE_PATHS("./Drivers" drivers_c_paths "c")
FILE_PATHS("./Modules" modules_c_paths "c")
FILE_PATHS("./Modules" modules_cpp_paths "cpp")
EXCLUDE_PATHS(modules_cpp_paths "test")
EXCLUDE_PATHS(modules_c_paths "test")
EXCLUDE_PATHS(modules_cpp_paths "build")
EXCLUDE_PATHS(modules_c_paths "build")
file(GLOB ${PROJECT_NAME}_CPP_SOURCES "./Core/Src/*.cpp")
file(GLOB ${PROJECT_NAME}_C_SOURCES "./Core/Src/*.c")
file(GLOB ${PROJECT_NAME}_S_SOURCES "./Core/Startup/*.s")
set(SOURCES "${drivers_c_paths};${modules_c_paths};${modules_cpp_paths};${${PROJECT_NAME}_CPP_SOURCES};${${PROJECT_NAME}_C_SOURCES};${${PROJECT_NAME}_S_SOURCES}")

# Добавляем исполняемый файл
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Указываем путь к файлу компоновщика
file(GLOB LINKER_SCRIPT_PATH "./*.ld")
set(LINKER_SCRIPT ${LINKER_SCRIPT_PATH})
set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# Указываем флаги компоновщика
set(TMP_LINKER_FLAGS ${CMAKE_LINKER_FLAGS})
set(CMAKE_EXE_LINKER_FLAGS "-T\"${LINKER_SCRIPT}\" ${TMP_LINKER_FLAGS}")
set(CMAKE_LINKER_FLAGS "-T\"${LINKER_SCRIPT}\" ${TMP_LINKER_FLAGS}")
message(STATUS "Linker: ${CMAKE_LINKER_FLAGS}")

# Указываем директории с заголовочными файлами
FILE_DIRECTORIES("./Drivers" drivers_h_dirs "h")
FILE_DIRECTORIES("./Modules" modules_h_dirs "h")
EXCLUDE_PATHS(modules_h_dirs "test")
set (INCLUDES  "./Core/Inc;${drivers_h_dirs};${modules_h_dirs}")
include_directories(${INCLUDES})
message(STATUS "PROJECT INCLUDES: ${INCLUDES}")
message(STATUS "PROJECT SOURCES: ${SOURCES}")

# Указываем дополнительные библиотеки, если необходимо
add_subdirectory("./Modules/Utils")
target_link_libraries(${PROJECT_NAME}.elf utilslib)
add_subdirectory("./Modules/StorageAT")
target_link_libraries(${PROJECT_NAME}.elf storageatlib)

# Добавляем команду для генерации bin файла
add_custom_command(
    TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ./${PROJECT_NAME}.elf ./${PROJECT_NAME}.bin
    COMMENT "Building ${PROJECT_NAME}.bin"
)