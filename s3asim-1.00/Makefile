# Supported platforms
# LA-MPI, MPICH2

ifeq ($(PLATFORM), LA-MPI)
mpi_path = /opt/lampi/lampi-1.5.12/gm
cc      = gcc -I ${mpi_path}/include -L ${mpi_path}/lib
CFLAGS	= -Wall -Wstrict-prototypes -g
else	
mpi_path = /home/aching/pvfs2-lock/mpich2-1.0.5p3-mod
cc      = $(mpi_path)/bin/mpicc	
CFLAGS	= -Wall -Wstrict-prototypes -g
endif
EXECUTABLE = s3asim-nj s3asim-test

all: $(EXECUTABLE)

s3asim-j: src/*.c src/*.h
ifeq ($(PLATFORM), LA-MPI)
	$(cc) -DHAVE_MPE $(CFLAGS) -o s3asim-j src/*.c -lmpi -lmpe -llmpe -lm
else
	$(cc) -DHAVE_MPE $(CFLAGS) -o s3asim-j src/*.c -lmpe -llmpe -lm
endif

s3asim-nj: src/*.c src/*.h
ifeq ($(PLATFORM), LA-MPI)
	$(cc) $(CFLAGS) -o s3asim-nj src/*.c -lm -lmpi
else
	$(cc) $(CFLAGS) -o s3asim-nj src/*.c -lm
endif

s3asim-test: src/*.c src/*.h
ifeq ($(PLATFORM), LA-MPI)
	$(cc) -DTEST_MODE $(CFLAGS) -o s3asim-test src/*.c -lm -lmpi
else
	$(cc) -DTEST_MODE $(CFLAGS) -o s3asim-test src/*.c -lm
endif

clean:
	rm -f db/*~ scripts/*~ src/*~ src/*.o *.o *~ core $(EXECUTABLE) s3asim.clog2 s3asim.slog2
