# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036

# File categories
src = lab2_list.c SortedList.c SortedList.h
submit = Makefile README

# Submission stuff
UID = 405140036
tarname = lab2b-$(UID).tar.gz
tarfiles = $(src) $(submit)
madefiles = lab2_list $(tarname)

# Compiler stuff
CC = gcc
opts = -Wall -Wextra -pthread

default: $(src)
	$(CC) lab2_list.c SortedList.c -o lab2_list $(opts)

.PHONY: dist
dist: $(src) $(submit)
	tar -cvzf $(tarname) $(tarfiles)

.PHONY: clean
clean:
	rm -rf $(madefiles)
