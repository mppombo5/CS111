# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036

UID = 405140036
tarname = lab3b-$(UID).tar.gz
madefiles = $(tarname) lab3b

# file categories
src = lab3b.py
submission = Makefile README

default: $(src)
	rm -rf lab3b
	ln -s lab3b.py lab3b
	chmod +x lab3b

.PHONY: dist
dist:
	tar -cvzf $(tarname) $(src) $(submission)

.PHONY: clean
clean:
	rm -rf $(madefiles)
