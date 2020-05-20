CC     = g++
CFLAGS = -g -pthread --std=c++11

CPPSOURCE = bank.cpp
EXECBIN = bank
OBJECTS = ${CPPSOURCE:.cc=.o}

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${CC} ${CFLAGS} -o $@ $^

%.o : %.cc
	${CC} ${CFLAGS} -c $<

.PHONY : clean

clean :
	rm ${OBJECTS} ${EXECBIN}