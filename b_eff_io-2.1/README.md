Title:  Profiling `b_eff_io` benchmark  
Author: Yanlong Yin  
Date:   March 6th, 2014  

# Official `b_eff_io` website

Website: https://fs.hlrs.de/projects/par/mpi//b_eff_io/

# Building `b_eff_io`

    $ mpicc -o b_eff_io b_eff_io.c -lm

# Building `b_eff_io` with IOSIG

    $ export IOSIG_CFLAGS="-w -finstrument-functions"
    $ export IOSIG_LDFLAGS="-Wl,-wrap,fopen,-wrap,fopen64,-wrap,fclose,-wrap,fread,-wrap,fwrite,-wrap,fseek,-wrap,open,-wrap,close,-wrap,read,-wrap,write,-wrap,lseek,-wrap,lseek64,-wrap,open64 -L${IOSIG_HOME}/src/collect -liosig"
    $ mpicc ${IOSIG_CFLAGS} -c b_eff_io.c
    $ mpicc -o b_eff_io b_eff_io.o -lm ${IOSIG_LDFLAGS}

# Running the benchmark

    $ mpirun -np 4 ./b_eff_io -MB 512 -MT 2048 -keep -N 4 -p ${LOCAL_STORAGE}
    
    
