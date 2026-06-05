sha256: sha256.o

clean:
	rm -f sha256 sha256.o

test: sha256
	./sha256
