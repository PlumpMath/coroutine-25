include  ../make.rule

LIB=libbase.so
BIN=server 
UNITTESTBIN=event log task cli timer

COMPILE_FLAG += -DRUNTIME_LOG_ENABLE

default:${BIN}

event:
	g++ --std=c++0x Event.cpp Exception.cpp -o $@
log:
	g++ -D__LOGGER_UNITTEST --std=c++0x Logger.cpp Exception.cpp -o $@

libbase.so:Epoll.o Address.o Socket.o Exception.o Logger.o Task.o TCPServer.o
	${CXX} ${LDFLAG_SO} $^ -o $@

cli: client.cpp libbase.so
	${CXX} ${CXXFLAG} ${LDFLAG} $^ -o $@

server:Server.cpp  libbase.so
	${CXX} ${CXXFLAG} -D__SERVER_UNITTEST ${LDFLAG} $^ -o $@


task:Task.cpp libbase.so
	${CXX} ${CXXFLAG}  -D__TASK_UNITTEST -lpthread ${LDFLAG} $^ -o $@
	mkdir -p ~/bin && mv $@ ~/bin


timer:Timer.cpp libbase.so
	${CXX} ${CXXFLAG} -D__RUNTIME_TIMER_UNITTEST  ${LDFLAG} $^ -o $@


.PHONY: ${BIN} ${LIB} ${UNITTESTBIN}

clean::
	rm -f ${BIN} ${LIB} ${UNITTESTBIN}

