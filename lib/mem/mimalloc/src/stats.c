/* ----------------------------------------------------------------------------
Copyright (c) 2018-2021, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#include "mimalloc.h"
#include "mimalloc/internal.h"
#include "mimalloc/atomic.h"
#include "mimalloc/prim.h"

#include <string.h> // memset

#if defined(_MSC_VER) && (_MSC_VER < 1920)
#pragma warning(disable:4204)  // non-constant aggregate initializer
#endif

/* -----------------------------------------------------------
  Statistics operations
----------------------------------------------------------- */

static void mi_stat_update_mt(mi_stat_count_t* stat, int64_t amount) {
  if (amount == 0) return;
  // add atomically
  int64_t current = mi_atomic_addi64_relaxed(&stat->current, amount);
  mi_atomic_maxi64_relaxed(&stat->peak, current + amount);
  if (amount > 0) {
    mi_atomic_addi64_relaxed(&stat->allocated, amount);
  }
  else {
    mi_atomic_addi64_relaxed(&stat->freed, -amount);
  }
}

static void mi_stat_update(mi_stat_count_t* stat, int64_t amount) {
  if (amount == 0) return;
  // add thread local
  stat->current += amount;
  if (stat->current > stat->peak) stat->peak = stat->current;
  if (amount > 0) {
    stat->allocated += amount;
  }
  else {
    stat->freed += -amount;
  }
}


// Adjust stats to compensate; for example before committing a range,
// first adjust downwards with parts that were already committed so
// we avoid double counting.
static void mi_stat_adjust_mt(mi_stat_count_t* stat, int64_t amount, bool on_alloc) {
  if (amount == 0) return;
  // adjust atomically
  mi_atomic_addi64_relaxed(&stat->current, amount);
  mi_atomic_addi64_relaxed((on_alloc ? &stat->allocated : &stat->freed), amount);
}

static void mi_stat_adjust(mi_stat_count_t* stat, int64_t amount, bool on_alloc) {
  if (amount == 0) return;
  stat->current += amount;
  if (on_alloc) {
    stat->allocated += amount;
  }
  else {
    stat->freed += amount;
  }
}

void __mi_stat_counter_increase_mt(mi_stat_counter_t* stat, size_t amount) {
  mi_atomic_addi64_relaxed(&stat->count, 1);
  mi_atomic_addi64_relaxed(&stat->total, (int64_t)amount);
}

void __mi_stat_counter_increase(mi_stat_counter_t* stat, size_t amount) {
  stat->count++;
  stat->total += amount;
}

void __mi_stat_increase_mt(mi_stat_count_t* stat, size_t amount) {
  mi_stat_update_mt(stat, (int64_t)amount);
}
void __mi_stat_increase(mi_stat_count_t* stat, size_t amount) {
  mi_stat_update(stat, (int64_t)amount);
}

void __mi_stat_decrease_mt(mi_stat_count_t* stat, size_t amount) {
  mi_stat_update_mt(stat, -((int64_t)amount));
}
void __mi_stat_decrease(mi_stat_count_t* stat, size_t amount) {
  mi_stat_update(stat, -((int64_t)amount));
}

void __mi_stat_adjust_increase_mt(mi_stat_count_t* stat, size_t amount, bool on_alloc) {
  mi_stat_adjust_mt(stat, (int64_t)amount, on_alloc);
}
void __mi_stat_adjust_increase(mi_stat_count_t* stat, size_t amount, bool on_alloc) {
  mi_stat_adjust(stat, (int64_t)amount, on_alloc);
}

void __mi_stat_adjust_decrease_mt(mi_stat_count_t* stat, size_t amount, bool on_alloc) {
  mi_stat_adjust_mt(stat, -((int64_t)amount), on_alloc);
}
void __mi_stat_adjust_decrease(mi_stat_count_t* stat, size_t amount, bool on_alloc) {
  mi_stat_adjust(stat, -((int64_t)amount), on_alloc);
}


