# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036

UID = 405140036
tarname = lab4c-$(UID).tar.gz
madefiles = lab4c $(tarname)
opts = -Wall -Wextra -lmraa

# file categories
src = lab4c.c
submission = Makefile README

default: $(src)
	gcc $(src) -o lab4c $(opts)

.PHONY: clean
clean:
	rm -rf $(madefiles)

.PHONY: dist
dist:
	tar -cvzf $(tarname) $(src) $(submission)
