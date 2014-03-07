Title:  Profiling `mpi-tile-io` benchmark  
Author: Yanlong Yin  
Date:   March 6th, 2014  

# Official `mpi-tile-io` website

Website: http://www.mcs.anl.gov/research/projects/pio-benchmark/

# Building `mpi-tile-io`

    $ make -f Makefile.linux

# Building `mpi-tile-io` with IOSIG

    $ make -f Makefile.linux.iosig

Check `Makefile.linux.iosig` for more details on enabling IOSIG.

# Running the benchmark

    $ mpirun -np 4 ./mpi-tile-io --nr_tiles_x 2 --nr_tiles_y 2 --sz_tile_x 256 --sz_tile_y 256 --sz_element 4096 --filename ${LOCAL_STORAGE}/datafile
    
Note: Datafile `${LOCAL_STORAGE}/datafile` should exist before running the benchmark. 

