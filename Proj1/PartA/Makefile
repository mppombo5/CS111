# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# ID: 405140036

tarfiles = lab1a.c Makefile README
madefiles = lab1a lab1a-$(UID).tar.gz
UID = 405140036

default: lab1a.c
	gcc lab1a.c -o lab1a -Wall -Wextra

.PHONY: clean
clean:
	rm -rf $(madefiles)

.PHONY: dist
dist:
	tar -cvzf lab1a-$(UID).tar.gz $(tarfiles)

