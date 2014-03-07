Title:  Profiling `noncontig` benchmark  
Author: Yanlong Yin  
Date:   March 6th, 2014  

# Official `noncontig` website

Website: http://www.mcs.anl.gov/research/projects/pio-benchmark/

# Building `noncontig`

    $ make
    $ ./noncontig -help
    
Check the Makefile to make sure you use the right `mpicc`.

# Building `noncontig` with IOSIG

    $ make -f Makefile.iosig

Check `Makefile.iosig` for more details on enabling IOSIG.

# Running the benchmark

    $ mpirun -np 4 ./noncontig -fname ${LOCAL_STORAGE}/datafile -fsize 1024 -all -elmtcount 256 -veclen 16 -offset 0
    

