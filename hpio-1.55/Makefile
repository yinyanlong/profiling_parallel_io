# Please change mpi_dir to point to your MPI installation
mpi_dir = /home/aching/pvfs2-lock/mpich2-1.0.5p3-mod
cc	= ${mpi_dir}/bin/mpicc
CFLAGS	= -Wall -Wstrict-prototypes -g

EXECUTABLE = hpio hpio-small

all: $(EXECUTABLE)

# Object files
.c.o:
	$(cc) $(CFLAGS) -c $*.c -o $*.o

hpio: src/*.c src/*.h src/*/*.c src/*/*.h
	$(cc) $(CFLAGS) -o hpio src/*.c src/*/*.c -lm 

hpio-small: src/*.c src/*.h src/*/*.c src/*/*.h
	$(cc) $(CFLAGS) -DHPIO_SMALL -o hpio-small src/*.c src/*/*.c -lm

clean:
	rm -f scripts/*~ src/*.o src/*~ src/*/*~ *.o core $(EXECUTABLE) *~ t.*

distclean:
	rm -f scripts/*~ src/*.o cscope* src/cscope* src/*/cscope* src/*~ src/*/*~ *.o core $(EXECUTABLE) *~ t.* \#* src/\#* src/*/\#* scripts/\#* *.header *.info *.min_results *.results *.failed

outputclean:
	rm -f *.header *.info *.min_results *.results *.failed t.*