// must be thread safe as it is called from stats_merge
static void mi_stat_add(mi_stat_count_t* stat, const mi_stat_count_t* src, int64_t unit) {
  if (stat==src) return;
  if (src->allocated==0 && src->freed==0) return;
  mi_atomic_addi64_relaxed( &stat->allocated, src->allocated * unit);
  mi_atomic_addi64_relaxed( &stat->current, src->current * unit);
  mi_atomic_addi64_relaxed( &stat->freed, src->freed * unit);
  // peak scores do not work across threads..
  mi_atomic_addi64_relaxed( &stat->peak, src->peak * unit);
}

static void mi_stat_counter_add(mi_stat_counter_t* stat, const mi_stat_counter_t* src, int64_t unit) {
  if (stat==src) return;
  mi_atomic_addi64_relaxed( &stat->total, src->total * unit);
  mi_atomic_addi64_relaxed( &stat->count, src->count * unit);
}

// must be thread safe as it is called from stats_merge
static void mi_stats_add(mi_stats_t* stats, const mi_stats_t* src) {
  if (stats==src) return;
  mi_stat_add(&stats->pages, &src->pages,1);
  mi_stat_add(&stats->reserved, &src->reserved, 1);
  mi_stat_add(&stats->committed, &src->committed, 1);
  mi_stat_add(&stats->reset, &src->reset, 1);
  mi_stat_add(&stats->purged, &src->purged, 1);
  mi_stat_add(&stats->page_committed, &src->page_committed, 1);

  mi_stat_add(&stats->pages_abandoned, &src->pages_abandoned, 1);
  mi_stat_add(&stats->threads, &src->threads, 1);

  mi_stat_add(&stats->malloc, &src->malloc, 1);
  mi_stat_add(&stats->normal, &src->normal, 1);
  mi_stat_add(&stats->huge, &src->huge, 1);
  mi_stat_add(&stats->giant, &src->giant, 1);

  mi_stat_counter_add(&stats->pages_extended, &src->pages_extended, 1);
  mi_stat_counter_add(&stats->mmap_calls, &src->mmap_calls, 1);
  mi_stat_counter_add(&stats->commit_calls, &src->commit_calls, 1);
  mi_stat_counter_add(&stats->reset_calls, &src->reset_calls, 1);
  mi_stat_counter_add(&stats->purge_calls, &src->purge_calls, 1);

  mi_stat_counter_add(&stats->page_no_retire, &src->page_no_retire, 1);
  mi_stat_counter_add(&stats->searches, &src->searches, 1);
  mi_stat_counter_add(&stats->normal_count, &src->normal_count, 1);
  mi_stat_counter_add(&stats->huge_count, &src->huge_count, 1);
  mi_stat_counter_add(&stats->guarded_alloc_count, &src->guarded_alloc_count, 1);
#if MI_STAT>1
  for (size_t i = 0; i <= MI_BIN_HUGE; i++) {
    if (src->normal_bins[i].allocated > 0 || src->normal_bins[i].freed > 0) {
      mi_stat_add(&stats->normal_bins[i], &src->normal_bins[i], 1);
    }
  }
#endif
}

/* -----------------------------------------------------------
  Display statistics
----------------------------------------------------------- */

// unit > 0 : size in binary bytes
// unit == 0: count as decimal
// unit < 0 : count in binary
static void mi_printf_amount(int64_t n, int64_t unit, mi_output_fun* out, void* arg, const char* fmt) {
  char buf[32]; _mi_memzero_var(buf);
  int  len = 32;
  const char* suffix = (unit <= 0 ? " " : "B");
  const int64_t base = (unit == 0 ? 1000 : 1024);
  if (unit>0) n *= unit;

  const int64_t pos = (n < 0 ? -n : n);
  if (pos < base) {
    if (n!=1 || suffix[0] != 'B') {  // skip printing 1 B for the unit column
      _mi_snprintf(buf, len, "%lld   %-3s", (long long)n, (n==0 ? "" : suffix));
    }
  }
  else {
    int64_t divider = base;
    const char* magnitude = "K";
    if (pos >= divider*base) { divider *= base; magnitude = "M"; }
    if (pos >= divider*base) { divider *= base; magnitude = "G"; }
    const int64_t tens = (n / (divider/10));
    const long whole = (long)(tens/10);
    const long frac1 = (long)(tens%10);
    char unitdesc[8];
    _mi_snprintf(unitdesc, 8, "%s%s%s", magnitude, (base==1024 ? "i" : ""), suffix);
    _mi_snprintf(buf, len, "%ld.%ld %-3s", whole, (frac1 < 0 ? -frac1 : frac1), unitdesc);
  }
  _mi_fprintf(out, arg, (fmt==NULL ? "%12s" : fmt), buf);
}


