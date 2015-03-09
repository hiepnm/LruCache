CC=gcc
CFLAGS=-O2 -g -ggdb -Wall -lpthread
OBJDIR=obj
OBJS=obj/lru.o obj/main.o
RM := rm -rf
MKDIR = mkdir -p
all: directories lrutest
directories: $(OBJDIR)
$(OBJDIR):
	$(MKDIR) $(OBJDIR)
obj/%.o: src/libs/%.c
	$(CC) $(CFLAGS) -c $< -o $@
obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@
lrutest: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
clean:
	-$(RM) $(OBJDIR) lrutest
