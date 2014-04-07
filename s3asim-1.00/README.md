Title:  Profiling `s3asim` benchmark  
Author: Antonis Kougkas  
Date:   April 6th, 2014  

# Official `S3ASIM` website

Website: http://users.eecs.northwestern.edu/~aching/research_webpage/s3asim.html

# Building `S3ASIM`

    $ make
    
Check the Makefile to make sure you use the right `mpicc`.

# Building `S3ASIM` with IOSIG

    $ make -f Makefile.iosig

Check `Makefile.iosig` for more details on enabling IOSIG.

#List of options to use

    $ mpirun -np 1 ./s3asim-nj -h

Read README for more detailed test parameters

#Running the benchmark

    $ mpirun -np <number of processes> ./s3asim-nj


