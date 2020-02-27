# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036

UID = 405140036
tarname = lab4b-$(UID).tar.gz
opts = -Wall -Wextra -lmraa -lm

# file categories
Cfiles = lab4b.c utilities.c
Hfiles = utilities.h
src = $(Cfiles) $(Hfiles)
submission = Makefile README
tarfiles = $(src) $(submission) smoke.sh

# files created by program
madefiles = $(tarname) lab4b

default: $(src)
	gcc $(Cfiles) -o lab4b $(opts)

.PHONY: check
check: default
	bash smoke.sh

.PHONY: clean
clean:
	rm -rf $(madefiles)

# includes a last-minute replacement of my mraa.h dev files
# in case of fatal forgetfulness to replace the #include
.PHONY: dist
dist:
	tar -cvzf $(tarname) $(tarfiles)
