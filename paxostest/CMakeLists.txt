cmake_minimum_required(VERSION 2.8.3)
project(paxostest)


## Use c++ 11x std
set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")



find_package(catkin REQUIRED roscpp)

catkin_package(
  INCLUDE_DIRS
  LIBRARIES
  CATKIN_DEPENDS
  DEPENDS
)

###########find_package(LIBEVENT REQUIRED)
set(LIBEVENT_ROOT "" CACHE STRING "LIBEVENT root directory")

find_path(LIBEVENT_INCLUDE_DIR event.h 
    HINTS "${LIBEVENT_ROOT}/include")

find_library(LIBEVENT_LIBRARY
   NAMES event
   HINTS "${LIBEVENT_ROOT}/lib")

set(LIBEVENT_LIBRARIES ${LIBEVENT_LIBRARY})
set(LIBEVENT_INCLUDE_DIRS ${LIBEVENT_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set LIBEVENT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LIBEVENT DEFAULT_MSG
                                  LIBEVENT_LIBRARY LIBEVENT_INCLUDE_DIR)

mark_as_advanced(LIBEVENT_INCLUDE_DIR LIBEVENT_LIBRARY)
######################
#find_package(MSGPACK REQUIRED)
set(MSGPACK_ROOT "" CACHE STRING "MessagePack root directory")

find_path(MSGPACK_INCLUDE_DIR msgpack.h HINTS "${MSGPACK_ROOT}/include")
find_library(MSGPACK_LIBRARY msgpack HINTS "${MSGPACK_ROOT}/lib")

set(MSGPACK_LIBRARIES ${MSGPACK_LIBRARY})
set(MSGPACK_INCLUDE_DIRS ${MSGPACK_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MSGPACK DEFAULT_MSG
                                  MSGPACK_LIBRARY MSGPACK_INCLUDE_DIR)
mark_as_advanced(MSGPACK_INCLUDE_DIR MSGPACK_LIBRARY)
##################
# find_package(BDB)
# find_package(LMDB)


# if (BDB_FOUND)
# 	add_definitions(-DHAS_BDB)
# endif()
# 
# if (LMDB_FOUND)
# 	add_definitions(-DHAS_LMDB)
# endif()

set(LIBPAXOS_LINKER_LIBS ${LIBPAXOS_LINKER_LIBS} m rt)


# add_subdirectory(paxos)

include_directories(paxos/include)

set(SRCS paxos/paxos.c paxos/acceptor.c paxos/learner.c paxos/proposer.c paxos/carray.c paxos/quorum.c
	paxos/storage.c paxos/storage_utils.c paxos/storage_mem.c)

add_library(paxos STATIC ${SRCS})
set_target_properties(paxos PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(paxos ${LIBPAXOS_LINKER_LIBS})


# 
# add_subdirectory(evpaxos)


include_directories(paxos/include)
include_directories(evpaxos/include)
include_directories(${LIBEVENT_INCLUDE_DIRS} ${MSGPACK_INCLUDE_DIRS})

set(LOCAL_SOURCES evpaxos/config.c evpaxos/message.c evpaxos/paxos_types_pack.c evpaxos/peers.c
	evpaxos/evacceptor.c evpaxos/evlearner.c evpaxos/evproposer.c evpaxos/evreplica.c)

add_library(evpaxos SHARED ${LOCAL_SOURCES})
set_target_properties(evpaxos PROPERTIES COMPILE_FLAGS "-fPIC")

target_link_libraries(evpaxos paxos ${LIBPAXOS_LINKER_LIBS} 
	${LIBEVENT_LIBRARIES} ${MSGPACK_LIBRARIES})

# set_target_properties(evpaxos PROPERTIES
# 	INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")

# INSTALL(TARGETS evpaxos DESTINATION lib)
# INSTALL(FILES include/evpaxos.h DESTINATION include)
# INSTALL(DIRECTORY include/evpaxos DESTINATION include)


# add_subdirectory(sample)

include_directories(paxos/include)
include_directories(evpaxos/include)
include_directories(${LIBEVENT_INCLUDE_DIRS})

# set(TEST_SOURCES sample/learner.c sample/acceptor.c sample/proposer.c sample/client.c sample/replica.c sample/counter.c)

add_executable(learner sample/learner.c)
set_target_properties(learner PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(learner evpaxos ${LIBPAXOS_LINKER_LIBS})

add_executable(acceptor sample/acceptor.c)
set_target_properties(acceptor PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(acceptor evpaxos ${LIBPAXOS_LINKER_LIBS})

add_executable(proposer sample/proposer.c)
set_target_properties(proposer PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(proposer evpaxos ${LIBPAXOS_LINKER_LIBS})

add_executable(client sample/client.c)
set_target_properties(client PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(client evpaxos ${LIBPAXOS_LINKER_LIBS})

add_executable(replica sample/replica.c)
set_target_properties(replica PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(replica evpaxos ${LIBPAXOS_LINKER_LIBS})

add_executable(counter sample/counter.c)
set_target_properties(counter PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(counter evpaxos ${LIBPAXOS_LINKER_LIBS})


# foreach(p ${TEST_SOURCES})
# 	get_filename_component(target "${p}" NAME_WE)
# 	add_executable(${target} ${p})
# 	target_link_libraries(${target} evpaxos ${LIBPAXOS_LINKER_LIBS})
# endforeach(p)




#enable_testing()
#add_subdirectory(unit)
