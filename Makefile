# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

IDIR =include/
CC=gcc
CFLAGS=-I$(IDIR)

SRC_DIR=src/

ODIR=obj/
BIN_DIR=../

LIBS= -lc #-lm -lnet

_DEPS = get_goals_rubi_csv.h # hellomake.h
DEPS = $(patsubst %,$(IDIR)%,$(_DEPS))

_OBJ =  get_goals_rubi_csv.o ggrc.o
OBJ = $(patsubst %,$(ODIR)%,$(_OBJ))


$(ODIR)%.o: $(SRC_DIR)%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ggrc: $(OBJ)
	gcc -o $(BIN_DIR)$@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

test:
	$(BIN_DIR)ggrc -h 145 -p 5:30.0 $(BIN_DIR)*.csv

clean:
	rm -f $(ODIR)*.o *~ core $(INCDIR)*~ 
	rm -f $(BIN_DIR)ggrc
