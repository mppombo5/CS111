# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# ID: 405-140-036

madefiles = lab0.o lab0 lab0.tar.gz
tarfiles = lab0.c Makefile README SmokeTest.sh

# default target
default: lab0.o
	gcc lab0.o -o lab0 -Wall -Wextra

lab0.o: lab0.c
	gcc -c lab0.c

.PHONY: clean
clean:
	rm -rf $(madefiles)

.PHONY: check
check:
	bash SmokeTest.sh

.PHONY: dist
dist:
	tar -cvzf lab0.tar.gz $(tarfiles)
