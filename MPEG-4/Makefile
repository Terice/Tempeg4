CC=gcc

CFLAGS+= -g -c
BIN=./
OBJ=./obj
target=a


endif
$(target):main.c $(OBJ)/box.o $(OBJ)/mpeg-4.o $(OBJ)/fn.o $(OBJ)/tree.o
	@$(CC) -g $^ -o $@ $(LDFLAGS)
	@echo "complie:end"
$(OBJ)/%.o:%.c
	@if [ ! -d $(OBJ) ]; then mkdir -p $(OBJ); fi;
	@$(CC) $^ -o $@
clean:
	@if [ -d $(OBJ) ]; then rm $(OBJ)/*.o; fi;
	@rm $(BIN)/$(target)