CC          = g++
CFLAGS      = -O3 -g -pthread --std=c++11

CPPHEADER   = pc.h
CPPSOURCE   = pc.cc main.cc
EXECBIN     = pc
OBJECTS     = ${CPPSOURCE:.cc=.o}

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${CC} ${CFLAGS} -o $@ $^
    
%.o : %.cc
	${CC} ${CFLAGS} -c $<

.PHONY : clean

clean :
	rm ${OBJECTS} ${EXECBIN}
