server: digraph.cpp dijkstra.o digraph.o server.o
	g++ -o server server.o digraph.o dijkstra.o

dijkstra.o: dijkstra.cpp dijkstra.h
	g++ -c dijkstra.cpp

server.o: server.cpp dijkstra.o digraph.o
	g++ -c server.cpp

digraph.o: digraph.cpp digraph.h
	g++ -c digraph.cpp

clean:
	rm -f server *.o
	rm -f dijkstra *.o
	rm -f digraph *.o
	
run:
	./server