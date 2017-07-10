BIN=./bin
OBJ=./obj
SRC=./src

CC=gcc
CFLAGS=-Wall -Wextra -ansi -pedantic -I$(SRC)
LDFLAGS=

ALL: $(BIN)/alarm_timeout.exe $(BIN)/shutdown_timeout.exe $(BIN)/ping_timeout.exe

$(BIN)/alarm_timeout.exe: $(OBJ)/alarm_timeout.o $(OBJ)/timeout_utils.o
	@echo $@
	@$(CC) $(OBJ)/alarm_timeout.o $(OBJ)/timeout_utils.o -o $@ $(LDFLAGS) -lwinmm

$(BIN)/shutdown_timeout.exe: $(OBJ)/shutdown_timeout.o $(OBJ)/timeout_utils.o
	@echo $@
	@$(CC) $(OBJ)/shutdown_timeout.o $(OBJ)/timeout_utils.o -o $@ $(LDFLAGS)

$(BIN)/ping_timeout.exe: $(OBJ)/ping_timeout.o $(OBJ)/timeout_utils.o
	@echo $@
	@$(CC) $(OBJ)/ping_timeout.o $(OBJ)/timeout_utils.o -o $@ $(LDFLAGS) -lwinmm -liphlpapi -lws2_32

$(OBJ)/alarm_timeout.o : $(SRC)/alarm_timeout.c $(SRC)/timeout_utils.h
	@echo $@
	@$(CC) -c $(CFLAGS) $(SRC)/alarm_timeout.c -o $@

$(OBJ)/shutdown_timeout.o : $(SRC)/shutdown_timeout.c $(SRC)/timeout_utils.h
	@echo $@
	@$(CC) -c $(CFLAGS) $(SRC)/shutdown_timeout.c -o $@

$(OBJ)/ping_timeout.o: $(SRC)/ping_timeout.c $(SRC)/timeout_utils.h
	@echo $@
	@$(CC) -c $(CFLAGS) $(SRC)/ping_timeout.c -o $@

$(OBJ)/timeout_utils.o: $(SRC)/timeout_utils.c $(SRC)/timeout_utils.h
	@echo $@
	@$(CC) -c $(CFLAGS) $(SRC)/timeout_utils.c -o $@

clean:
	@echo clean
	@rm -f $(OBJ)/*.o $(BIN)/*.exe > NUL 2>&1
