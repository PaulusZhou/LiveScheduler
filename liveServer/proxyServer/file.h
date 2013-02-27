#ifndef _FILE_H_
#define _FILE_H_

#define MAX_G_STRING_SIZE 64
#include <inttypes.h>
#include <sys/time.h>

#ifndef BUFFSIZE
#define BUFFSIZE 4096
#endif
#define SLURP_FAILURE -1

#define SYNAPSE_SUCCESS 0
#define SYNAPSE_FAILURE -1
typedef struct {
  struct timeval last_read;
  float thresh;
  char *name;
  char *buffer;
  size_t buffersize;
} timely_file; 
enum g_type_t {
   g_string,  /* huh uh.. he said g string */
   g_int8,
   g_uint8,
   g_int16,
   g_uint16,
   g_int32,
   g_uint32,
   g_float,
   g_double,
   g_timestamp    /* a uint32 */
};
typedef enum g_type_t g_type_t;
typedef union {
    int8_t   int8;
   uint8_t  uint8;
   int16_t  int16;
  uint16_t uint16;
   int32_t  int32;
  uint32_t uint32;
   float   f; 
   double  d;
   char str[MAX_G_STRING_SIZE];
} g_val_t; 

int slurpfile (char * filename, char **buffer, int buflen);
float timediff (const struct timeval *thistime,
                const struct timeval *lasttime);
char *update_file (timely_file *tf);
char *skip_whitespace (const char *p);
char *skip_token (const char *p);
#endif
