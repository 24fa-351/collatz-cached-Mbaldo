colconj: colconj.c
	gcc -o colconj colconj.c

clean:
	rm colconj

test: colconj