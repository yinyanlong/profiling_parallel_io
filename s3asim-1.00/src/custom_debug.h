/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#ifndef _CUSTOM_DEBUG_H
#define _CUSTOM_DEBUG_H

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#define NO_DEBUG                (uint64_t)0
#define MASTER                  ((uint64_t)1 << 0)
#define WORKER                  ((uint64_t)1 << 1)
#define MASTER_MSG              ((uint64_t)1 << 2)
#define WORKER_MSG              ((uint64_t)1 << 3)
#define MASTER_ERR              ((uint64_t)1 << 4)
#define WORKER_ERR              ((uint64_t)1 << 5)
#define MASTER_COMPUTE          ((uint64_t)1 << 6)
#define WORKER_COMPUTE          ((uint64_t)1 << 7)
#define MASTER_IO               ((uint64_t)1 << 8)
#define WORKER_IO               ((uint64_t)1 << 9)
#define MASTER_ISEND            ((uint64_t)1 << 10)
#define WORKER_ISEND            ((uint64_t)1 << 11)
#define MASTER_IRECV            ((uint64_t)1 << 12)
#define WORKER_IRECV            ((uint64_t)1 << 13)
#define MASTER_GATHER           ((uint64_t)1 << 14)
#define WORKER_GATHER           ((uint64_t)1 << 15)
#define MASTER_RAND             ((uint64_t)1 << 16)
#define WORKER_RAND             ((uint64_t)1 << 17)

#define MSG_ALL                 (uint64_t) (MASTER_MSG + WORKER_MSG)
#define MASTER_ALL              (uint64_t) (MASTER + MASTER_MSG + MASTER_ERR \
                                            + MASTER_COMPUTE + MASTER_IO \
                                            + MASTER_ISEND + MASTER_IRECV \
                                            + MASTER_GATHER + MASTER_RAND)
#define WORKER_ALL              (uint64_t) (WORKER + WORKER_MSG + WORKER_ERR \
                                            + WORKER_COMPUTE + WORKER_IO \
                                            + WORKER_ISEND + WORKER_IRECV \
                                            + WORKER_GATHER + WORKER_RAND)
#define DEBUG_ALL               (uint64_t) (MASTER_ALL + WORKER_ALL)

#define DEBUG_BUF_SIZE 1024

int custom_debug(uint64_t mask,
		 const char *format,
		 ...);
#endif
