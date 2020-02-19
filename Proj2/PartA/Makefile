# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036

UID = 405140036
CC = gcc
tarname = lab2a-$(UID).tar.gz
opts = -Wall -Wextra -pthread
hw = Makefile README
src = lab2_add.c lab2_list.c SortedList.h SortedList.c
provided = lab2_add.gp lab2_list.gp
scripts = gencsv gencsvy listcsv addplots.sh listplots.sh
csv = lab2_add.csv lab2_list.csv
pics = lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png
tarfiles = $(hw) $(src) $(provide) $(scripts) $(csv) $(pics)
madefiles = lab2_add lab2_list $(tarname)

build: lab2_add lab2_list

lab2_add: lab2_add.c
	$(CC) lab2_add.c -o lab2_add $(opts)

lab2_list: lab2_list.c SortedList.c SortedList.h
	$(CC) lab2_list.c SortedList.c -o lab2_list $(opts)

.PHONY: tests
tests:
	bash addplots.sh
	bash listplots.sh

.PHONY: graphs
graphs:
	./lab2_add.gp
	./lab2_list.gp

.PHONY: dist
dist: build tests graphs
	rm -rf $(tarname)
	tar -cvzf $(tarname) $(tarfiles)

.PHONY: clean
clean:
	rm -rf $(madefiles)
