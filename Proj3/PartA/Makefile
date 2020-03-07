# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036

UID = 405140036
tarname = lab3a-$(UID).tar.gz
execname = lab3a
opts = -Wall -Wextra
CC = clang++

# File categories
src = lab3a.cpp ext2_fs.h
submission = Makefile README
madefiles = $(execname) $(tarname)

# Targets
default: $(src)
	$(CC) lab3a.cpp -o $(execname) $(opts)

.PHONY: dist
dist:
	tar -cvzf $(tarname) $(submission)

.PHONY: clean
clean:
	rm -rf $(madefiles)
