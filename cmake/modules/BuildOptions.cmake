include(CheckCCompilerFlag)

option(ENABLE_COMPILER_CHECKING
       "Enable compiler extra checking"
       OFF)
option(ENABLE_SEPARATING_DEBUG_SYMBOLS
       "Enable separating debug symbols"
       OFF)
option(ENABLE_GOLD_LINKED
       "Enable gold linger"
       OFF)

if(ENABLE_COMPILER_CHECKING)
    if (MSVC)
        add_compile_options(/W4 /WX)
    else()
        add_compile_options(-Wall -Wextra -pedantic -Werror)
    endif()
endif()

if (ENABLE_SEPARATING_DEBUG_SYMBOLS)
    check_c_compiler_flag("-gsplit-dwarf" HAVE_SPLIT_DWARF)
    if (HAVE_SPLIT_DWARF)
        add_compile_options("$<$<CONFIG:Debug>:-gsplit-dwarf>")
    else()
        message(WARNING "Separating debug symbols not supported")
    endif()
endif()

if (ENABLE_GOLD_LINKED)
    add_link_options(-fuse-ld=gold)
endif()
