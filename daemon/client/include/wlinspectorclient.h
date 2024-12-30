#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct String String;

typedef struct Window {
  struct String app_id;
  struct String title;
} Window;

typedef struct VecArr_Window {
  struct Window *arr;
  uintptr_t length;
  uintptr_t cap;
} VecArr_Window;

typedef struct CProcess {
  uint32_t pid;
  uint64_t last_update;
  struct VecArr_Window windows;
} CProcess;

typedef struct VecArr_CProcess {
  struct CProcess *arr;
  uintptr_t length;
  uintptr_t cap;
} VecArr_CProcess;

struct VecArr_CProcess wli_list_window_info(void);

void wli_free_window_info(struct VecArr_CProcess *info);
