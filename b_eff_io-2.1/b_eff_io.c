/*--------------------------------------------------------------------*/
/*                                                                    */
/*  Effective I/O Bandwidth (b_eff_io) Benchmark                      */
/*                                                                    */
/*  Author: Rolf Rabenseifner                                         */
/*                                                                    */
/*  Copyright (c) Rolf Rabenseifner, HLRS, University of Stuttgart    */
/*                                                                    */
/*  Further information: www.hlrs.de/mpi/b_eff_io/                    */
/*                                                                    */
#define Revision "2.1"
/*                                                                    */
#define Date     "Dec. 12, 2001"
/*                                                                    */
/*--------------------------------------------------------------------*/

/* for ROMIO 1.0: #define WITHOUT_SHARED */

/* Compilation, execution and postprocessing -- examples:
 *
 * CRAY T3E:  Prerequisites: using moduls mpt
 *            Compilation on T3E :   
 *              cc -o b_eff_io -D WITHOUT_SHARED b_eff_io.c 
 *              cc -o b_eff_io -D WITHOUT_SHARED b_eff_io.c \
 *                 ../ufs_t3e/ad_ufs_open.o ../ufs_t3e/ad_ufs_read.o \
 *                 ../ufs_t3e/ad_ufs_write.o
 *              cc -o b_eff_io -D WITHOUT_SHARED b_eff_io.c ../ufs_t3e/ad_ufs_*.o 
 *            Execution: export MPI_BUFFER_MAX=4099 
 *            T3E-900 with 128 MB/processor and 512 PEs: 
 *              mpirun -np 64 ./b_eff_io -MB 128 -MT 65536 \
 *                     -p $SCRDIR -f b_eff_io_T3E900_064PE
 *            T3E-1200 with 512 MB/processor and 512: 
 *              mpirun -np <NP> ./b_eff_io -MB 512 -MT 262144 \
 *                     -p $SCRDIR -f b_eff_io_T3E1200_064PE
 *
 * SX-4:      Prerequisites: -
 *            Compilation on SX-4/32 with 256 MB/processor:
 *              mpicc -o b_eff_io b_eff_io.c -lm
 *            Execution: 
 *              mpirun -np 8 ./b_eff_io -MB 256 -MT 8192 \
 *                     -p $SCRDIR -f b_eff_io_SX4_08PE 
 *
 * Postprocessing (on local workstation):
 *              b_eff_io_eps 64 b_eff_io_T3E900_064PE 
 *              b_eff_io_eps 64 b_eff_io_T3E1200_064PE
 *              b_eff_io_eps  8 b_eff_io_SX4_08PE
 *
 * Outputfiles: b_eff_io_T3E900_064PE.sum       human readable summary
 *                                   .prot      full benchmark protocol
 *                                   _*_mono.eps   diagrams black/white
 *                                   _*_color.eps  colored, for slides
 *                                   _on1page.ps   summary sheet 
 *              Same for b_eff_io_T3E1200_064PE and b_eff_io_SX4_08PE.
 */


/* Additional options for debugging the b_eff_io benchmark:
    - compile time:
         -D WITHOUT_INFOFILE     removes the info-file handling
         -D WITHOUT_FILEACCESS   removes all write and read access
    - runtime
         -adapt                  another time adaptation algorithm
   All officially allowed options are described in the HELP output
   and man page. 
*/ 

#define T_CRIT        1800   /* see also HELP text */
#define T_PERCENTAGE  0.90   /* see also HELP text */
#ifndef MAX_INFOLINE_LNG     /* see also HELP text */
#  define MAX_INFOLINE_LNG 2000 
#endif 

#define HELP \
\
" Synopsis:                                                             \n\n"\
\
"  mpicc -o b_eff_io <compile time options> b_eff_io.c -lm              \n\n"\
\
"  mpirun -np <NP> ./b_eff_io <runtime options>                         \n\n"\
\
" General compile time options:                                         \n\n"\
\
"  -D WITHOUT_SHARED            to substitute the shared file pointer   \n"\
"                               by individual file pointers             \n"\
"                               (implies runtime option -noshared)      \n\n"\
\
" Runtime options:                                                      \n\n"\
\
"  -MB <number of megabytes memory per node>  (mandatory)               \n"\
"                               A node is defined as the unit used by or\n"\
"                               useable for one MPI process. This value \n"\
"                               is used to compute the maximum chunk    \n"\
"                               size for the patterns 1, 10, 18, 26 and \n"\
"                               35. The maximum chunk size is defined   \n"\
"                               as max( 2MB, memory per node / 128).    \n\n"\
\
"  -MT <number of megabytes memory of the total system> (mandatory)     \n"\
"                               This value is used to compute the       \n"\
"                               ratio of transferred bytes to           \n"\
"                               the size of the total memory.           \n\n"\
\
"  -noshared                    to substitute the shared file pointer   \n"\
"                               by individual file pointers in pattern  \n"\
"                               type 1 (implied by the compile time     \n"\
"                               option -D WITHOUT_SHARED).              \n\n"\
\
"  -nounique                    to remove MPI_MODE_UNIQUE_OPEN          \n"\
"                               on each file opening                    \n\n"\
\
"  -rewrite                     do rewrite between write and read       \n"\
"                               for all patterns                        \n\n"\
\
"  -keep                        to keep all benchmarking files          \n"\
"                               on close after last pattern test        \n\n"\
\
"  -N <number of processes>[,<number of processes>[,...]]               \n"\
"                               defines the partition sizes used        \n"\
"                               for this benchmark (default:            \n"\
"                               see Default Partition Sizes)            \n\n"\
\
"  -T <scheduled time>          scheduled time for all partitions of    \n"\
"                               processes N (default = 1800 [seconds],  \n"\
"                               see also option -N).                    \n\n"\
\
"  -p <path of fast_filesystem> path of the filesystem that should be   \n"\
"                               benchmarked, i.e.  where this bench-    \n"\
"                               marks should write its scratch files    \n"\
"                               (default is the current directory).     \n\n"\
\
"  -i <info_file>               file containing file hints, see         \n"\
"                               section Info File Format below          \n"\
"                               (default is to use no hints,            \n"\
"                               i.e., to use only MPI_INFO_NULL).       \n"\
"                               Using -i, the really used hints are     \n"\
"                               printed in the <prefix>.prot protocol   \n"\
"                               file. The default hints can be viewed   \n"\
"                               by using -i with an empty info-file.    \n\n"\
\
"  -e <number of errors>        maximum of errors printed in each       \n"\
"                               pattern (default = 1).                  \n\n"\
\
"  -f <protocol files' prefix>  prefix of the protocol file and         \n"\
"                               the summary file.                       \n"\
"                               The protocol and summary will be named  \n"\
"                                <prefix>.prot   and   <prefix>.sum     \n"\
"                               (default prefix = b_eff_io).            \n\n"\
\
" Remarks:                                                              \n"\
"   -- Already existing scratch files are automatically removed before  \n"\
"      benchmarking is started.                                         \n"\
"   -- If the result should be used for comparing different systems, the\n"\
"      benchmark is only valid if the following criterions are reached: \n"\
"       (1)   T >= 1800 sec,                                            \n"\
"       (2)   the option -noshared is NOT used, and                     \n"\
"       (3)   no errors are reported.                                   \n\n"\
\
" Default Partition Sizes:                                              \n\n"\
/* see default_size_rule.f */ \
\
"   By default, the benchmark is done with three partition sizes:       \n"\
"     - small  = 2 ** ( round ( log_2(#NODES) * 0.70 ) )                \n"\
"     - medium = 2 ** ( round ( log_2(#NODES) * 0.35 ) )                \n"\
"     - large  = #NODES,                                                \n"\
"   with #NODES = size of MPI_COMM_WORLD and the following exceptions:  \n"\
"   small=1 if #NODES==3, and medium=3 if #NODES==4.                    \n\n"\
\
"   Examples: The following table shows the medium and small sizes      \n"\
"             for given sizes of MPI_COMM_WORLD:                        \n\n"\
\
"   from #NODES=  1  2  4   5  12  20  32   87  142  232   625  1024    \n"\
"         until:  1  3  4  11  19  31  86  141  231  624  1023  1680    \n"\
"   ==> medium =  1  2  3   4   8   8  16   32   32   64   128   128    \n"\
"   ==> small  =  1  1  2   2   2   4   4    4    8    8     8    16    \n\n"\
\
"   from #NODES=  1681  4523   7420  12174 32768 53762  88205 237431    \n"\
"         until:  4522  7419  12173  32767 53761 88204 237430 389544    \n"\
"   ==> medium =   256   512    512   1024  2048  2048   4096   8192    \n"\
"   ==> small  =    16    16     32     32    32    64     64     64    \n\n"\
\
" Info File Format:                                                     \n\n"\
\
"   Each line of the info_file can be an empty line (only white space)  \n"\
"   or a comment line starting with # in the first column or must       \n"\
"   have the following syntax:                                          \n\n"\
\
"    <proc_num> <access> <pat_type> <chunk_size> <info_key> <info_value>\n\n"\
\
"   The items must be separated by white space (space or tab).          \n"\
"   String values must be typed without apostrophes. They are case      \n"\
"   sensitive. Allowed values are:                                      \n\n"\
\
"   - for <proc_num>   : the string 'all' or any number used with the   \n"\
"                        the option -N or used by the default of -N .   \n"\
"   - for <access>     : the string 'all', 'write', 'rewrite', or 'read'.\n"\
"   - for <pat_type>   : the string 'all', 'scatter', 'shared',         \n"\
"                        'separated', 'segmented', or 'seg_coll'.       \n"\
"   - for <chunk_size> : the string 'on_open' if the hint should be     \n"\
"                        used on opening the file, or the               \n"\
"                        string 'Lmax', or the chunk sizes 1024,        \n"\
"                        1032, 32768, 32776, 1048576, or 1048584 if the \n"\
"                        hint should be set with MPI_File_set_view or   \n"\
"                        MPI_File_set_info before writing or reading    \n"\
"                        the specific pattern with this chunk size.     \n"\
"                        'Lmax' denotes the maximum chunk size.         \n"\
"                        The string 'all' means that the hint is used   \n"\
"                        with MPI_File_open and additionally for each   \n"\
"                        pattern with MPI_File_set_view or              \n"\
"                        MPI_File_set_info.                             \n"\
"   - for <info_key>   : Any value according to the MPI-2 standard or   \n"\
"                        valid for the benchmarked MPI implementation.  \n"\
"                        Other values will be ignored without any       \n"\
"                        warning. The string <info_key> can not contain \n"\
"                        white space. The accepted values are reported  \n"\
"                        in the protocol file <prefix>.prot .           \n"\
"   - for <info_value> : The <info_value> may include any character.    \n"\
"                        Only trailing white space is removed.          \n"\
"                        The accepted values are reported in the        \n"\
"                        protocol file <prefix>.prot .                  \n\n"\
\
"   Each line must not have more than 2000 characters. This restriction \n"\
"   can be enlarged with the compile time option                        \n"\
"   -D MAX_INFOLINE_LNG=<length>                                        \n\n"\
\
"   Example:                                                            \n"\
"    # proc_num  access  pat_type  chunk_size  info_key            value\n"\
"           all     all   scatter         all  striping_factor       5  \n"\
"           all     all    shared         all  striping_factor       5  \n"\
"           all     all separated         all  striping_factor       1  \n"\
"           all     all segmented         all  striping_factor       5  \n"\
"           all     all  seg_coll         all  striping_factor       5  \n"\
"           all     all       all        1024  collective_buffering true\n"\
"           all     all       all        1032  collective_buffering true\n"\
"           all     all       all       32768  collective_buffering true\n"\
"           all     all       all       32776  collective_buffering true\n"\
"           all   write       all         all  access_style   write_once\n"\
"           all rewrite       all         all  access_style   write_once\n"\
"           all    read       all         all  access_style    read_once\n\n"\
\
" Postprocessing:                                                       \n\n"\
\
"   With b_eff_io_eps, one can select one section of summary protocol   \n"\
"   file and generate diagrams.                                         \n\n"\
\
"   Synopsis: b_eff_io_eps <number of processes> [ <protocol files' prefix> ]\n\n"\
\
"   Output: <protocol files' prefix>_write_mono.eps  (black/white, e.g.,\n"\
"           <protocol files' prefix>_rewrt_mono.eps   for publication)  \n"\
"           <protocol files' prefix>_read_mono.eps                      \n"\
"           <protocol files' prefix>_write_color.eps (color, thick lines,\n"\
"           <protocol files' prefix>_rewrt_color.eps  e.g., for slides) \n"\
"           <protocol files' prefix>_read_color.eps                     \n\n"\
\
" See also:  www.hlrs.de/mpi/b_eff_io/                                  \n"

