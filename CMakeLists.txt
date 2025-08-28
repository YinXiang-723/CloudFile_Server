cmake_minimum_required(VERSION 3.5)
PROJECT(tc_http_server)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找必要的系统包
find_package(Threads REQUIRED)

AUX_SOURCE_DIRECTORY(./ SRC_LIST)
AUX_SOURCE_DIRECTORY(./common COMMON_LIST)
AUX_SOURCE_DIRECTORY(./jsoncpp JSON_LIST)
AUX_SOURCE_DIRECTORY(./mysql MYSQL_LIST)
AUX_SOURCE_DIRECTORY(./redis REDIS_LIST)
AUX_SOURCE_DIRECTORY(./base BASE_LIST)
AUX_SOURCE_DIRECTORY(./api API_LIST)
AUX_SOURCE_DIRECTORY(./log LOG_LIST)

ADD_DEFINITIONS( -g -W -Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DAC_HAS_INFO
-DAC_HAS_WARNING -DAC_HAS_ERROR -DAC_HAS_CRITICAL -DTIXML_USE_STL
-DAC_HAS_DEBUG -DLINUX_DAEMON -std=c++14)

# 设置包含目录
INCLUDE_DIRECTORIES(/usr/include/fastdfs /usr/include/fastcommon 
                    /usr/local/include/hiredis /usr/include/mysql
                    /usr/local/include)
INCLUDE_DIRECTORIES(./ ./include)
INCLUDE_DIRECTORIES(./jsoncpp/include)
INCLUDE_DIRECTORIES(./mysql)
INCLUDE_DIRECTORIES(./redis)
INCLUDE_DIRECTORIES(./base)
INCLUDE_DIRECTORIES(./api)
INCLUDE_DIRECTORIES(./log)


#ADD_XXXX必须在TARGET_LINK_LIBRARIES前面，否则会报错
#ADD_LIBRARY(${PROJECTNAME} SHARED/STATIC ${SRC_LIST})
ADD_EXECUTABLE(tc_http_server  ${BASE_LIST} ${API_LIST} ${LOG_LIST}
        ${MYSQL_LIST} ${REDIS_LIST} ${JSON_LIST} ${SRC_LIST} ${COMMON_LIST})

# 链接库
TARGET_LINK_LIBRARIES(tc_http_server 
    pthread 
    fdfsclient 
    fastcommon 
    m 
    mysqlclient 
    hiredis 
    jsoncpp
    ${CMAKE_THREAD_LIBS_INIT}) 
