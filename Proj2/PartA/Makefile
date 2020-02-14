# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036

UID = 405140036
tarname = lab2a-$(UID).tar.gz
opts = -Wall -Wextra -pthread
tarfiles = Makefile lab2_add.c lab2_list.c SortedList.h SortedList.c
madefiles = lab2_add lab2_list $(tarname)

build: lab2_add lab2_list

lab2_add: lab2_add.c
	gcc lab2_add.c -o lab2_add $(opts)

lab2_list: lab2_list.c SortedList.c SortedList.h
	gcc lab2_list.c SortedList.c -o lab2_list $(opts)

.PHONY: dist
dist:
	tar -cvzf $(tarname) $(tarfiles)

.PHONY: clean
clean:
	rm -rf $(madefiles)
