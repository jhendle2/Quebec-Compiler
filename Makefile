CC:=gcc
CFLAGS:=-Wall -std=gnu99

SRC:=src
OBJ:=obj

SRCS:=$(wildcard $(SRC)/*.c)
HDRS:=$(wildcard $(SRC)/*.h)
OBJS:=$(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))

INCLUDE:=-I$(SRC)

APP:=quebec

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(APP): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE)    -o $@ $(OBJS)

EXAMPLE:=examples/simplest.c
# EXAMPLE:=src/main.c
test: $(APP)
	./$(APP) -v -f $(EXAMPLE) -o a.out -r
# ./$(APP) -f $(EXAMPLE) -o a.out

valgrind: $(APP)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(APP) -v -f $(EXAMPLE) -o a.out
# ./$(APP) -ast $(EXAMPLE)
