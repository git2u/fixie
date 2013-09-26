set(FIXIE_SOURCE
    color.hpp
    color.cpp
    debug.hpp
    context.cpp
    context.hpp
    fixie.cpp
    math_util.hpp
)

set(FIXIE_INCLUDE
    ../include/fixie.h
    ../include/fixie_gl_es.h
    ../include/fixie_gl_es_ext.h
    ../include/fixie_platform.h
)

add_library(fixie SHARED ${FIXIE_SOURCE} ${FIXIE_INCLUDE})

source_group(src FILES ${FIXIE_SOURCE})
source_group(include FILES ${FIXIE_INCLUDE})

set_target_properties(fixie PROPERTIES
    OUTPUT_NAME "fixie"
    CLEAN_DIRECT_OUTPUT 1
    VERSION ${FIXIE_VERSION}
)

add_definitions(-DFIXIE_EXPORT)