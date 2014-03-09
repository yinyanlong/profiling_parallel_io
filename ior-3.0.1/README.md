Title:  Profiling `ior` benchmark  
Author: Yanlong Yin  
Date:   March 6th, 2014  

# To get IOR

Official website: https://github.com/chaos/ior/releases

# To build the default IOR

Quoted from the official user guide:

> If "configure" is missing from the top level directory, you
   probably retrieved this code directly from the repository.
   Run "./bootstrap". If your versions of the autotools are not new enough to run
   this script, download and official tarball in which the
   configure script is already provided.

    $ ./configure --prefix=${INSTALL}
    $ make
    $ make install
    $ cd ./src; install ior ${INSTALL}/bin    # do this if last command failed
    $ ior -h                                  # make sure ${INSTALL}/bin is in $PATH
    
# To build IOR with IOSIG

    $ export IOSIG_CFLAGS="-w -finstrument-functions"
    $ export IOSIG_LDFLAGS="-Wl,-wrap,fopen,-wrap,fopen64,-wrap,fclose,-wrap,fread,-wrap,fwrite,-wrap,fseek,-wrap,open,-wrap,close,-wrap,read,-wrap,write,-wrap,lseek,-wrap,lseek64,-wrap,open64 -L${IOSIG_HOME}/src/collect"
    $ export CFLAGS="${CFLAGS} ${IOSIG_CFLAGS}"
    $ export LDFLAGS="${LDFLAGS} ${IOSIG_LDFLAGS}"
    $ export LIBS="-liosig"
    $ ./configure --prefix=${INSTALL} 

# To run with POSIX

    $ ior -a POSIX -b 24m -o /tmp/datafile -w -k -t 4k

# To run with MPIIO

Write some data:

    $ mpiexec -np 4 ior -a MPIIO -b 256m -o ${LOCAL_STORAGE}/datafile -w -k -t 64k

Read the data:

    # clear cache if necessary
    $ mpiexec -np 4 ior -a MPIIO -b 256m -o ${LOCAL_STORAGE}/datafile -r -k -t 64k

# To uninstall

    $ make uninstall



