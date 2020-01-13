set(BINARY ${CMAKE_PROJECT_NAME}_xmllite)
set(SOURCES
qname.cc
qname.h
xmlbuilder.cc
xmlbuilder.h
xmlconstants.cc
xmlconstants.h
xmlelement.cc
xmlelement.h
xmlnsstack.cc
xmlnsstack.h
xmlparser.cc
xmlparser.h
xmlprinter.cc
xmlprinter.h
)

set(TESTSOURCES
qname_unittest.cc
unittest_main.cc
xmlbuilder_unittest.cc
xmlelement_unittest.cc
xmlnsstack_unittest.cc
xmlparser_unittest.cc
xmlprinter_unittest.cc
)

add_library(${BINARY} STATIC ${SOURCES})
target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_base expat)
add_executable(${BINARY}_test ${TESTSOURCES})
target_link_libraries(${BINARY}_test PRIVATE ${BINARY} gtest)
# add_test(NAME ${BINARY}_test COMMAND ${BINARY}_test)