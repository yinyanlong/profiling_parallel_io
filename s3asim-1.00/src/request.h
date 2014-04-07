/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _REQUEST_H
#define _REQUEST_H

/* Processing states */
#define NO_WORK                  -1
#define RET_QUERY_FRAG           -2
#define CUR_QUERY_SCHED_DONE     -3
#define ALL_QUERY_SCHED_DONE     -4

/* Control msgs */
#define WORK_REQ                 0 
#define MAX_REQ                  1

/* MPI Tags */
#define MASTER_CONTROL            1
#define WORKER_CONTROL            1
#define MASTER_SEND_WORK_INFO     2
#define WORKER_RECV_WORK_INFO     2
#define MASTER_RECV_FRAG_INFO     3
#define WORKER_SEND_FRAG_INFO     3
#define MASTER_RECV_RESULT_SIZE   4 
#define WORKER_SEND_RESULT_SIZE   4
#define MASTER_RECV_RESULT_INFO   5
#define WORKER_SEND_RESULT_INFO   5
#define MASTER_RECV_RESULT_DATA   6
#define WORKER_SEND_RESULT_DATA   6
#define MASTER_SEND_OFFSETS_COUNT 7
#define WORKER_RECV_OFFSETS_COUNT 7
#define MASTER_SEND_OFFSETS       8
#define WORKER_RECV_OFFSETS       8

#endif