static void mi_print_amount(int64_t n, int64_t unit, mi_output_fun* out, void* arg) {
  mi_printf_amount(n,unit,out,arg,NULL);
}

static void mi_print_count(int64_t n, int64_t unit, mi_output_fun* out, void* arg) {
  if (unit==1) _mi_fprintf(out, arg, "%12s"," ");
          else mi_print_amount(n,0,out,arg);
}

static void mi_stat_print_ex(const mi_stat_count_t* stat, const char* msg, int64_t unit, mi_output_fun* out, void* arg, const char* notok ) {
  _mi_fprintf(out, arg,"%10s:", msg);
  if (unit != 0) {
    if (unit > 0) {
      mi_print_amount(stat->peak, unit, out, arg);
      mi_print_amount(stat->allocated, unit, out, arg);
      mi_print_amount(stat->freed, unit, out, arg);
      mi_print_amount(stat->current, unit, out, arg);
      mi_print_amount(unit, 1, out, arg);
      mi_print_count(stat->allocated, unit, out, arg);
    }
    else {
      mi_print_amount(stat->peak, -1, out, arg);
      mi_print_amount(stat->allocated, -1, out, arg);
      mi_print_amount(stat->freed, -1, out, arg);
      mi_print_amount(stat->current, -1, out, arg);
      if (unit == -1) {
        _mi_fprintf(out, arg, "%24s", "");
      }
      else {
        mi_print_amount(-unit, 1, out, arg);
        mi_print_count((stat->allocated / -unit), 0, out, arg);
      }
    }
    if (stat->allocated > stat->freed) {
      _mi_fprintf(out, arg, "  ");
      _mi_fprintf(out, arg, (notok == NULL ? "not all freed" : notok));
      _mi_fprintf(out, arg, "\n");
    }
    else {
      _mi_fprintf(out, arg, "  ok\n");
    }
  }
  else {
    mi_print_amount(stat->peak, 1, out, arg);
    mi_print_amount(stat->allocated, 1, out, arg);
    _mi_fprintf(out, arg, "%11s", " ");  // no freed
    mi_print_amount(stat->current, 1, out, arg);
    _mi_fprintf(out, arg, "\n");
  }
}

static void mi_stat_print(const mi_stat_count_t* stat, const char* msg, int64_t unit, mi_output_fun* out, void* arg) {
  mi_stat_print_ex(stat, msg, unit, out, arg, NULL);
}

static void mi_stat_peak_print(const mi_stat_count_t* stat, const char* msg, int64_t unit, mi_output_fun* out, void* arg) {
  _mi_fprintf(out, arg, "%10s:", msg);
  mi_print_amount(stat->peak, unit, out, arg);
  _mi_fprintf(out, arg, "\n");
}

static void mi_stat_counter_print(const mi_stat_counter_t* stat, const char* msg, mi_output_fun* out, void* arg ) {
  _mi_fprintf(out, arg, "%10s:", msg);
  mi_print_amount(stat->total, -1, out, arg);
  _mi_fprintf(out, arg, "\n");
}


static void mi_stat_counter_print_avg(const mi_stat_counter_t* stat, const char* msg, mi_output_fun* out, void* arg) {
  const int64_t avg_tens = (stat->count == 0 ? 0 : (stat->total*10 / stat->count));
  const long avg_whole = (long)(avg_tens/10);
  const long avg_frac1 = (long)(avg_tens%10);
  _mi_fprintf(out, arg, "%10s: %5ld.%ld avg\n", msg, avg_whole, avg_frac1);
}


