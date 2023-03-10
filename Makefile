hello: 
	gcc src/main.c src/hashmap.c -o main -luv
clean: 
	rm main
