
cmake_minimum_required (VERSION 2.6)
project ( SockServer )

include_directories("../../network")

SET (HEADER_FILES
  ../../network/server.h 
  ../../network/zmalloc.h 
  ../../network/ae.h 
  ../../network/anet.h 
  ../../network/config.h
)

SET (SOURCE_FILES
 ../sockServer.c
 ../../network/server.c 
 ../../network/worker.c 
 ../../network/zmalloc.c 
 ../../network/ae.c 
 ../../network/anet.c 
 ../../network/ae_epoll.c 
)


ADD_EXECUTABLE( SockServer ${HEADER_FILES}  ${SOURCE_FILES} )
TARGET_LINK_LIBRARIES( SockServer  )  
