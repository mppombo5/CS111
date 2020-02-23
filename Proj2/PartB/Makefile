# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036

# File categories
src = lab2_list.c SortedList.c SortedList.h
scripts = lab2b.gp listcsv makecsv.sh lab2b_list.csv
submit = Makefile README profile.out
pics = lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png

# Submission stuff
UID = 405140036
tarname = lab2b-$(UID).tar.gz
tarfiles = $(src) $(submit) $(scripts) $(pics)
madefiles = lab2_list $(tarname) lab2b.prof

# Compiler stuff
CC = gcc
opts = -Wall -Wextra -pthread

default: $(src)
	$(CC) lab2_list.c SortedList.c -o lab2_list $(opts)

.PHONY: tests
tests: default
	bash makecsv.sh > lab2b_list.csv

# necessary for the "profile" rule
profbuild: $(src)
	$(CC) lab2_list.c SortedList.c -o lab2_list $(opts) -g -lprofiler

.PHONY: profile
profile: profbuild
	CPUPROFILE=lab2b.prof ./lab2_list --iterations=1000 --threads=12 --sync=s
	pprof --text --lines ./lab2_list lab2b.prof

.PHONY: graphs
graphs: tests
	./lab2b.gp

.PHONY: dist
dist: default tests graphs
	tar -cvzf $(tarname) $(tarfiles)

.PHONY: clean
clean:
	rm -rf $(madefiles)
