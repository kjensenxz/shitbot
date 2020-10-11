#CFLAGS += -pg -ggdb -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wconversion
CC = gcc
CFLAGS += -std=gnu17 -ggdb -D __DEBUG__ 
CFLAGS += -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Wconversion -Wshadow
CFLAGS += -Wshadow -Wconversion
#CFLAGS += -fsanitize=undefined -fsanitize=leak -fsanitize=address -fpie
LDLIBS =
OBJS = bot.o net.o parse.o

shitbot: $(OBJS)
	$(CC) $(CFLAGS) $(LDLIBS) $(OBJS) -o $@

bot.o:

clean: 
	rm -f $(OBJS) sandbox
