CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2
endif

# MySQL 头文件和库路径
MYSQL_LIB = /usr/local/mysql/lib
BUNDLE_LIB = /root/Program/bundle


server: main.cpp \
./timer/lst_timer.cpp \
./http/http_conn.cpp \
./log/log.cpp \
./CGImysql/sql_connection_pool.cpp \
./metrics/metrics.cpp\
webserver.cpp \
config.cpp \
./Util/StorageConfig.cpp \
./Util/base64.cpp \
./Storage/DataManager.cpp \

	$(CXX) -o server $^ $(CXXFLAGS)  -L$(MYSQL_LIB) -lpthread -lmysqlclient -ljsoncpp -L$(BUNDLE_LIB) -lbundle -lstdc++fs
clean:
	rm -f server
