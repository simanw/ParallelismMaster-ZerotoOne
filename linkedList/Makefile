CC          = g++
CFLAGS      = -O3 -g --std=c++11 -pthread -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

CPPHEADER   = ll.h
CPPSOURCE   = ll.cpp main.cpp
EXECBIN     = ll
OBJECTS     = ${CPPSOURCE:.cpp=.o}

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${CC} ${CFLAGS} -o $@ $^
    
%.o : %.cpp
	${CC} ${CFLAGS} -c $<

.PHONY : clean

clean :
	rm ${OBJECTS} ${EXECBIN}