static void mi_print_header(mi_output_fun* out, void* arg ) {
  _mi_fprintf(out, arg, "%10s: %11s %11s %11s %11s %11s %11s\n", "heap stats", "peak   ", "total   ", "freed   ", "current   ", "unit   ", "count   ");
}

#if MI_STAT>1
static void mi_stats_print_bins(const mi_stat_count_t* bins, size_t max, const char* fmt, mi_output_fun* out, void* arg) {
  bool found = false;
  char buf[64];
  for (size_t i = 0; i <= max; i++) {
    if (bins[i].allocated > 0) {
      found = true;
      int64_t unit = _mi_bin_size((uint8_t)i);
      _mi_snprintf(buf, 64, "%s %3lu", fmt, (long)i);
      mi_stat_print(&bins[i], buf, unit, out, arg);
    }
  }
  if (found) {
    _mi_fprintf(out, arg, "\n");
    mi_print_header(out, arg);
  }
}
#endif



//------------------------------------------------------------
// Use an output wrapper for line-buffered output
// (which is nice when using loggers etc.)
//------------------------------------------------------------
typedef struct buffered_s {
  mi_output_fun* out;   // original output function
  void*          arg;   // and state
  char*          buf;   // local buffer of at least size `count+1`
  size_t         used;  // currently used chars `used <= count`
  size_t         count; // total chars available for output
} buffered_t;

static void mi_buffered_flush(buffered_t* buf) {
  buf->buf[buf->used] = 0;
  _mi_fputs(buf->out, buf->arg, NULL, buf->buf);
  buf->used = 0;
}

static void mi_cdecl mi_buffered_out(const char* msg, void* arg) {
  buffered_t* buf = (buffered_t*)arg;
  if (msg==NULL || buf==NULL) return;
  for (const char* src = msg; *src != 0; src++) {
    char c = *src;
    if (buf->used >= buf->count) mi_buffered_flush(buf);
    mi_assert_internal(buf->used < buf->count);
    buf->buf[buf->used++] = c;
    if (c == '\n') mi_buffered_flush(buf);
  }
}

//------------------------------------------------------------
// Print statistics
//------------------------------------------------------------

