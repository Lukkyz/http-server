hello: 
	gcc src/main.c src/hashmap.c src/http.c src/server.c -o main -luv
clean: 
	rm main
