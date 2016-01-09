FILE (GLOB_RECURSE project_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)
SET (project_BIN qzip)
SET (project_BIN qunzip)
SET (project_BIN qcat)

ADD_LIBRARY(quicklz SHARED quicklz.c)

LINK_DIRECTORIES(${quicklz}/lib)
ADD_EXECUTABLE(qzip qzip.c quicklz.c)
ADD_EXECUTABLE(qcat qzip.c quicklz.c)
ADD_EXECUTABLE(qunzip qzip.c quicklz.c)



TARGET_LINK_LIBRARIES(qzip quicklz)
TARGET_LINK_LIBRARIES(qcat quicklz)
TARGET_LINK_LIBRARIES(qunzip quicklz)

SET_TARGET_PROPERTIES(quicklz PROPERTIES VERSION "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}" OUTPUT_NAME quicklz CLEAN_DIRECT_OUTPUT 1)

INSTALL(FILES quicklz.h DESTINATION include)
INSTALL(TARGETS qzip qcat qunzip DESTINATION bin)
INSTALL(TARGETS quicklz DESTINATION lib)
INSTALL(FILES quicklz.h DESTINATION include)
