CC          = g++
CFLAGS      = -O3 -g --std=c++11

CPPHEADER   = btree.h rwmutex.h
CPPSOURCE   = btree.cc main.cc
EXECBIN     = btree
OBJECTS     = ${CPPSOURCE:.cpp=.o}

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${CC} ${CFLAGS} -pthread -o $@ $^
    
%.o : %.cpp
	${CC} ${CFLAGS} -c $<

.PHONY : clean

clean :
	rm ${OBJECTS} ${EXECBIN}
