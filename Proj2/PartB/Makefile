# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036

# File categories
src = lab2_list.c SortedList.c SortedList.h
scripts = lab2_list.gp listcsv makecsv.sh
submit = Makefile README profile.out

# Submission stuff
UID = 405140036
tarname = lab2b-$(UID).tar.gz
tarfiles = $(src) $(submit) $(scripts)
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

.PHONY: dist
dist:
	tar -cvzf $(tarname) $(tarfiles)

.PHONY: clean
clean:
	rm -rf $(madefiles)
