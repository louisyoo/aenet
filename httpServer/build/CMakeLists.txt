
cmake_minimum_required (VERSION 2.6)
project ( httpServer )

include_directories("../../network")

SET (HEADER_FILES
  ../httpServer.h
  ../../network/server.h 
  ../../network/zmalloc.h 
  ../../network/ae.h 
  ../../network/anet.h 
  ../../network/config.h
)

SET (SOURCE_FILES
 ../httpServer.c
 ../../network/server.c 
 ../../network/worker.c 
 ../../network/zmalloc.c 
 ../../network/ae.c 
 ../../network/anet.c 
 ../../network/ae_epoll.c 
)


ADD_EXECUTABLE( httpServer ${HEADER_FILES}  ${SOURCE_FILES} )
TARGET_LINK_LIBRARIES( httpServer  )  
