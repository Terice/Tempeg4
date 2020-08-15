CC=gcc

CFLAGS+= -g -c
BIN=./
OBJ=./obj
target=a

LINUX_COLORED_TERMINAL:=n
ifeq ( $(LINUX_COLORED_TERMINAL) , y )
CFLAGS+=-DLINUX_COLORED_TERMINAL
endif

$(target):main.c $(OBJ)/box.o $(OBJ)/mpeg-4.o
	@echo "complie:start with the files"
	@$(CC) -g $^ -o $@ $(LDFLAGS)
	@echo "complie:end"
$(OBJ)/%.o:%.c
	@if [ ! -d $(OBJ) ]; then mkdir -p $(OBJ); fi;
	@$(CC) $(CFLAGS) $^ -o $@
clean:
	@if [ -d $(OBJ) ]; then rm $(OBJ)/*.o; fi;
	@rm $(BIN)/$(target)