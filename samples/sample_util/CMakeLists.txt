FILE(GLOB SAMPLE_UTIL_SOURCE *.cpp *.hpp *inl)
add_library(${SAMPLE_UTIL_PROJECT_NAME} STATIC ${SAMPLE_UTIL_SOURCE})
source_group(src FILES ${SAMPLE_UTIL_SOURCE})
target_link_libraries(${SAMPLE_UTIL_PROJECT_NAME} ${FIXIE_PROJECT_NAME})
set_target_properties(${SAMPLE_UTIL_PROJECT_NAME} PROPERTIES FOLDER samples)
