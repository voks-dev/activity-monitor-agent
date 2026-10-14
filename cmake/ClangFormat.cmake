find_program(CLANG_FORMAT_EXECUTABLE
    NAMES clang-format clang-format-17 clang-format-16 clang-format-15
    DOC "Путь к исполняемому файлу clang-format"
)

if(NOT CLANG_FORMAT_EXECUTABLE)
    message(WARNING "clang-format не найден. Цели 'format' и 'check-format' недоступны.")
    return()
endif()

message(STATUS "Найден clang-format: ${CLANG_FORMAT_EXECUTABLE}")

file(GLOB_RECURSE ALL_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/include/*.h"
    "${CMAKE_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_SOURCE_DIR}/tests/*.cpp"
)

if(NOT ALL_SOURCE_FILES)
    message(WARNING "Не найдены исходные файлы для форматирования.")
    return()
endif()

list(LENGTH ALL_SOURCE_FILES SOURCE_FILE_COUNT)
message(STATUS "clang-format: найдено ${SOURCE_FILE_COUNT} файлов для обработки")

add_custom_target(format
    COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i ${ALL_SOURCE_FILES}
    COMMENT "Форматирование всех исходных файлов через clang-format"
    VERBATIM
)
