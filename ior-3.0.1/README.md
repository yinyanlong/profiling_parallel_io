Title: README for the project `profiling_parallel_io`  
Author: Yanlong Yin  
Date: March 6th, 2014  


# Building the default IOR

    $ ./configure --prefix=${INSTALL}
    $ make
    $ make install
    $ cd ./src; install ior ${INSTALL}/bin    # do this if last command failed
    $ ior -h                                  # make sure ${INSTALL}/bin is in $PATH
    
# Building IOR with IOSIG

    $ ./configure

# Run with POSIX

    $ ior -a POSIX -b 24m -o /tmp/datafile -w -k -t 4k

# Run with MPIIO

    $ mpiexec -np 4 ior -a MPIIO -b 24m -o /tmp/datafile -w -k -t 4k



