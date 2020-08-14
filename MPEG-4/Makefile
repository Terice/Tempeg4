CC=gcc

CFLAGS+= -g -c
BIN=./
OBJ=./obj
target=a
$(target):main.c $(OBJ)/box.o $(OBJ)/mpeg-4.o
	$(CC) -g $^ -o $@ $(LDFLAGS)
$(OBJ)/%.o:%.c
	@if [ ! -d $(OBJ) ]; then mkdir -p $(OBJ); fi;
	$(CC) $(CFLAGS) $^ -o $@
clean:
	@if [ -d $(OBJ) ]; then rm $(OBJ)/*.o; fi;
	@rm $(BIN)/$(target)