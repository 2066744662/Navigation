all:
	#make compile
	make run

run:	
	gnome-terminal -- ./server/server
	python3 client/client.py
	
clean:
	(cd server; make clean)
	rm -f inpipe
	rm -f outpipe

compile:
	(cd server; make)