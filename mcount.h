/*
 * data structures for handling mcount records
 *
 * Copyright (C) 2014-2015, LG Electronics, Namhyung Kim <namhyung.kim@lge.com>
 *
 * Released under the GPL v2.
 */

#ifndef FTRACE_MCOUNT_H
#define FTRACE_MCOUNT_H

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <libelf.h>
#include <limits.h>

#include "utils/rbtree.h"
#include "utils/symbol.h"

#define likely(x)  __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)

#define MCOUNT_RSTACK_MAX  1024
#define MCOUNT_NOTRACE_IDX 0x10000
#define MCOUNT_INVALID_DYNIDX 0xffff
#define MCOUNT_DEFAULT_DEPTH  (INT_MAX - 1)

struct mcount_ret_stack {
	unsigned long *parent_loc;
	unsigned long parent_ip;
	unsigned long child_ip;
	/* time in nsec (CLOCK_MONOTONIC) */
	uint64_t start_time;
	uint64_t end_time;
	int tid;
	unsigned short depth;
	unsigned short dyn_idx;
};

void __monstartup(unsigned long low, unsigned long high);
void _mcleanup(void);
void mcount_restore(void);
void mcount_reset(void);

enum ftrace_ret_stack_type {
	FTRACE_ENTRY,
	FTRACE_EXIT,
	FTRACE_LOST,
};

#define FTRACE_UNUSED  0xa

/* reduced version of mcount_ret_stack */
struct ftrace_ret_stack {
	uint64_t time;
	uint64_t type:   2;
	uint64_t unused: 4;
	uint64_t depth:  10;
	uint64_t addr:   48;
};

#define SHMEM_BUFFER_SIZE  (128 * 1024)

enum shmem_buffer_flags {
	SHMEM_FL_NEW		= (1U << 0),
	SHMEM_FL_WRITTEN	= (1U << 1),
};

struct mcount_shmem_buffer {
	unsigned size;
	unsigned flag;
	char data[];
};

#define FTRACE_MSG_MAGIC 0xface

#define FTRACE_MSG_REC_START  1U
#define FTRACE_MSG_REC_END    2U
#define FTRACE_MSG_TID        3U
#define FTRACE_MSG_FORK_START 4U
#define FTRACE_MSG_FORK_END   5U
#define FTRACE_MSG_SESSION    6U
#define FTRACE_MSG_LOST       7U

/* msg format for communicating by pipe */
struct ftrace_msg {
	unsigned short magic; /* FTRACE_MSG_MAGIC */
	unsigned short type;  /* FTRACE_MSG_REC_* */
	unsigned int len;
	unsigned char data[];
};

struct ftrace_msg_task {
	uint64_t time;
	int32_t  pid;
	int32_t  tid;
};

struct ftrace_msg_sess {
	struct ftrace_msg_task task;
	char sid[16];
	int  unused;
	int  namelen;
	char exename[];
};

#define FTRACE_MAGIC_LEN  8
#define FTRACE_MAGIC_STR  "Ftrace!"
#define FTRACE_FILE_VERSION  3
#define FTRACE_FILE_VERSION_MIN  2
#define FTRACE_FILE_NAME  "ftrace.data"
#define FTRACE_DIR_NAME   "ftrace.dir"

struct ftrace_file_header {
	char magic[FTRACE_MAGIC_LEN];
	uint32_t version;
	uint16_t header_size;
	uint8_t  endian;
	uint8_t  class;
	uint64_t feat_mask;
	uint64_t info_mask;
	uint64_t unused;
};

enum ftrace_feat_bits {
	/* bit index */
	PLTHOOK_BIT,
	TASK_SESSION_BIT,
	KERNEL_BIT,

	/* bit mask */
	PLTHOOK			= (1U << PLTHOOK_BIT),
	TASK_SESSION		= (1U << TASK_SESSION_BIT),
	KERNEL			= (1U << KERNEL_BIT),
};

enum ftrace_info_bits {
	EXE_NAME,
	EXE_BUILD_ID,
	EXIT_STATUS,
	CMDLINE,
	CPUINFO,
	MEMINFO,
	OSINFO,
	TASKINFO,
};

struct ftrace_info {
	char *exename;
	unsigned char build_id[20];
	int exit_status;
	char *cmdline;
	int nr_cpus_online;
	int nr_cpus_possible;
	char *cpudesc;
	char *meminfo;
	char *kernel;
	char *hostname;
	char *distro;
	int nr_tid;
	int *tids;
};

struct ftrace_kernel;

struct ftrace_file_handle {
	FILE *fp;
	const char *dirname;
	struct ftrace_file_header hdr;
	struct ftrace_info info;
	struct ftrace_kernel *kern;
	int depth;
};

struct kbuffer;
struct pevent;

struct ftrace_kernel {
	int pid;
	int nr_cpus;
	int depth;
	int *traces;
	int *fds;
	int64_t *offsets;
	int64_t *sizes;
	void **mmaps;
	struct kbuffer **kbufs;
	struct pevent *pevent;
	struct mcount_ret_stack *rstacks;
	bool *rstack_valid;
	bool *rstack_done;
	char *output_dir;
	char *filters;
	char *notrace;
};

int start_kernel_tracing(struct ftrace_kernel *kernel);
int record_kernel_tracing(struct ftrace_kernel *kernel);
int stop_kernel_tracing(struct ftrace_kernel *kernel);
int finish_kernel_tracing(struct ftrace_kernel *kernel);

int setup_kernel_data(struct ftrace_kernel *kernel);
int read_kernel_stack(struct ftrace_kernel *kernel, struct mcount_ret_stack *rstack);
int finish_kernel_data(struct ftrace_kernel *kernel);

int read_tid_list(int *tids, bool skip_unknown);
void free_tid_list(void);

void fill_ftrace_info(uint64_t *info_mask, int fd, char *exename, Elf *elf,
		      int status);
int read_ftrace_info(uint64_t info_mask, struct ftrace_file_handle *handle);
void clear_ftrace_info(struct ftrace_info *info);

int arch_fill_cpuinfo_model(int fd);

#endif /* FTRACE_MCOUNT_H */
