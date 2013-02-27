#ifndef _METRICS_H_
#define _METRICS_H_

#define MAX_G_STRING_SIZE 64
#include <inttypes.h>
#include <sys/time.h>
#include "file.h"

g_val_t metric_init(void);
g_val_t cpu_num_func(void);
g_val_t cpu_speed_func(void);
g_val_t mem_total_func(void);
g_val_t swap_total_func(void);
g_val_t boottime_func(void);
g_val_t sys_clock_func(void);
g_val_t machine_type_func(void);
g_val_t os_name_func(void);
g_val_t os_release_func(void);
g_val_t mtu_func(void);
g_val_t cpu_user_func(void);
g_val_t cpu_nice_func(void);
g_val_t cpu_system_func(void);
g_val_t cpu_idle_func(void);
g_val_t cpu_wio_func(void);
g_val_t cpu_aidle_func(void);
g_val_t cpu_intr_func(void);
g_val_t cpu_sintr_func(void);
g_val_t bytes_in_func(void);
g_val_t bytes_out_func(void);
g_val_t pkts_in_func(void);
g_val_t pkts_out_func(void);
g_val_t disk_total_func(void);
g_val_t disk_free_func(void);
g_val_t part_max_used_func(void);
g_val_t load_one_func(void);
g_val_t load_five_func(void);
g_val_t load_fifteen_func(void);
g_val_t proc_run_func(void);
g_val_t proc_total_func(void);
g_val_t mem_free_func(void);
g_val_t mem_shared_func(void);
g_val_t mem_buffers_func(void);
g_val_t mem_cached_func(void);
g_val_t swap_free_func(void);
g_val_t gexec_func(void);
g_val_t heartbeat_func(void);
g_val_t location_func(void); 
#endif
