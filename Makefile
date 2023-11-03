all: filetrap

filetrap: filetrap.c 
	gcc -g -o $@ $^

clean:
	rm filetrap
