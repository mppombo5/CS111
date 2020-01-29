# NAME: Matthew Pombo
# ID: 405140036
# EMAIL: mppombo5@gmail.com

UID = 405140036
opts = -Wall -Wextra
tarfiles = lab1b-client.c lab1b-server.c README Makefile
madefiles = lab1b-client lab1b-server lab1b-$(UID).tar.gz

default: lab1b-client.c lab1b-server.c
	gcc lab1b-client.c -o lab1b-client $(opts)
	gcc lab1b-server.c -o lab1b-server $(opts)

.PHONY: dist
dist:
	tar -cvzf lab1b-$(UID).tar.gz $(tarfiles)

.PHONY: clean
clean:
	rm -rf $(madefiles)

