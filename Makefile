CC=gcc
CFLAGS=-Wall -Wextra -ansi -pedantic -I.
LDFLAGS=
BIN=./bin
OBJ=./obj

ALL: $(BIN)/alarm_timeout.exe $(BIN)/shutdown_timeout.exe

$(BIN)/alarm_timeout.exe: $(OBJ)/alarm_timeout.o $(OBJ)/timeout_utils.o
	@echo $@
	@$(CC) $(OBJ)/alarm_timeout.o $(OBJ)/timeout_utils.o -o $@ $(LDFLAGS) -lwinmm

$(BIN)/shutdown_timeout.exe: $(OBJ)/shutdown_timeout.o $(OBJ)/timeout_utils.o
	@echo $@
	@$(CC) $(OBJ)/shutdown_timeout.o $(OBJ)/timeout_utils.o -o $@ $(LDFLAGS)

$(OBJ)/alarm_timeout.o : alarm_timeout.c timeout_utils.h
	@echo $@
	@$(CC) -c $(CFLAGS) alarm_timeout.c -o $@

$(OBJ)/shutdown_timeout.o : shutdown_timeout.c timeout_utils.h
	@echo $@
	@$(CC) -c $(CFLAGS) shutdown_timeout.c -o $@

$(OBJ)/timeout_utils.o: timeout_utils.c timeout_utils.h
	@echo $@
	@$(CC) -c $(CFLAGS) timeout_utils.c -o $@

clean:
	@echo clean
	@rm -f $(OBJ)/*.o $(BIN)/*.exe > NUL 2>&1