#include "mpi.h"

/* --- for printing */
#include <stdio.h>
/* --- for pow(), log(), sqrt(): */
#include <math.h>
/* --- for fast malloc(), free(), time(): */
#include <sys/types.h>
/* --- for fast malloc(): */
#include <malloc.h>
/* --- for time(), ctime(): */
#include <time.h>
/* --- for drand48(), srand48(): */
#include <stdlib.h>
/* --- for uname(): */
#include <sys/utsname.h>
/* --- for strcmp(), strcat(), strlen(): */
#include <string.h>

#define ACCESSES   3           /* 0=write, 1=re-write, 2=read */
#define TYPE_0_PTRNS 9
#define TYPE_1_PTRNS 8
#define TYPE_2_PTRNS 8
#define TYPE_3_PTRNS 9
#define TYPE_4_PTRNS 9
#define PATTERNS  (TYPE_0_PTRNS+TYPE_1_PTRNS+TYPE_2_PTRNS+TYPE_3_PTRNS+TYPE_4_PTRNS)
#define TYPE_0_START 0
#define TYPE_1_START (TYPE_0_START+TYPE_0_PTRNS)
#define TYPE_2_START (TYPE_1_START+TYPE_1_PTRNS)
#define TYPE_3_START (TYPE_2_START+TYPE_2_PTRNS)
#define TYPE_4_START (TYPE_3_START+TYPE_3_PTRNS)
#define PAT_TYPES  5
#define POSITIONS  8

#define max(a,b) ( (a)>(b) ?   (a)  : (b) )
#define min(a,b) ( (a)<(b) ?   (a)  : (b) )

#define M_PART -1
#define MB     *1024*1024
#define kB     *1024
#define SAME   -1

#define ETYPE     double
#define MPI_ETYPE MPI_DOUBLE

#define N_list_DIM 10

