CC = gcc
DEST = fp_diff
OBJ = fp_diff.o

$(DEST): $(OBJ)
	$(CC) -o $(DEST) $(OBJ)

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -rf $(DEST) $(OBJ)
