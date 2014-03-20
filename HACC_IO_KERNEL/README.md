Title:  Profiling `HACC_IO` benchmark  
Author: Yanlong Yin  
Date:   March 19th, 2014  

# Official `HACC_IO` website

Website: https://asc.llnl.gov/CORAL-benchmarks/#hacc

# Building the original benchmark

    $ make
    $ ./HACC_IO
     USAGE <exec> <particles/rank>  < Full file path>
    $ ./HACC_OPEN_CLOSE
     USAGE <exec> <particles/rank>  < Full file path>
    
# Building with IOSIG

    $ make -f Makefile.iosig

Check `Makefile.iosig` for more details on enabling IOSIG.

# Running the benchmark

    $ mpirun -np 4 ./HACC_IO 262144 $LOCAL_STORAGE
    $ mpiexec -np 4 ./HACC_OPEN_CLOSE 262144 $LOCAL_STORAGE
    