static void _mi_stats_print(mi_stats_t* stats, mi_output_fun* out0, void* arg0) mi_attr_noexcept {
  // wrap the output function to be line buffered
  char buf[256]; _mi_memzero_var(buf);
  buffered_t buffer = { out0, arg0, NULL, 0, 255 };
  buffer.buf = buf;
  mi_output_fun* out = &mi_buffered_out;
  void* arg = &buffer;

  // and print using that
  mi_print_header(out,arg);
  #if MI_STAT>1
  mi_stats_print_bins(stats->normal_bins, MI_BIN_HUGE, "normal",out,arg);
  #endif
  #if MI_STAT
  mi_stat_print(&stats->normal, "normal", (stats->normal_count.count == 0 ? 1 : -(stats->normal.allocated / stats->normal_count.count)), out, arg);
  mi_stat_print(&stats->huge, "huge", (stats->huge_count.count == 0 ? 1 : -(stats->huge.allocated / stats->huge_count.count)), out, arg);
  mi_stat_count_t total = { 0,0,0,0 };
  mi_stat_add(&total, &stats->normal, 1);
  mi_stat_add(&total, &stats->huge, 1);
  mi_stat_print(&total, "total", 1, out, arg);
  #endif
  #if MI_STAT>1
  mi_stat_print(&stats->malloc, "malloc req", 1, out, arg);
  _mi_fprintf(out, arg, "\n");
  #endif
  mi_stat_print_ex(&stats->reserved, "reserved", 1, out, arg, "");
  mi_stat_print_ex(&stats->committed, "committed", 1, out, arg, "");
  mi_stat_peak_print(&stats->reset, "reset", 1, out, arg );
  mi_stat_peak_print(&stats->purged, "purged", 1, out, arg );
  //mi_stat_print(&stats->segments, "segments", -1, out, arg);
  //mi_stat_print(&stats->segments_abandoned, "-abandoned", -1, out, arg);
  //mi_stat_print(&stats->segments_cache, "-cached", -1, out, arg);
  mi_stat_print_ex(&stats->page_committed, "touched", 1, out, arg, "");
  mi_stat_print_ex(&stats->pages, "pages", -1, out, arg, "");
  mi_stat_print(&stats->pages_abandoned, "-abandoned", -1, out, arg);
  mi_stat_counter_print(&stats->pages_reclaim_on_alloc, "-reclaima", out, arg);
  mi_stat_counter_print(&stats->pages_reclaim_on_free,  "-reclaimf", out, arg);
  mi_stat_counter_print(&stats->pages_reabandon_full, "-reabandon", out, arg);
  mi_stat_counter_print(&stats->pages_unabandon_busy_wait, "-waits", out, arg);
  mi_stat_counter_print(&stats->pages_extended, "-extended", out, arg);
  mi_stat_counter_print(&stats->page_no_retire, "-noretire", out, arg);
  mi_stat_counter_print(&stats->arena_count, "arenas", out, arg);
  mi_stat_counter_print(&stats->arena_purges, "-purges", out, arg);
  mi_stat_counter_print(&stats->mmap_calls, "mmap calls", out, arg);
  mi_stat_counter_print(&stats->commit_calls, " -commit", out, arg);
  mi_stat_counter_print(&stats->reset_calls, "-reset", out, arg);
  mi_stat_counter_print(&stats->purge_calls, "-purge", out, arg);
  mi_stat_counter_print(&stats->guarded_alloc_count, "guarded", out, arg);
  mi_stat_print(&stats->threads, "threads", -1, out, arg);
  mi_stat_counter_print_avg(&stats->searches, "searches", out, arg);
  _mi_fprintf(out, arg, "%10s: %5zu\n", "numa nodes", _mi_os_numa_node_count());

  size_t elapsed;
  size_t user_time;
  size_t sys_time;
  size_t current_rss;
  size_t peak_rss;
  size_t current_commit;
  size_t peak_commit;
  size_t page_faults;
  mi_process_info(&elapsed, &user_time, &sys_time, &current_rss, &peak_rss, &current_commit, &peak_commit, &page_faults);
  _mi_fprintf(out, arg, "%10s: %5ld.%03ld s\n", "elapsed", elapsed/1000, elapsed%1000);
  _mi_fprintf(out, arg, "%10s: user: %ld.%03ld s, system: %ld.%03ld s, faults: %lu, rss: ", "process",
              user_time/1000, user_time%1000, sys_time/1000, sys_time%1000, (unsigned long)page_faults );
  mi_printf_amount((int64_t)peak_rss, 1, out, arg, "%s");
  if (peak_commit > 0) {
    _mi_fprintf(out, arg, ", commit: ");
    mi_printf_amount((int64_t)peak_commit, 1, out, arg, "%s");
  }
  _mi_fprintf(out, arg, "\n");
}

static mi_msecs_t mi_process_start; // = 0

// return thread local stats
static mi_stats_t* mi_get_tld_stats(void) {
  return &mi_heap_get_default()->tld->stats;
}

void mi_stats_reset(void) mi_attr_noexcept {
  mi_stats_t* stats = mi_get_tld_stats();
  mi_subproc_t* subproc = _mi_subproc();
  if (stats != &subproc->stats) { _mi_memzero(stats, sizeof(mi_stats_t)); }
  _mi_memzero(&subproc->stats, sizeof(mi_stats_t));
  if (mi_process_start == 0) { mi_process_start = _mi_clock_start(); };
}

void _mi_stats_merge_from(mi_stats_t* to, mi_stats_t* from) {
  if (to != from) {
    mi_stats_add(to, from);
    _mi_memzero(from, sizeof(mi_stats_t));
  }
}

