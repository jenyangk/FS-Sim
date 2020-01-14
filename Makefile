CC      = g++
CFLAGS  = -Wall -Werror -std=c++11
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:%.c=%.o)

.PHONY: all clean

all: fs

clean:
	rm *.o fs

clean-all: clean

compress:
	zip fs-sim.zip readme.md *.cpp *.h Makefile

leak_check: 
	valgrind --tool=memcheck --leak-check=yes --track-origins=yes ./fs

%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@ -g

fs: $(OBJECTS)
	$(CC) $(CFLAGS) -o fs $(OBJECTS)