main(int argc, char **argv) {
  int arg_error, MEMORY_PER_PROCESSOR, MT, SHARED, UNIQUE, REWRITE, DELETE;
  int ADAPT;
  int ierr, ierr2; 
  int T_MAX, N_list[N_list_DIM], N_list_lng=0, Ep_max;
  char *str, *PATH, *PREFIX;

  double T, Tn, Tu, Tp, sum_Tp, b_eff_io_Tn;
  double t, t_pat, sum_t_pat, t_type, t_access,
  t_partition[N_list_DIM][ACCESSES], t_total;
  double t_sync, t_open, t_close, t_barrier, t_bcast;
  double t_last_io, t_last_barr_bcast;
  int N,n,i_n, w_rank,n_rank, p,type_n,color,key, a,a_old, type,part,pos, r,r_max;
  int i, l, L, L_m_part, U, Usum, l_etype, Ep, Ep_sum, En[N_list_DIM], En_sum[N_list_DIM];
  MPI_Offset Lseg, L3, Spat, Stype, Spartition[N_list_DIM], Saccess[N_list_DIM];

  FILE *fp, *fp_short, *fp_all[3], *fpx;
  MPI_Comm n_comm;
  char filename[50], full_filename[150];
  MPI_File fh; 
  char conti;
  int gsizes[2], distribs[2], dargs[2], psizes[2];
  MPI_Datatype etype, filetype;
  MPI_Offset disp;
  MPI_Status sts;
  ETYPE *wr_buf, *rd_buf;

  MPI_Info info_open[N_list_DIM][ACCESSES][PAT_TYPES];
  MPI_Info info_view[N_list_DIM][ACCESSES][PATTERNS];
  int      info_v_st[N_list_DIM][ACCESSES][PATTERNS];
           /* info_v_st==0   ==>  info set with MPI_File_set_view
                       ==1   ==>  info set with MPI_File_set_info */
  char *info_filename = NULL;

  double rep[PATTERNS][N_list_DIM];
  double bw_pat,  bw_type,  bw_access,  bw_partition;
  double         wbw_type, wbw_access[N_list_DIM], wbw_partition, b_eff_io;
  int            wgh_type, wgh_access[N_list_DIM], wgh_partition, b_eff_io_n;
  double bw_x[N_list_DIM][ACCESSES][PAT_TYPES][POSITIONS+1], wbw_x[ACCESSES];
  double b_eff_io_ACCESSMETHOD_PATTERNTYPE[N_list_DIM][ACCESSES][PAT_TYPES];
  double b_eff_io_ACCESSMETHOD[N_list_DIM][ACCESSES], b_eff_io_PARTITION[N_list_DIM];
  double b_eff_io_ACCESS_WRT_REWRT[N_list_DIM];
  int b_eff_io_En;
  struct utsname  uname_info;
  time_t t_now;
  int crit1[N_list_DIM], crit2[N_list_DIM], crit3[N_list_DIM],
  crit_all[N_list_DIM], crit_all_n;

  int l_pos[POSITIONS] =
  { 0, 1 kB, 1 kB +8, 32 kB, 32 kB +8, 1 MB, 1 MB +8, M_PART };

  char *access_string[ACCESSES] = { "write", "rewrite", "read" };
  char *pattype_string[PAT_TYPES] =
  { "scatter", "shared", "separated", "segmented", "seg_coll" };

  struct {
    int    type;  /* 0 = strided & individual pointer
                     1 = strided & shared pointer
                     2 = individual files, non-collective
                     3 = segmented files, non-collective
                     4 = segmented files, collective */
    int       l;
    int       L;
    int       U;
    int    part;  /* 0 = first,  1 = middle,  2 = last */
    int     pos;  /* 0 = unused, 1..7 = output line    */
  }
  pattern_db[PATTERNS] =
  { /* type,        l,           L, U, part, pos */
    {     0,     1 MB,        1 MB, 0,    0,   0  },  
    {     0,   M_PART,        SAME, 4,    1,   7  },  
    {     0,     1 MB,        2 MB, 4,    1,   8  }, /*pos=8 - exception */
    {     0,     1 MB,        1 MB, 4,    1,   5  },
    {     0,    32 kB,        1 MB, 2,    1,   3  },
    {     0,     1 kB,        1 MB, 2,    1,   1  },
    {     0, 32 kB +8, 1 MB + 256 , 2,    1,   4  },  
    {     0,  1 kB +8, 1 MB + 8 kB, 2,    1,   2  },  
    {     0,  1 MB +8, 1 MB +   8 , 2,    2,   6  },  

    {     1,     1 MB,        SAME, 0,    0,   0  },  
    {     1,   M_PART,        SAME, 4,    1,   7  },  
    {     1,     1 MB,        SAME, 2,    1,   5  },  
    {     1,    32 kB,        SAME, 1,    1,   3  },  
    {     1,     1 kB,        SAME, 1,    1,   1  },  
    {     1, 32 kB +8,        SAME, 1,    1,   4  },  
    {     1,  1 kB +8,        SAME, 1,    1,   2  },  
    {     1,  1 MB +8,        SAME, 2,    2,   6  },  

    {     2,     1 MB,        SAME, 0,    0,   0  },  
    {     2,   M_PART,        SAME, 2,    1,   7  },  
    {     2,     1 MB,        SAME, 2,    1,   5  },  
    {     2,    32 kB,        SAME, 1,    1,   3  },  
    {     2,     1 kB,        SAME, 1,    1,   1  },  
    {     2, 32 kB +8,        SAME, 1,    1,   4  },  
    {     2,  1 kB +8,        SAME, 1,    1,   2  },  
    {     2,  1 MB +8,        SAME, 2,    2,   6  },  

    {     3,     1 MB,        SAME, 0,    0,   0  },  
    {     3,   M_PART,        SAME, 2,    1,   7  },  
    {     3,     1 MB,        SAME, 2,    1,   5  },  
    {     3,    32 kB,        SAME, 1,    1,   3  },  
    {     3,     1 kB,        SAME, 1,    1,   1  },  
    {     3, 32 kB +8,        SAME, 1,    1,   4  },  
    {     3,  1 kB +8,        SAME, 1,    1,   2  },  
    {     3,  1 MB +8,        SAME, 2,    1,   6  },  
    {     3,        0,        SAME, 0,    2,   0  }, /*(1)*/

    {     4,     1 MB,        SAME, 0,    0,   0  },  
    {     4,   M_PART,        SAME, 2,    1,   7  },  
    {     4,     1 MB,        SAME, 2,    1,   5  },  
    {     4,    32 kB,        SAME, 1,    1,   3  },  
    {     4,     1 kB,        SAME, 1,    1,   1  },  
    {     4, 32 kB +8,        SAME, 1,    1,   4  },  
    {     4,  1 kB +8,        SAME, 1,    1,   2  },  
    {     4,  1 MB +8,        SAME, 2,    1,   6  },  
    {     4,        0,        SAME, 0,    2,   0  }  /*(1)*/
  };

  /* (1) part==2 also implies, that r_max = 1 is choosen */

  MPI_Init( &argc, &argv);
  MPI_Barrier(MPI_COMM_WORLD); 
  t_total = MPI_Wtime();
  MPI_Comm_size(MPI_COMM_WORLD, &N);
  MPI_Comm_rank(MPI_COMM_WORLD, &w_rank);
  /* defaults: */ 
  MEMORY_PER_PROCESSOR = -1; 
  MT = -1; 
#   ifdef WITHOUT_SHARED
  SHARED = 0; 
#   else
  SHARED = 1; 
#   endif 
  UNIQUE = 1;
  REWRITE = 0;
  DELETE = 1; 
  T_MAX  = T_CRIT;
  PATH   = ".";  
  Ep_max = 1; 
  PREFIX = "b_eff_io"; 
  ADAPT  = 0; 

  arg_error = 0; 
  for (i=1; i < argc; i++) {
    str = argv[i];
    if (strcmp(str, "-MB") == 0) {
      i++;
      if (i < argc) {
        MEMORY_PER_PROCESSOR = atoi ( argv[i] ); 
        if (MEMORY_PER_PROCESSOR <= 0) { 
          arg_error++; 
          if (w_rank==0) printf( "Invalid argument for -MB\n"); 
        } 
      } else { 
        arg_error++; 
        if (w_rank==0) printf( "Missing argument for -MB\n"); 
      } 
    }
    else if (strcmp(str, "-MT") == 0) {
      i++;
      if (i < argc) {
        MT = atoi ( argv[i] ); 
        if (MT <= 0) { 
          arg_error++; 
          if (w_rank==0) printf( "Invalid argument for -MT\n"); 
        } 
      } else { 
        arg_error++; 
        if (w_rank==0) printf( "Missing argument for -MT\n"); 
      } 
    }
    else if (strcmp(str, "-noshared") == 0) {
      SHARED = 0; 
    }
    else if (strcmp(str, "-nounique") == 0) {
      UNIQUE = 0; 
    }
    else if (strcmp(str, "-rewrite") == 0) {
      REWRITE = 1; 
    }
    else if (strcmp(str, "-keep") == 0) {
      DELETE = 0; 
    }
    else if (strcmp(str, "-d") == 0) {
      DELETE = 1; 
    }
    else if (strcmp(str, "-adapt") == 0) {
      ADAPT = 1; /* NOT ALLOWED FOR BENCHMARKING */
    }
    else if (strcmp(str, "-N") == 0) {
      char *str; int n_partition;
      i++;
      if (i < argc) {
        str = argv[i]; 
        do {
          n_partition = atoi(str); 
          if ((n_partition <= 0) || (n_partition > N)) {
            arg_error++; 
            if (w_rank==0) printf( "Invalid argument for -N\n"); 
          } else {
            if (N_list_lng >= N_list_DIM) {
              arg_error++; 
              if (w_rank==0) printf( "Too many (>%1d) arguments with -N\n",N_list_DIM); 
            } else { 
              N_list[N_list_lng] = n_partition;
              N_list_lng++;
            } 
          } 
          while ((*str != '\0') && (*str != ',')) str++; 
          if (*str == ',') str++; /* read the delimiter */
        } while (*str != '\0');
      } else {
        arg_error++;
        if (w_rank==0) printf( "Missing argument for -N\n");
      }
    }
    else if (strcmp(str, "-T") == 0) {
      i++;
      if (i < argc) {
        T_MAX = atoi ( argv[i] );
        if (T_MAX <= 0) {
          arg_error++; 
          if (w_rank==0) printf( "Invalid argument for -T\n"); 
        } 
      } else { 
        arg_error++; 
        if (w_rank==0) printf( "Missing argument for -T\n"); 
      } 
    }
    else if (strcmp(str, "-p") == 0) {
      i++;
      if (i < argc) {
        PATH = argv[i];
      } else {
        arg_error++;
        if (w_rank==0) printf( "Missing argument for -p\n");
      }
    }
    else if (strcmp(str, "-i") == 0) {
      i++;
      if (i < argc) {
        info_filename = argv[i];
      } else {
        arg_error++;
        if (w_rank==0) printf( "Missing argument for -o\n");
      } 
    }
    else if (strcmp(str, "-e") == 0) {
      i++;
      if (i < argc) {
        Ep_max = atoi ( argv[i] ); 
        if (Ep_max <= 0) { 
          arg_error++; 
          if (w_rank==0) printf( "Invalid argument for -e\n"); 
        } 
      } else { 
        arg_error++; 
        if (w_rank==0) printf( "Missing argument for -e\n"); 
      } 
    }
    else if (strcmp(str, "-f") == 0) {
      i++;
      if (i < argc) {
        PREFIX = argv[i]; 
      } else { 
        arg_error++; 
        if (w_rank==0) printf( "Missing argument for -f\n"); 
      } 
    }
    else {
      arg_error++; 
      if (w_rank==0) printf( "Invalid option %s\n", str);
    } 

  } /* endfor (i=1; i < argc; i++) */  

  /* some defaults: */ 
  if (N_list_lng == 0) {
    if (N==1) {
      N_list_lng = 1;   N_list[0] = N; 
    }else if (N==2) { 
      N_list_lng = 2;   N_list[0] = 1;  N_list[1] = N;
    }else{ 
      N_list_lng = 3;
      /* small  = 2 ** ( round ( log_2(#NODES) * 0.70 ) ) */
      /* medium = 2 ** ( round ( log_2(#NODES) * 0.35 ) ) */
      /* round (x) := (int)(0.5+x) */
      /* small=1 if #NODES==3, and medium=3 if #NODES==4 */
      N_list[0] = (int)(0.5+pow(2.0, (double)((int)(0.5000001+ log((double)N)/log(2.0) * 0.35 ))));
      if (N==3) N_list[0] = 1;
      N_list[1] = (int)(0.5+pow(2.0, (double)((int)(0.5000001+ log((double)N)/log(2.0) * 0.70 ))));
      if (N==4) N_list[1] = 3;
      N_list[2] = N;
    } 
  }

  /***************************************************************************/
  /* initialize infos */
  for (i_n=0; i_n<N_list_DIM; i_n++)
    for (a=0; a<ACCESSES; a++) {
      for (type=0; type<PAT_TYPES; type++) {
        info_open[i_n][a][type] = MPI_INFO_NULL;
      }
      for (p=0; p<PATTERNS; p++) {
        info_view[i_n][a][p] = MPI_INFO_NULL;
        info_v_st[i_n][a][p] = 0;
      }
    }

  /*  hint-file-reading                                                       */
  if (info_filename) {
#  ifdef WITHOUT_INFOFILE
    printf("can't use option -i %s, because the benchmark was compiled with WITHOUT_INFOFILE\n",info_filename);
    arg_error++;
#  else  /* INFOFILE allowed */ 
    char line[MAX_INFOLINE_LNG+1];
    int  a1, a2, p1, p2, n1, n2, infoline_correct, line_num, r;
    char *pos, *num, *acc, *pat, *len, *info_key, *info_value; 
    if ((fp=fopen(info_filename,"r"))==NULL) {
      printf("%s: can't open %s\n", argv[0], info_filename);
      arg_error++; 
    }
    if (fp != NULL) { 
     r=~EOF;
     line_num=0; 
     while (r!=EOF) {
      line_num++;
      infoline_correct = 0;
 
      /* read one line */
      for (i=0; i<=MAX_INFOLINE_LNG; i++) {
        r=fgetc(fp);
        if ((r==EOF) || (r=='\n'))  break;
        else line[i] = r;
      }   
      if (i == (MAX_INFOLINE_LNG+1)) {
        arg_error++; 
        if (w_rank==0) printf("error in info-file %s, line %d: line too long, more than %d characters\n",info_filename,line_num,MAX_INFOLINE_LNG);
        while ((r!=EOF) && (r!='\n')) r=fgetc(fp);  
      }
 
      /* remove trailing white space */ 
      while ( (i>0) && ( (line[i-1] == '\t') 
                         || (line[i-1] ==  ' ') ) ) i--;
      line[i] = '\0';
 
      if ((i>0) && (line[0] != '#')) { /* real info line */
 
#ifdef DEBUG_INFOFILE
        if (w_rank==0) {
          printf("DEBUG INFOFILE A: [%d]-%s\n", line_num, line); fflush(stdout);
        }
#endif
 
        infoline_correct = 0;
 
        pos = line;
        while ( ((*pos)!='\0') && (((*pos)=='\t') || ((*pos)==' ')) ) pos++;
 
        num = pos;
        while (((*pos)!='\0') && ((*pos)!='\t') && ((*pos)!=' ')) pos++;
        if ((*pos)!='\0') {*pos = '\0'; pos++;} 
        while ( ((*pos)!='\0') && (((*pos)=='\t') || ((*pos)==' ')) ) pos++;
 
        acc = pos;
        while (((*pos)!='\0') && ((*pos)!='\t') && ((*pos)!=' ')) pos++;
        if ((*pos)!='\0') {*pos = '\0'; pos++;} 
        while ( ((*pos)!='\0') && (((*pos)=='\t') || ((*pos)==' ')) ) pos++;
 
        pat = pos;
        while (((*pos)!='\0') && ((*pos)!='\t') && ((*pos)!=' ')) pos++;
        if ((*pos)!='\0') {*pos = '\0'; pos++;} 
        while ( ((*pos)!='\0') && (((*pos)=='\t') || ((*pos)==' ')) ) pos++;
 
        len = pos;
        while (((*pos)!='\0') && ((*pos)!='\t') && ((*pos)!=' ')) pos++;
        if ((*pos)!='\0') {*pos = '\0'; pos++;} 
        while ( ((*pos)!='\0') && (((*pos)=='\t') || ((*pos)==' ')) ) pos++;
 
        info_key = pos;
        while (((*pos)!='\0') && ((*pos)!='\t') && ((*pos)!=' ')) pos++;
        if ((*pos)!='\0') {*pos = '\0'; pos++;} 
        while ( ((*pos)!='\0') && (((*pos)=='\t') || ((*pos)==' ')) ) pos++;
         
        info_value = pos;
           
 
#ifdef DEBUG_INFOFILE
        if (w_rank==0) printf("DEBUG INFOFILE B: [%d]:%s:%s:%s:%s:%s:%s:\n", line_num, 
                        num, acc, pat, len, info_key, info_value); fflush(stdout);
#endif

        if (strlen(info_key) > MPI_MAX_INFO_KEY) info_key[0] = '\0';

        if (info_key[0] != '\0') {

          n1=0; n2=0;
          if (strcmp(num, "all") == 0) {
            n2 = N_list_lng;
          } else {
            n=atoi(num);
            while (n1 < N_list_lng)
              if (n != N_list[n1])
                n1++;
              else {
                n2=n1+1;
                break;
              }
          }

          a1=0; a2=0;
          if (strcmp(acc, "all") == 0) { a1=0; a2=3; }
          else if (strcmp(acc, "write") == 0) { a1=0; a2=1; }
          else if (strcmp(acc, "rewrite") == 0) { a1=1; a2=2; }
          else if (strcmp(acc, "read") == 0) { a1=2; a2=3; }

          p1=0; p2=0;
          if (strcmp(pat, "all") == 0) 
            p2=PATTERNS;
          else if (strcmp(pat, "scatter") == 0) {
            p1=TYPE_0_START; p2=TYPE_1_START;
          } else if (strcmp(pat, "shared") == 0) {
            p1=TYPE_1_START; p2=TYPE_2_START;
          } else if (strcmp(pat, "separated") == 0) {
            p1=TYPE_2_START; p2=TYPE_3_START;
          } else if (strcmp(pat, "segmented") == 0) {
            p1=TYPE_3_START; p2=TYPE_4_START;
          } else if (strcmp(pat, "seg_coll") == 0) {
            p1=TYPE_4_START; p2=PATTERNS;
          }

#ifdef DEBUG_INFOFILE
          if (w_rank==0) printf("DEBUG INFOFILE C: [%d] num %d-%d / access %d-%d / pat %d-%d\n", 
                                   line_num, n1,n2, a1,a2, p1,p2);
#endif

          for (i_n=n1; i_n<n2; i_n++)
            for (a=a1; a<a2; a++)
              for (p=p1; p<p2; p++) {
               if ((strcmp(len, "on_open") == 0)   /* for set info_open */
                   || (strcmp(len, "all") == 0)) { 
                 if (pattern_db[p].part == 0) { /* store it only once*/
                   type = pattern_db[p].type;
#ifdef DEBUG_INFOFILE
                   if (w_rank==0) printf("DEBUG INFOFILE D: [%d] on_open  n=%d  a=%d  type=%d : %s : %s :\n",
                                            line_num,i_n,a,type, info_key, info_value); 
#endif
                   if (info_open[i_n][a][type] == MPI_INFO_NULL)
                     MPI_Info_create(&info_open[i_n][a][type]);
                   MPI_Info_set(info_open[i_n][a][type], info_key, info_value);
                   infoline_correct = 1; 
                 } 
               } /* if ... for set info_open */
               if ((strcmp(len, "all") == 0) ||    /* for set info_view */
                    ( (strcmp(len, "Lmax") == 0) && (pattern_db[p].l == M_PART) ) ||
                    (((*len)>='0')&&((*len)<='9')&&(atoi(len) == pattern_db[p].l))) {
                  if (info_view[i_n][a][p] == MPI_INFO_NULL)
                    MPI_Info_create(&info_view[i_n][a][p]);
                  MPI_Info_set(info_view[i_n][a][p], info_key, info_value);
                  infoline_correct = 1; 
               } /*endif ... for set info_view */ 
              } /*endfor p */
            /*endfor a */
          /*endfor i_n */
        } /* end if (info_key...) */
 
        if (!infoline_correct) {
          arg_error++; 
          if (w_rank==0) {
            printf("error in info-file %s, line %d: line ignored",info_filename,line_num);
            if (n2 <= n1) printf(" - invalid number of processes");
            if (a2 <= a1) printf(" - invalid access value");
            if (p2 <= p1) printf(" - invalid pattern");
            if ((n1<n2) && (a1<a2) && (p1<p2) && (info_key[0] != '\0'))
                           printf(" - invalid chunk size"); 
            if (info_key[0] == '\0') printf(" - empty info key has more than %d (MPI_MAX_INFO_KEY) characters", MPI_MAX_INFO_KEY);
            printf("\n");
          } 
        }
      } /* end if ... real info line */
     } /* end while (r!=EOF) */
     fclose(fp);
    } /* if (fp != NULL)    */
#  endif /* INFOFILE allowed */ 
  } /* if (info_filename) */

  /***************************************************************************/
  /* print info */
#ifdef DEBUG_INFOFILE
  if (w_rank==0) {
    char info_key[MPI_MAX_INFO_KEY+1], *info_value;
    int i, r, valuelen = 0;
    printf("printing infos \n");
    for (i_n=0; i_n<N_list_lng; i_n++) {
      for (a=0; a<ACCESSES; a++) {
        for (p=0; p<PATTERNS; p++) {
          type = pattern_db[p].type; 
          if (pattern_db[p].part == 0) {
           /* open info */
           if (info_open[i_n][a][type] == MPI_INFO_NULL) {
             printf("OPN : %4d : %7s : %9s : on_open : <no key> : <no value> :\n",
                N_list[i_n], access_string[a], pattype_string[type]);
           }else{
            MPI_Info_get_nkeys(info_open[i_n][a][type], &i);
            for (r=0; r<i; r++) {
                MPI_Info_get_nthkey(info_open[i_n][a][type], r, info_key);
                MPI_Info_get_valuelen(info_open[i_n][a][type], info_key, &valuelen, &l);
                if (l) {
                info_value = (char*)malloc(valuelen+1);
                MPI_Info_get(info_open[i_n][a][type], info_key, valuelen, info_value, &l);
                printf("OPN : %4d : %7s : %9s : on_open : %s : %s :\n", N_list[i_n],
                    access_string[a], pattype_string[type], info_key, info_value);
                free(info_value); 
                }
            }
           }
          } /*endif (pattern_db[p].part == 0) */ 
          /* view info */
          if (info_view[i_n][a][p] == MPI_INFO_NULL) {
            printf("p%02d : %4d : %7s : %9s ", p,
                N_list[i_n], access_string[a], pattype_string[type]);
            if (pattern_db[p].l == M_PART) printf(":    Lmax ");
            else printf(": %7d ", pattern_db[p].l);
            printf(": <no key> : <no value> :\n"); 
          }else{
            MPI_Info_get_nkeys(info_view[i_n][a][p], &i);
            for (r=0; r<i; r++) {
              MPI_Info_get_nthkey(info_view[i_n][a][p], r, info_key);
              MPI_Info_get_valuelen(info_view[i_n][a][p], info_key, &valuelen, &l);
              if (l) {
                info_value = (char*)malloc(valuelen+1);
                MPI_Info_get(info_view[i_n][a][p], info_key, valuelen, info_value, &l);
                printf("p%02d : %4d : %7s : %9s ", p, 
                    N_list[i_n], access_string[a], pattype_string[type]);
                if (pattern_db[p].l == M_PART) printf(":    Lmax ");
                else printf(": %7d ", pattern_db[p].l);
                printf(": %s : %s :\n", info_key, info_value);
                free(info_value); 
              }
            }
          }
        } /* end for p */
      } /* end for a */
    } /* end for i_n */
    printf("printing done\n");
  } /*if (w_rank==0)*/
#endif /* DEBUG_INFOFILE */

  /***************************************************************************/

  if (MEMORY_PER_PROCESSOR < 0) {
    arg_error++; 
    if (w_rank==0) 
      printf( "Missing mandatory argument -MB <memory per process> [in MBytes]\n");
  } 
  if (MT < 0) {
    arg_error++; 
    if (w_rank==0) 
      printf( "Missing mandatory argument -MT <total memory> [in MBytes]\n");
  } 
  if (arg_error > 0) {
    if (w_rank==0) printf( "Aborted due to argument errors.\n\n"); 
    if (w_rank==0) printf(HELP); 
    MPI_Finalize();
    return(1);
  }

  /* running the benchmark */

  /*
   * Definition: L_m_part := MEMORY_PER_PROCESSOR MB/128 
   *           AND in the range  2 MB .. 1024 MB
   *
   * Caution:   1024 MB is the maximal positiv power of 2, that can
   *     be stored into 32 bit int variables
   */

  L_m_part = 1 MB/128 * max(2*128, min(MEMORY_PER_PROCESSOR, 1024*128));

  if (w_rank == 0)
  {
    sprintf(filename,"%s.prot",PREFIX); fp = fopen(filename,"w");
    fp_all[0] = fp;
    sprintf(filename,"%s.sum",PREFIX); fp_short = fopen(filename,"w");
    fp_all[1] = fp_short;
    fp_all[2] = stdout;
    uname(&uname_info);
    t_now = time((time_t *) 0);
    for (i=0; i<2; i++)
    { fpx = fp_all[i];
      fprintf(fpx,"b_eff_io.c, Revision %s from %s\n\n", Revision, Date);
      fprintf(fpx,"MEMORY_PER_PROCESSOR = %4d MBytes  [1MBytes = 1024*1024 bytes, 1MB = 1e6 bytes]\n",
          MEMORY_PER_PROCESSOR);
      fprintf(fpx,"Maximum chunk size   = %8.3f MBytes\n\n",
          L_m_part/(1.0 MB));
      {
        int j;
        fprintf(fpx,"-N ");
        for (j=0; j<N_list_lng; j++)
          fprintf(fpx,"%c%d",(j==0 ? ' ':','),N_list[j]);
      }
      fprintf(fpx," T=%1d, MT=%1d MBytes", T_MAX, MT);
#     ifdef WITHOUT_INFOFILE
        fprintf(fpx,", -D WITHOUT_INFOFILE");
#     endif
      if (info_filename) 
        fprintf(fpx," -i %s", info_filename);
#     ifdef WITHOUT_SHARED
        fprintf(fpx,", -D WITHOUT_SHARED");
#     endif
      if (!SHARED) fprintf(fpx,", -noshared");
      if (!UNIQUE) fprintf(fpx,", -nounique");
      if (REWRITE) fprintf(fpx,", -rewrite");
      if (!DELETE) fprintf(fpx,", -keep");
      if (ADAPT)   fprintf(fpx,", -adapt (NOT ALLOWED FOR BENCHMARKING)");
      fprintf(fpx,"\n");
      fprintf(fpx,"PATH=%s, PREFIX=%s\n", PATH, PREFIX);
      fprintf(fpx,"       system name : %s\n", uname_info.sysname);
      fprintf(fpx,"       hostname    : %s\n", uname_info.nodename);
      fprintf(fpx,"       OS release  : %s\n", uname_info.release);
      fprintf(fpx,"       OS version  : %s\n", uname_info.version);
      fprintf(fpx,"       machine     : %s\n\n", uname_info.machine);
      fprintf(fpx,"Date of measurement: %s\n\n", ctime(&t_now));
    }
  }

  etype = MPI_ETYPE;
  l_etype = sizeof(ETYPE);

  if ( !( wr_buf = (ETYPE *)malloc((size_t)L_m_part) ) ||
      !( rd_buf = (ETYPE *)malloc((size_t)L_m_part) ) ) {
    printf("[%d] ERROR - not enough space for buffers - is MEMORY_PER_PROCESSOR = %d MByte correct?\n", w_rank, MEMORY_PER_PROCESSOR);
    MPI_Abort(MPI_COMM_WORLD,1);
  }
  for (i=0; i<L_m_part/l_etype; i++) {
    wr_buf[i] = -2*i; rd_buf[i] = -2*i-1;
  }

  for (p=0; p<PATTERNS; p++) {
    if (pattern_db[p].l == M_PART) pattern_db[p].l = L_m_part;
    if (pattern_db[p].L == SAME)   pattern_db[p].L = pattern_db[p].l;
    pattern_db[p].L = pattern_db[p].L / l_etype; 
    pattern_db[p].l = pattern_db[p].l / l_etype; 
  }

  Usum = 0;
  for (a=0; a<ACCESSES; a++) {
    type_n = ((!REWRITE && (a==1)) ? TYPE_0_PTRNS : PATTERNS);
    for (p=0; p < type_n; p++) {
      Usum += pattern_db[p].U; 
    }
  } 

  for (i=0; i<POSITIONS; i++)
    if (l_pos[i]==M_PART)
      l_pos[i] = L_m_part;

  T = T_MAX;

  for (i_n = 0; i_n < N_list_lng; i_n++) {
    wbw_access[i_n] = 0.0; wgh_access[i_n] = 0;
    Saccess[i_n] = 0;
    b_eff_io_ACCESS_WRT_REWRT[i_n] = 0.0;
    En[i_n] = 0;
  }

  Tn = T_MAX / N_list_lng;
  Tu = Tn / Usum;

  /* this is for the three different access types */
  for (a=0; a<ACCESSES; a++) {
    if (w_rank == 0) {
      fprintf(fp,"\n-----+---+------+----+--------+----------+----------+----------------+-------+-------------+-------+-------+------+-----+-----+-----+-----+----------------\n");
      fprintf(fp,"num. acc-| pat- |pat-|scheduled     chunk|     chunk|filename        | repeat|  transferred|  meas-|=sum of|time of meas.calls|last |last | measured       \n");
      fprintf(fp,"of   |ess| tern |tern|  time  |      size|      size|                | factor|   MB of this|  ured |   I/O |barr- |bcast|file-| I/O |barr.| bandwidth      \n");
      fprintf(fp,"PEs  |   | type |    | [sec]  |   on disk| in memory|                |       |      pattern|  time |       | ier  |     | sync|call |+bcst| of this pattern\n");
      fprintf(fp,"-----+---+------+----+--------+----------+----------+----------------+-------+-------------+-------+-------+------+-----+-----+-----+-----+----------------\n");
    }

    /* this is for the different numbers of processes */
    for (i_n = 0; i_n < N_list_lng; i_n++) {
      n = N_list[i_n];

      MPI_Barrier(MPI_COMM_WORLD);
      color = (w_rank < n ? 1 : 2);  key = 0;
      MPI_Comm_split(MPI_COMM_WORLD,color,key,&n_comm);
      MPI_Comm_rank(n_comm, &n_rank);

      if (color==1) {
        if (w_rank != n_rank) printf("[%d] ERROR - ranks %d!=%d\n", w_rank, w_rank, n_rank);
        MPI_Barrier(n_comm);
        t_partition[i_n][a] = MPI_Wtime();
        Spartition[i_n] = 0;
        bw_partition = 0;
        wbw_partition = 0; wgh_partition = 0;
        b_eff_io_ACCESSMETHOD[i_n][a] = 0.0;

        sum_Tp = 0; sum_t_pat = 0;
        /* this is for the different pattern types */
        type_n = ((!REWRITE && (a==1)) ? TYPE_0_PTRNS : PATTERNS);
        for (p=0; p < type_n; p++) {
          Ep = 0;
          type = pattern_db[p].type;
          part = pattern_db[p].part;
          pos  = pattern_db[p].pos;
          if (type==3 && part==0) {
            /* Initializing the repetition counts and Lseg for type=3,4 */
            int p0, p1, p2, p3;
            L3 = 0;
            rep[TYPE_3_START][i_n] = 1;
            rep[TYPE_4_START][i_n] = 1;
            L3 += pattern_db[TYPE_3_START].L;
            for (p3=1; p3<TYPE_3_PTRNS-1; p3++) {
              p0 = (p3 <= 1 ? p3 : p3+1); p1 = p3; p2 = p3;
              rep[TYPE_3_START+p3][i_n] = min(
                  rep[TYPE_0_START+p0][i_n] * (pattern_db[TYPE_0_START+p0].L / pattern_db[TYPE_0_START+p0].l ),
                  min( 10*rep[TYPE_1_START+p1][i_n], rep[TYPE_2_START+p2][i_n] ));
              L3 += rep[TYPE_3_START+p3][i_n] * ((MPI_Offset)pattern_db[TYPE_3_START+p3].L);
              rep[TYPE_4_START+p3][i_n] = rep[TYPE_3_START+p3][i_n];
            }
            Lseg = ( (L3-1) / (1 MB / l_etype) + 1) * (1 MB / l_etype);
            pattern_db[TYPE_3_START+TYPE_3_PTRNS-1].l = (int)(Lseg-L3);
            pattern_db[TYPE_3_START+TYPE_3_PTRNS-1].L = (int)(Lseg-L3);
            rep[TYPE_3_START+TYPE_3_PTRNS-1][i_n] = 1;
            pattern_db[TYPE_4_START+TYPE_4_PTRNS-1].l = (int)(Lseg-L3);
            pattern_db[TYPE_4_START+TYPE_4_PTRNS-1].L = (int)(Lseg-L3);
            rep[TYPE_4_START+TYPE_4_PTRNS-1][i_n] = 1;
          }
          l  = pattern_db[p].l;
          L  = pattern_db[p].L;
          U  = pattern_db[p].U;
          Tp = Tu * U;
          if (ADAPT && a==0 && type<=2 && p>=1 && sum_t_pat>sum_Tp && U!=0 && sum_Tp>0)
            /* reduction if t_sync results in sum_t_pat > sum_Tp */
            Tp = Tp / ( 0.5 * (1 + sum_t_pat/sum_Tp) );
          if (a>0 || type>=3)
            r_max = rep[p][i_n];
          else
            r_max = 0;
          Spat = 0;
          if (part == 0) {
            Stype = 0;
            wbw_type = 0; wgh_type = 0;
            if (type != 2)
              sprintf(filename,"b_eff_io_data_i%02d_%03d_%1d",i_n,n,type);
            else
              sprintf(filename,"b_eff_io_data_i%02d_%03d_%1d.%03d",i_n,n,type,n_rank);
            sprintf(full_filename,"%s/%s",PATH,filename);
            if (a==0) {
              /* remove existing old file */
              ierr=MPI_File_open((type != 2 ? n_comm : MPI_COMM_SELF), full_filename,
                      MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE,
                      MPI_INFO_NULL, &fh);
              ierr2=MPI_File_close(&fh);
              if ((ierr != MPI_SUCCESS) || (ierr2 != MPI_SUCCESS)) {
                  En[i_n]++;
                    printf("[%d] ERROR in deleting before opening: n=%d, p=%d, filename=%s, ierr_open=%d, ierr_close=%d\n",
                        w_rank,n,p,full_filename,ierr,ierr2);
                    if(w_rank==0) fprintf(fp,"[%d] ERROR in deleting before opening: n=%d, p=%d, filename=%s, ierr_open=%d, ierr_close=%d\n",
                        w_rank,n,p,full_filename,ierr,ierr2);
              }
            }
 
#ifndef WITHOUT_INFOFILE 
            if ( (w_rank == 0) && (info_filename) && (info_open[i_n][a][type] != MPI_INFO_NULL) ) {
              int i, r, l, valuelen;  
              char info_key[MPI_MAX_INFO_KEY+1], info_value[MAX_INFOLINE_LNG]; 
              MPI_Info_get_nkeys(info_open[i_n][a][type], &i);
              if (i>0) { 
                fprintf(fp,"file_info on File_open : %d %s %s on_open", 
                    n, access_string[a], pattype_string[type]);
                for (r=0; r<i; r++) {
                  MPI_Info_get_nthkey(info_open[i_n][a][type], r, info_key);
                  MPI_Info_get_valuelen(info_open[i_n][a][type], info_key, &valuelen, &l);
                  if (l) {
                    info_value[0]='\0'; 
                    MPI_Info_get(info_open[i_n][a][type], info_key, valuelen, info_value, &l);
                    fprintf(fp," :: %s %s", info_key, info_value);
                  }
                }
                fprintf(fp,"\n"); 
              }
            }
#endif 
 
            MPI_Barrier(n_comm);
            t_type = 0;
            t_open = MPI_Wtime();
            ierr=MPI_File_open((type != 2 ? n_comm : MPI_COMM_SELF), full_filename,
                     (a==0 ? MPI_MODE_CREATE | MPI_MODE_RDWR
                      :(a==1 ? MPI_MODE_RDWR :/*=2*/ MPI_MODE_RDONLY
                        | (DELETE ? MPI_MODE_DELETE_ON_CLOSE : 0)))
                     | (UNIQUE ? MPI_MODE_UNIQUE_OPEN : 0),
                     info_open[i_n][a][type] /* default: MPI_INFO_NULL */, &fh);
            if (ierr != MPI_SUCCESS) {
                  En[i_n]++;
                    printf("[%d] ERROR in File_open: n=%d, p=%d, filename=%s, ierr=%d\n",
                        w_rank,n,p,full_filename,ierr);
                    if(w_rank==0) fprintf(fp,"[%d] ERROR in File_open: n=%d, p=%d, filename=%s, ierr=%d\n",
                        w_rank,n,p,full_filename,ierr);
            }
            MPI_Barrier(n_comm);
            t_open = MPI_Wtime() - t_open;
            t_type += t_open;
          } /*endif (part == 0)*/

          MPI_Barrier(n_comm);
          t_pat = -MPI_Wtime();
          t_barrier = 0;  t_bcast = 0;  t_sync = 0;

          if (type == 0 || ((type == 1) && (!SHARED))) {
            gsizes[0]   = n;
            gsizes[1]   = l;
            distribs[0] = MPI_DISTRIBUTE_BLOCK;
            distribs[1] = MPI_DISTRIBUTE_NONE;
            dargs[0]    = MPI_DISTRIBUTE_DFLT_DARG;
            dargs[1]    = MPI_DISTRIBUTE_DFLT_DARG;
            psizes[0]   = n;
            psizes[1]   = 1;
            MPI_Type_create_darray(n, n_rank, 2, gsizes, distribs,
                dargs, psizes, MPI_ORDER_C,etype,
                &filetype);
            MPI_Type_commit(&filetype);
            if (part == 0) disp = 0;

            ierr=MPI_File_set_view(fh, disp, etype, filetype, "native",
                     info_view[i_n][a][p] /* default: MPI_INFO_NULL */);
            if (ierr != MPI_SUCCESS) {
                  En[i_n]++;
                    printf("[%d] ERROR in File_set_view: n=%d, p=%d, ierr=%d\n",
                        w_rank,n,p,ierr);
                    if(w_rank==0) fprintf(fp,"[%d] ERROR in File_set_view: n=%d, p=%d, ierr=%d\n",
                        w_rank,n,p,ierr);
            }
            MPI_Type_free(&filetype);
          } else if (part == 0) {
            if (type<3) disp = 0;
            else        disp = ((MPI_Offset)n_rank) * Lseg
              * ((MPI_Offset)l_etype);
            ierr=MPI_File_set_view(fh, disp, etype, etype, "native",
                     info_view[i_n][a][p] /* default: MPI_INFO_NULL */);
            if (ierr != MPI_SUCCESS) {
                  En[i_n]++;
                    printf("[%d] ERROR in File_set_view: n=%d, p=%d, ierr=%d\n",
                        w_rank,n,p,ierr);
                    if(w_rank==0) fprintf(fp,"[%d] ERROR in File_set_view: n=%d, p=%d, ierr=%d\n",
                        w_rank,n,p,ierr);
            }
          }else{
#           ifndef WITHOUT_INFOFILE 
              if (info_view[i_n][a][p] != MPI_INFO_NULL) {
                ierr=MPI_File_set_info(fh, info_view[i_n][a][p]); 
                if (ierr != MPI_SUCCESS) {
                  En[i_n]++;
                    printf("[%d] ERROR in File_set_info: n=%d, p=%d, ierr=%d\n",
                        w_rank,n,p,ierr);
                    if(w_rank==0) fprintf(fp,"[%d] ERROR in File_set_info: n=%d, p=%d, ierr=%d\n",
                        w_rank,n,p,ierr);
                }
              } 
              info_v_st[i_n][a][p] = 1;
#           endif 
          }
 
#ifndef WITHOUT_INFOFILE 
          if ( (w_rank == 0) && (info_filename) ) { 
            int i, r, l, valuelen;  
            char info_key[MPI_MAX_INFO_KEY+1], info_value[MAX_INFOLINE_LNG]; 
            MPI_Info info_used; 
            if (info_view[i_n][a][p] != MPI_INFO_NULL) {
              MPI_Info_get_nkeys(info_view[i_n][a][p], &i);
              if (i>0) { 
                fprintf(fp,"file_info File_%s p%02d: %d %s %s", 
                    (info_v_st[i_n][a][p] ? "info" : "view"),
                    p, n, access_string[a], pattype_string[type]);
                if (pattern_db[p].l == L_m_part/l_etype) fprintf(fp,"    Lmax");
                else fprintf(fp," %7.0f", 1.0*pattern_db[p].l*l_etype);
                for (r=0; r<i; r++) {
                  MPI_Info_get_nthkey(info_view[i_n][a][p], r, info_key);
                  MPI_Info_get_valuelen(info_view[i_n][a][p], info_key, &valuelen, &l);
                  if (l) {
                    info_value[0]='\0'; 
                    MPI_Info_get(info_view[i_n][a][p], info_key, valuelen, info_value, &l);
                    fprintf(fp," :: %s %s", info_key, info_value);
                  }
                }
                fprintf(fp,"\n");
              }
            }
            ierr=MPI_File_get_info(fh, &info_used); 
            if (ierr != MPI_SUCCESS) {
                  En[i_n]++;
                    printf("[%d] ERROR in File_get_info: n=%d, p=%d, ierr=%d\n",
                        w_rank,n,p,ierr);
                    if(w_rank==0) fprintf(fp,"[%d] ERROR in File_get_info: n=%d, p=%d, ierr=%d\n",
                        w_rank,n,p,ierr);
            }
            MPI_Info_get_nkeys(info_used, &i);
            if (i>0) { 
              fprintf(fp,"file_info USED ON   p%02d: %d %s %s", p, 
                      n, access_string[a], pattype_string[type]);
              if (pattern_db[p].l == L_m_part/l_etype) fprintf(fp,"    Lmax");
              else fprintf(fp," %7.0f", 1.0*pattern_db[p].l*l_etype);
              for (r=0; r<i; r++) {
                MPI_Info_get_nthkey(info_used, r, info_key);
                MPI_Info_get_valuelen(info_used, info_key, &valuelen, &l);
                if (l) {
                  info_value[0]='\0'; 
                  MPI_Info_get(info_used, info_key, valuelen, info_value, &l);
                  fprintf(fp," :: %s %s", info_key, info_value);
                }
              }
              fprintf(fp,"\n");
            }
            MPI_Info_free(&info_used); 
          } 
#endif 

          r = 0;
#ifdef WITHOUT_FILEACCESS 
          t_last_io = 0; t_last_barr_bcast=0; 
#else  /*WITHOUT_FILEACCESS*/
          do {
            t_last_io = -MPI_Wtime();
            if (a<2)
              for (i=0; i<L; i+=l) {
                wr_buf[i]     = 1e7*r + 1000*i + 10*p + a+1;
                wr_buf[i+l-1] = 1e7*r + 1000*i + 10*p + a+6;
              }
            switch (type) {
              case 0: /* strided, individual pointer, collective */
                if(a<2) ierr=MPI_File_write_all(fh,wr_buf,L,etype,&sts);
                else    ierr=MPI_File_read_all (fh,rd_buf,L,etype,&sts);
                break;
              case 1: /* strided, shared pointer, collective */
#ifdef WITHOUT_SHARED
                /* substituted individual pointer */
                if(a<2) ierr=MPI_File_write_all(fh,wr_buf,L,etype,&sts);
                else    ierr=MPI_File_read_all (fh,rd_buf,L,etype,&sts);
#else
                if (SHARED) { 
                 if(a<2) ierr=MPI_File_write_ordered(fh,wr_buf,L,etype,&sts);
                 else    ierr=MPI_File_read_ordered (fh,rd_buf,L,etype,&sts);
                }else{
                 if(a<2) ierr=MPI_File_write_all(fh,wr_buf,L,etype,&sts);
                 else    ierr=MPI_File_read_all (fh,rd_buf,L,etype,&sts);
                } 
#endif
                break;
              case 2: /* individual files, non-collective */
                if(a<2) ierr=MPI_File_write(fh,wr_buf,L,etype,&sts);
                else    ierr=MPI_File_read (fh,rd_buf,L,etype,&sts);
                break; 
              case 3: /* segmented, individual pointer, collective */ 
                if(a<2) ierr=MPI_File_write(fh,wr_buf,L,etype,&sts);
                else    ierr=MPI_File_read (fh,rd_buf,L,etype,&sts);
                break; 
              case 4: /* segmented, individual pointer, collective */ 
                if(a<2) ierr=MPI_File_write_all(fh,wr_buf,L,etype,&sts);
                else    ierr=MPI_File_read_all (fh,rd_buf,L,etype,&sts);
                break; 
            } /* switch (type) */
            if (ierr != MPI_SUCCESS) {
                  Ep++;  En[i_n]++;
                  if (Ep<=Ep_max)
                    printf("[%d] ERROR in File_read/write: n=%d, p=%d, ierr=%d\n",
                        w_rank,n,p,ierr);
                  if ((Ep<=Ep_max) && (w_rank==0)) 
                    fprintf(fp,"[%d] ERROR in File_read/write: n=%d, p=%d, ierr=%d\n",
                        w_rank,n,p,ierr);
            }
            if (a==2) {
              a_old = (REWRITE ? 1 : (type==0 ? 1 : 0 ));
              for (i=0; i<L; i+=l)
                if ( (rd_buf[i] != 1e7*r + 1000*i + 10*p + a_old+1) ||
                    (rd_buf[i+l-1] != 1e7*r + 1000*i + 10*p + a_old+6)) {
                  Ep++;  En[i_n]++;
                  /* TEST: the following conditions can be modified
                     without real impact to the total result: */
                  if (Ep<=Ep_max)
                    printf("[%d] ERROR in reading: p=%d, r=%d, i=%d, buf=(%1.0f,%1.0f) != (%1.0f,%1.0f)\n",
                        w_rank,p,r,i,rd_buf[i],rd_buf[i+l-1],
                        1e7*r + 1000*i + 10*p + a_old+1, 
                        1e7*r + 1000*i + 10*p + a_old+6);
                  if (Ep<=Ep_max && w_rank==0)
                    fprintf(fp,"[%d] ERROR in reading: p=%d, r=%d, i=%d, buf=(%1.0f,%1.0f) != (%1.0f,%1.0f)\n",
                        w_rank,p,r,i,rd_buf[i],rd_buf[i+l-1],
                        1e7*r + 1000*i + 10*p + a_old+1,
                        1e7*r + 1000*i + 10*p + a_old+6);
                }
            } /* if (a==2) */
            r++;
            t = MPI_Wtime();
            t_last_io += t;
            t_last_barr_bcast = -t;
            if (a==0 && type<=2) {
              t_barrier -= t;
              MPI_Barrier(n_comm);
              t = MPI_Wtime();
              t_barrier += t;
              t_bcast -= t;
              conti = (t + t_pat) * (r+1) / r <= Tp;
              MPI_Bcast(&conti, 1, MPI_CHAR, 0, n_comm);
              t = MPI_Wtime();
              t_bcast += t;
            }
            t_last_barr_bcast += t;
          } while ((a==0 && type<=2 ? conti : r<r_max));
#endif /*WITHOUT_FILEACCESS*/
 
          Spat = l_etype * ((MPI_Offset)L) * r;
          Stype += Spat;
          if(a<2) {
            t_barrier -= MPI_Wtime();
            MPI_Barrier(n_comm);
            t = MPI_Wtime();
            t_barrier += t;
            t_sync -= t;
            MPI_File_sync(fh);
            t_sync += MPI_Wtime();
          }
          t_barrier -= MPI_Wtime();
          MPI_Barrier(n_comm); 
          t = MPI_Wtime(); 
          t_barrier += t; 
          t_pat += t;
          t_type += t_pat;
          rep[p][i_n] = r;
          bw_pat = Spat * n / t_pat;
          wbw_type += bw_pat * U;       wgh_type += U;
          wbw_partition += bw_pat * U;  wgh_partition += U;
          if (type == 0 || ((type == 1) && (!SHARED)))
            disp = disp + ((MPI_Offset)n) * Spat;
          sum_Tp += Tu * U;  sum_t_pat += t_pat;
          if (part == 2) {
            t_close = MPI_Wtime();
            MPI_File_close(&fh);
            MPI_Barrier(n_comm);
            t = MPI_Wtime();
            t_close = t - t_close;
            t_type += t_close;
            bw_type = Stype * n / t_type;
          }
          MPI_Reduce(&Ep, &Ep_sum, 1, MPI_INT, MPI_SUM, 0, n_comm);
          if (w_rank == 0) {
            fprintf(fp,"n=%-3d a=%d type=%d p=%2d Tp=%5.2f l=%8.0f L=%8.0f %-16s r=%5d S=%8.3f MB t=%5.2f =%6.3f+%6.3f+%5.3f+%5.3f %5.3f %5.3f bw=%8.3f MB/s",
                n, a, type, p, Tp, 1.0*l*l_etype, 1.0*L*l_etype,
                filename+14, r, 1e-6*Spat*n, t_pat,
                t_pat - t_barrier - t_bcast - t_sync,
                t_barrier, t_bcast, t_sync,
                t_last_io, t_last_barr_bcast, 1e-6*bw_pat);
            if (Ep_sum > 0)
              fprintf(fp," %d ERRORS",Ep_sum);
            fprintf(fp,"\n"); fflush(fp);
            bw_x[i_n][a][type][pos] = ( Ep_sum > 0 ? -999.999e6 : bw_pat );
            if (part == 2) {
              fprintf(fp, " total pattern type:  S=%7.3f MB  t=%4.2f t_op=%4.2f t_cl=%4.2f wbw=%8.3f MB/s b_eff_io_%s_%s=%8.3f MB/s",
                  1e-6*Stype*n, t_type, t_open, t_close,
                  1e-6*wbw_type/wgh_type,
                  access_string[a], pattype_string[type],
                  1e-6*bw_type);
              if (type>=3)
                fprintf(fp, " Lseg=%1.0f MBytes",1.0*l_etype*Lseg/(1 MB));
              fprintf(fp, "\n"); fflush(fp);
              b_eff_io_ACCESSMETHOD_PATTERNTYPE[i_n][a][type] = bw_type;
              b_eff_io_ACCESSMETHOD[i_n][a] += (type==0 ? 2 : 1) * bw_type;

              if ((a==0) || ((a==1) && (type==0)))
                b_eff_io_ACCESS_WRT_REWRT[i_n] += bw_type;

              /* double weighted scatter-type 0 */
              Spartition[i_n] += Stype;
            } /* if (part == 2) */
          } /* if (w_rank == 0) */
        } /*endfor p*/

        MPI_Reduce(&En[i_n], &En_sum[i_n], 1, MPI_INT, MPI_SUM, 0, n_comm);
        MPI_Barrier(n_comm);
        t_partition[i_n][a] = MPI_Wtime() - t_partition[i_n][a];
        bw_partition = Spartition[i_n] * n / t_partition[i_n][a];

        Saccess[i_n] += Spartition[i_n]*n;
        wbw_access[i_n] += wbw_partition; wgh_access[i_n] += wgh_partition;

        /* double weighted scatter-type 0: + 1 */
        crit1[i_n] = (Tn >= T_CRIT);
        crit2[i_n] = (SHARED == 1);
        crit3[i_n] = (En_sum[i_n] == 0);
        crit_all[i_n] = crit1[i_n] && crit2[i_n] && crit3[i_n];

        if (w_rank == 0) {
          if (En_sum[i_n] > 0)
            fprintf(fp," ----- with %d ERRORS --!--!--!--!--", En_sum[i_n]);
          fprintf(fp, " total access method: S=%7.3f MB  t=%4.2f (Tn=%1.0fsec) bw=%8.3f MB/s, wbw=%8.3f MB/s, b_eff_io_%s=%8.3f MB/s\n",
              1e-6*Spartition[i_n]*n, t_partition[i_n][a], Tn,
              1e-6*bw_partition, 1e-6*wbw_partition/wgh_partition,
              access_string[a],
              1e-6*b_eff_io_ACCESSMETHOD[i_n][a] / (!REWRITE && a==1 ? 2 : (PAT_TYPES + 1) ));
        }
      } /* endif (color==1) */
    } /*endfor n*/
  } /*endfor a*/

  MPI_Barrier(MPI_COMM_WORLD);
  t_total = MPI_Wtime() - t_total;
  if (w_rank == 0) {
    b_eff_io = -1;
    for (i_n = 0; i_n < N_list_lng; i_n++) {
      n = N_list[i_n];

      b_eff_io_ACCESS_WRT_REWRT[i_n] /= (PAT_TYPES + 1);
      b_eff_io_ACCESSMETHOD[i_n][0] /= (PAT_TYPES + 1);
      b_eff_io_ACCESSMETHOD[i_n][1] /= ( REWRITE ? (PAT_TYPES + 1) : 2 );
      b_eff_io_ACCESSMETHOD[i_n][2] /= (PAT_TYPES + 1);

      b_eff_io_PARTITION[i_n] =
        sqrt( b_eff_io_ACCESS_WRT_REWRT[i_n] * b_eff_io_ACCESSMETHOD[i_n][2] );

      if (b_eff_io_PARTITION[i_n] > b_eff_io) {
        b_eff_io   = b_eff_io_PARTITION[i_n];
        b_eff_io_n = n;  b_eff_io_Tn = Tn;  b_eff_io_En = En_sum[i_n];
        crit_all_n = crit_all[i_n];
      }

      t_access = t_partition[i_n][0] + t_partition[i_n][1] + t_partition[i_n][2];
      bw_access = Saccess[i_n] / t_access;
      fprintf(fp, " total partition: n=%-3d S=%7.3f MB  t=%4.2f bw=%8.3f MB/s, wbw=%8.3f MB/s, b_eff_io=%8.3f MB/s\n",
          n, 1e-6*Saccess[i_n], t_access, 1e-6*bw_access,
          1e-6*wbw_access[i_n]/wgh_access[i_n],
          1e-6*b_eff_io_PARTITION[i_n]);
    }

    fprintf(fp, " total execution time is %7.2f sec\n",t_total);
    fprintf(fp, "\n b_eff_io := maximum over all bw_partition = %8.3f MB/s\n", 1e-6*b_eff_io);

    /* Summary protocol */
    for (i_n = 0; i_n < N_list_lng; i_n++) {
      n = N_list[i_n];
      fprintf(fp_short, "\nSummary of file I/O bandwidth accumulated on %3d processes with %4d MByte/PE\n", n, MEMORY_PER_PROCESSOR);
      fprintf(fp_short,   "-----------------------------------------------------------------------------\n\n");
      fprintf(fp_short, " number pos chunk-   access   type=0   type=1   type=2   type=3   type=4\n");
      fprintf(fp_short, " of PEs     size (l) methode scatter  shared  separate segmened seg-coll\n");
      fprintf(fp_short, "            [bytes]  methode  [MB/s]   [MB/s]   [MB/s]   [MB/s]   [MB/s]\n");
      fprintf(fp_short, " -----------------------------------------------------------------------\n\n");
      for (a=0; a<ACCESSES; a++) {
        for (pos=1; pos<POSITIONS; pos++) {
          fprintf(fp_short, "%4d PEs %1d %8d %-7s", n, pos, l_pos[pos], access_string[a]);
          for (type=0; type<PAT_TYPES; type++)
            if (a==1 && !REWRITE && type>=1)
              fprintf(fp_short, "        -");
            else
              fprintf(fp_short, " %8.3f", 1e-6*bw_x[i_n][a][type][pos]);
          fprintf(fp_short, "\n");
        }
        fprintf(fp_short, "%4d PEs      total-%-7s",n,access_string[a]);
        for (type=0; type<PAT_TYPES; type++)
          if (a==1 && !REWRITE && type>=1)
            fprintf(fp_short, "        -");
          else
            fprintf(fp_short, " %8.3f", 1e-6*b_eff_io_ACCESSMETHOD_PATTERNTYPE[i_n][a][type]);
        fprintf(fp_short, "\n\n");
      }

      fprintf(fp_short, "This table shows all results, except pattern 2 (scatter, l=1MBytes, L=2MBytes): \n");
      fprintf(fp_short, "  bw_pat2= %8.3f MB/s write, %8.3f MB/s rewrite, %8.3f MB/s read\n\n",
          1e-6*bw_x[i_n][0][0][8], 1e-6*bw_x[i_n][1][0][8], 1e-6*bw_x[i_n][2][0][8]);

      fprintf(fp_short, "(For gnuplot:)\n");
      fprintf(fp_short, "  set xtics ( '1k' 1, '+8' 2, '32k' 4, '+8' 5, '1M' 7, '+8' 8, '%1dM' 10)\n",L_m_part/1024/1024);
      fprintf(fp_short, "  set title '%s %s %s %s %s' -4\n",
          uname_info.sysname, uname_info.nodename, uname_info.release,
          uname_info.version, uname_info.machine);
      fprintf(fp_short, "  set label 1 'b_eff_io'  at 10,50000 right\n");
      fprintf(fp_short, "  set label 2 'rel. %s'  at 10,25000 right\n",Revision);
      fprintf(fp_short, "  set label 3 'T=%3.1fmin' at 10,12500 right\n",Tn/60.);
      fprintf(fp_short, "  set label 4 'n=%1d'     at 10,6250  right\n",n);
      if (!SHARED) {
        fprintf(fp_short,"  set label 5 'workaround for type 1:'  at 10,0.50 right\n");
        fprintf(fp_short,"  set label 6 'individual file pointer' at 10,0.25 right\n");
      }
      fprintf(fp_short, "\n");
      for (a=0; a<ACCESSES; a++)
      {
        fprintf(fp_short,"weighted average bandwidth for %-7s : %8.3f MB/s on %1d processes\n",
            access_string[a],
            1e-6*b_eff_io_ACCESSMETHOD[i_n][a], n);
      }
      fprintf(fp_short,"(type=0 is weighted double)\n");
      fprintf(fp_short, "\nTotal amount of data written/read with each access method: %8.3f MBytes\n",
          1.0*Spartition[i_n]*n/(1.0 MB));
      fprintf(fp_short, "  = %4.1f percent of the total memory (%1d MBytes)\n",
          100.*Spartition[i_n]*n/(MT * 1.0 MB), MT);
      fprintf(fp_short, "\nb_eff_io of these measurements = %8.3f MB/s on %1d processes with %3d MByte/PE and scheduled time=%3.1f min",
          1e-6*b_eff_io_PARTITION[i_n], n,
          MEMORY_PER_PROCESSOR, Tn/60.);
      if (En_sum[i_n] > 0) fprintf(fp_short,", with %d ERRORS --!--!--!--",En_sum[i_n]);
      fprintf(fp_short, "\n\n");

      fprintf(fp_short, "%s for comparison of different systems\n",
          (crit_all[i_n] ? "VALID" : "NOT VALID") );
      if ( ! crit_all[i_n] ) {
        fprintf(fp_short, "  criterion 1: scheduled time %3.1f min >= %2d min -- %s\n",
            Tn/60., T_CRIT/60, (crit1[i_n] ? "reached" : "NOT reached"));
        fprintf(fp_short, "  criterion 2: shared file pointers must be used for pattern type 1 -- %s\n",
            (crit2[i_n] ? "reached" : "NOT reached") );
        fprintf(fp_short, "  criterion 3: error count (%1d) == 0 -- %s\n",
            En_sum[i_n], (crit3[i_n] ? "reached" : "NOT reached") );
      }
    }

    fprintf(fp_short, "\nMaximum over all number of PEs\n");
    fprintf(fp_short,   "------------------------------\n\n");

    for (i=0; i<3; i++) {
      fpx = fp_all[i];
      fprintf(fpx, "\n b_eff_io = %8.3f MB/s on %1d processes"
          " with %3d MByte/PE, scheduled time=%3.1f Min, on %s %s %s %s %s",
          1e-6*b_eff_io, b_eff_io_n, MEMORY_PER_PROCESSOR, b_eff_io_Tn/60,
          uname_info.sysname, uname_info.nodename, uname_info.release,
          uname_info.version, uname_info.machine);
      if (!crit_all_n) fprintf(fpx,", NOT VALID (see above)");
      if (b_eff_io_En > 0) fprintf(fpx,", with %d ERRORS --!--!--!--",b_eff_io_En);
      fprintf(fpx,"\n");
    }
    fclose(fp);
    fclose(fp_short);
  }
 
  MPI_Finalize();
  return(0);
}
