# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036

UID = 405140036
tarname = lab4c-$(UID).tar.gz
madefiles = lab4c_tcp lab4c_tls $(tarname)
opts = -Wall -Wextra -lm -std=gnu11 -lmraa
tls_opts = -lssl -lcrypto

# file categories
tcp_src = lab4c_tcp.c utilities.c
tls_src = lab4c_tls.c utilities.c
src = lab4c_tcp.c lab4c_tls.c utilities.c
headers = utilities.h
submission = Makefile README

default: $(src) $(headers)
	gcc $(tcp_src) -o lab4c_tcp $(opts)
	gcc $(tls_src) -o lab4c_tls $(opts) $(tls_opts)

.PHONY: clean
clean:
	rm -rf $(madefiles)

.PHONY: dist
dist:
	tar -cvzf $(tarname) $(src) $(submission) $(headers)
