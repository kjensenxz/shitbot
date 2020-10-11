#CFLAGS += -pg -ggdb -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wconversion
CC = gcc
CFLAGS += -std=gnu17 -ggdb -D __DEBUG__ 
CFLAGS += -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Wconversion -Wshadow
CFLAGS += -Wshadow -Wconversion
CFLAGS += -fsanitize=undefined -fsanitize=leak -fsanitize=address -fpie
LDLIBS =
OBJS = sandboxd.o net.o

sandboxd: $(OBJS)
	$(CC) $(CFLAGS) $(LDLIBS) $(OBJS) -o $@

sandboxd.o:

clean: 
	rm -f $(OBJS) sandbox

gprof: sandboxd
	gprof $@ gmon.out > analysis.txt
