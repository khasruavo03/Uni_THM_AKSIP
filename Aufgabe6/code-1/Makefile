CC = gcc
CFLAGS = -O2 -Wall
BIGINT_DIR = bigint/src
BIGINT_OBJ = $(BIGINT_DIR)/bigint.o
BIGINT_LIB = $(BIGINT_DIR)/libbigint.a

all: njvm-static

$(BIGINT_OBJ): $(BIGINT_DIR)/bigint.c $(BIGINT_DIR)/bigint.h $(BIGINT_DIR)/support.h
	$(CC) $(CFLAGS) -c -o $(BIGINT_OBJ) $(BIGINT_DIR)/bigint.c

$(BIGINT_LIB): $(BIGINT_OBJ)
	ar -crs $(BIGINT_LIB) $(BIGINT_OBJ)

njvm-static: njvm.c $(BIGINT_LIB)
	$(CC) $(CFLAGS) -static -o $@ njvm.c $(BIGINT_LIB)

njvm: njvm.c $(BIGINT_LIB)
	$(CC) $(CFLAGS) -o $@ njvm.c $(BIGINT_LIB)

clean:
	rm -f njvm njvm-static $(BIGINT_OBJ) $(BIGINT_LIB)
