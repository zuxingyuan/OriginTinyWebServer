CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2
endif

# MySQL 头文件和库路径
MYSQL_LIB = /usr/local/mysql/lib


server: main.cpp ./timer/lst_timer.cpp ./http/http_conn.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp  ./metrics/metrics.cpp webserver.cpp  config.cpp
	$(CXX) -o server $^ $(CXXFLAGS)  -L$(MYSQL_LIB) -lpthread -lmysqlclient
clean:
	rm -f server
