#include "util/memstream.h"

#include <pthread.h>
#include <stdlib.h>

struct aros_memstream_state {
   struct u_memstream *mem;
   char **bufp;
   size_t *sizep;
   struct aros_memstream_state *next;
};

static pthread_mutex_t g_memstream_lock = PTHREAD_MUTEX_INITIALIZER;
static struct aros_memstream_state *g_memstream_states;

static struct aros_memstream_state *
find_state_locked(const struct u_memstream *mem, struct aros_memstream_state **prev_out)
{
   struct aros_memstream_state *prev = NULL;
   struct aros_memstream_state *cur = g_memstream_states;

   while (cur) {
      if (cur->mem == mem) {
         if (prev_out)
            *prev_out = prev;
         return cur;
      }
      prev = cur;
      cur = cur->next;
   }

   if (prev_out)
      *prev_out = NULL;
   return NULL;
}

static void
update_buffer_locked(struct aros_memstream_state *st)
{
   FILE *f = st->mem->f;
   long current_pos = ftell(f);
   long size_long;
   size_t size;
   char *buf;

   if (current_pos < 0)
      current_pos = 0;

   if (fseek(f, 0, SEEK_END) != 0)
      return;

   size_long = ftell(f);
   if (size_long < 0)
      size_long = 0;
   size = (size_t)size_long;

   buf = *st->bufp;
   if (!buf || *st->sizep < size + 1) {
      char *new_buf = realloc(buf, size + 1);
      if (!new_buf) {
         fseek(f, current_pos, SEEK_SET);
         return;
      }
      buf = new_buf;
      *st->bufp = new_buf;
   }

   if (size > 0) {
      if (fseek(f, 0, SEEK_SET) != 0)
         return;
      (void)fread(buf, 1, size, f);
   }

   buf[size] = '\0';
   *st->sizep = size;

   (void)fseek(f, current_pos, SEEK_SET);
}

bool
u_memstream_open(struct u_memstream *mem, char **bufp, size_t *sizep)
{
   struct aros_memstream_state *state;
   FILE *f = tmpfile();

   if (!f)
      return false;

   state = calloc(1, sizeof(*state));
   if (!state) {
      fclose(f);
      return false;
   }

   *bufp = NULL;
   *sizep = 0;

   mem->f = f;
   state->mem = mem;
   state->bufp = bufp;
   state->sizep = sizep;

   pthread_mutex_lock(&g_memstream_lock);
   state->next = g_memstream_states;
   g_memstream_states = state;
   pthread_mutex_unlock(&g_memstream_lock);

   return true;
}

void
u_memstream_close(struct u_memstream *mem)
{
   struct aros_memstream_state *state;
   struct aros_memstream_state *prev;
   FILE *f = mem->f;

   if (!f)
      return;

   pthread_mutex_lock(&g_memstream_lock);
   state = find_state_locked(mem, &prev);
   if (state) {
      (void)fflush(f);
      update_buffer_locked(state);
      if (prev)
         prev->next = state->next;
      else
         g_memstream_states = state->next;
   }
   pthread_mutex_unlock(&g_memstream_lock);

   if (state)
      free(state);

   fclose(f);
   mem->f = NULL;
}

int
u_memstream_flush(struct u_memstream *mem)
{
   int ret;
   struct aros_memstream_state *state;

   if (!mem->f)
      return 0;

   ret = fflush(mem->f);
   if (ret != 0)
      return ret;

   pthread_mutex_lock(&g_memstream_lock);
   state = find_state_locked(mem, NULL);
   if (state)
      update_buffer_locked(state);
   pthread_mutex_unlock(&g_memstream_lock);

   return 0;
}