void _mi_stats_done(mi_stats_t* stats) {  // called from `mi_thread_done`
  _mi_stats_merge_from(&_mi_subproc()->stats, stats);
}

void mi_stats_merge(void) mi_attr_noexcept {
  _mi_stats_done( mi_get_tld_stats() );
}

void mi_stats_print_out(mi_output_fun* out, void* arg) mi_attr_noexcept {
  mi_stats_merge();
  _mi_stats_print(&_mi_subproc()->stats, out, arg);
}

void mi_stats_print(void* out) mi_attr_noexcept {
  // for compatibility there is an `out` parameter (which can be `stdout` or `stderr`)
  mi_stats_print_out((mi_output_fun*)out, NULL);
}

void mi_thread_stats_print_out(mi_output_fun* out, void* arg) mi_attr_noexcept {
  _mi_stats_print(mi_get_tld_stats(), out, arg);
}


// ----------------------------------------------------------------
// Basic timer for convenience; use milli-seconds to avoid doubles
// ----------------------------------------------------------------

static mi_msecs_t mi_clock_diff;

mi_msecs_t _mi_clock_now(void) {
  return _mi_prim_clock_now();
}

mi_msecs_t _mi_clock_start(void) {
  if (mi_clock_diff == 0.0) {
    mi_msecs_t t0 = _mi_clock_now();
    mi_clock_diff = _mi_clock_now() - t0;
  }
  return _mi_clock_now();
}

mi_msecs_t _mi_clock_end(mi_msecs_t start) {
  mi_msecs_t end = _mi_clock_now();
  return (end - start - mi_clock_diff);
}


// --------------------------------------------------------
// Basic process statistics
// --------------------------------------------------------

mi_decl_export void mi_process_info(size_t* elapsed_msecs, size_t* user_msecs, size_t* system_msecs, size_t* current_rss, size_t* peak_rss, size_t* current_commit, size_t* peak_commit, size_t* page_faults) mi_attr_noexcept
{
  mi_subproc_t* subproc = _mi_subproc();
  mi_process_info_t pinfo;
  _mi_memzero_var(pinfo);
  pinfo.elapsed        = _mi_clock_end(mi_process_start);
  pinfo.current_commit = (size_t)(mi_atomic_loadi64_relaxed((_Atomic(int64_t)*)(&subproc->stats.committed.current)));
  pinfo.peak_commit    = (size_t)(mi_atomic_loadi64_relaxed((_Atomic(int64_t)*)(&subproc->stats.committed.peak)));
  pinfo.current_rss    = pinfo.current_commit;
  pinfo.peak_rss       = pinfo.peak_commit;
  pinfo.utime          = 0;
  pinfo.stime          = 0;
  pinfo.page_faults    = 0;

  _mi_prim_process_info(&pinfo);

  if (elapsed_msecs!=NULL)  *elapsed_msecs  = (pinfo.elapsed < 0 ? 0 : (pinfo.elapsed < (mi_msecs_t)PTRDIFF_MAX ? (size_t)pinfo.elapsed : PTRDIFF_MAX));
  if (user_msecs!=NULL)     *user_msecs     = (pinfo.utime < 0 ? 0 : (pinfo.utime < (mi_msecs_t)PTRDIFF_MAX ? (size_t)pinfo.utime : PTRDIFF_MAX));
  if (system_msecs!=NULL)   *system_msecs   = (pinfo.stime < 0 ? 0 : (pinfo.stime < (mi_msecs_t)PTRDIFF_MAX ? (size_t)pinfo.stime : PTRDIFF_MAX));
  if (current_rss!=NULL)    *current_rss    = pinfo.current_rss;
  if (peak_rss!=NULL)       *peak_rss       = pinfo.peak_rss;
  if (current_commit!=NULL) *current_commit = pinfo.current_commit;
  if (peak_commit!=NULL)    *peak_commit    = pinfo.peak_commit;
  if (page_faults!=NULL)    *page_faults    = pinfo.page_faults;
}
