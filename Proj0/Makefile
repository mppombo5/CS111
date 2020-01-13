# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# ID: 405140036

id = 405140036
madefiles = lab0 lab0-$(id).tar.gz
tarfiles = lab0.c Makefile README SmokeTest.sh breakpoint.png backtrace.png

# default target
default: lab0.c
	gcc -g lab0.c -o lab0 -Wall -Wextra

.PHONY: clean
clean:
	rm -rf $(madefiles)

.PHONY: check
check:
	bash SmokeTest.sh

dist: lab0
	tar -cvzf lab0-$(id).tar.gz $(tarfiles)
