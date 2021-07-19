all: rtu

rtu: 
	gcc rtu-ao4.c -o rtu-ao4 -lsqlite3 -lmodbus  

clean:
	rm -rf *.o rtu-ao4
