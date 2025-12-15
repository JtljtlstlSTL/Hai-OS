//
// formatted console output -- printf, panic.
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

volatile int panicking = 0; // printing a panic message
volatile int panicked = 0; // spinning forever at end of a panic

// lock to avoid interleaving concurrent printf's.
static struct {
  struct spinlock lock;
} pr;

static char digits[] = "0123456789abcdef";

static void vprintfmt(char *fmt, va_list ap);
static void print_level_prefix(enum log_level level);

// 全局日志级别，可运行时调节以抑制噪声。
static enum log_level current_log_level = LOG_INFO;

static void
printint(long long xx, int base, int sign)
{
  char buf[20];
  int i;
  unsigned long long x;

  if(sign && (sign = (xx < 0)))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}

static void
printptr(uint64 x)
{
  int i;
  consputc('0');
  consputc('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the console.
int
printf(char *fmt, ...)
{
  va_list ap;

  if(panicking == 0)
    acquire(&pr.lock);

  va_start(ap, fmt);
  vprintfmt(fmt, ap);
  va_end(ap);

  if(panicking == 0)
    release(&pr.lock);

  return 0;
}

int
klog(enum log_level level, char *fmt, ...)
{
  va_list ap;

  // 低于当前等级的日志直接丢弃（panic 之外），保持输出可控。
  if(level < current_log_level)
    return 0;

  if(panicking == 0)
    acquire(&pr.lock);

  // 输出 Hai-OS 品牌化日志前缀，再复用 printf 的格式化逻辑。
  print_level_prefix(level);

  va_start(ap, fmt);
  vprintfmt(fmt, ap);
  va_end(ap);

  consputc('\n');

  if(panicking == 0)
    release(&pr.lock);

  return 0;
}

enum log_level
klog_set_level(enum log_level level)
{
  enum log_level previous = current_log_level;
  current_log_level = level;
  return previous;
}

enum log_level
klog_get_level(void)
{
  return current_log_level;
}

void
panic(char *s)
{
  panicking = 1;
  printf("panic: ");
  printf("%s\n", s);
  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;
}

void
printfinit(void)
{
  initlock(&pr.lock, "pr");
}

static void
vprintfmt(char *fmt, va_list ap)
{
  int i, cx, c0, c1, c2;
  char *s;

  for(i = 0; (cx = fmt[i] & 0xff) != 0; i++){
    if(cx != '%'){
      consputc(cx);
      continue;
    }
    i++;
    c0 = fmt[i+0] & 0xff;
    c1 = c2 = 0;
    if(c0) c1 = fmt[i+1] & 0xff;
    if(c1) c2 = fmt[i+2] & 0xff;
    if(c0 == 'd'){
      printint(va_arg(ap, int), 10, 1);
    } else if(c0 == 'l' && c1 == 'd'){
      printint(va_arg(ap, uint64), 10, 1);
      i += 1;
    } else if(c0 == 'l' && c1 == 'l' && c2 == 'd'){
      printint(va_arg(ap, uint64), 10, 1);
      i += 2;
    } else if(c0 == 'u'){
      printint(va_arg(ap, uint32), 10, 0);
    } else if(c0 == 'l' && c1 == 'u'){
      printint(va_arg(ap, uint64), 10, 0);
      i += 1;
    } else if(c0 == 'l' && c1 == 'l' && c2 == 'u'){
      printint(va_arg(ap, uint64), 10, 0);
      i += 2;
    } else if(c0 == 'x'){
      printint(va_arg(ap, uint32), 16, 0);
    } else if(c0 == 'l' && c1 == 'x'){
      printint(va_arg(ap, uint64), 16, 0);
      i += 1;
    } else if(c0 == 'l' && c1 == 'l' && c2 == 'x'){
      printint(va_arg(ap, uint64), 16, 0);
      i += 2;
    } else if(c0 == 'p'){
      printptr(va_arg(ap, uint64));
    } else if(c0 == 'c'){
      consputc(va_arg(ap, uint));
    } else if(c0 == 's'){
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
    } else if(c0 == '%'){
      consputc('%');
    } else if(c0 == 0){
      break;
    } else {
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c0);
    }

  }
}

static void
print_level_prefix(enum log_level level)
{
  static const char *levels[] = {"INFO", "WARN", "ERROR", "DEBUG"};
  const char *tag = "INFO";
  if(level >= 0 && level < (int)(sizeof(levels)/sizeof(levels[0])))
    tag = levels[level];

  // 统一日志前缀格式：[Hai-OS hX tYYY LEVEL]
  consputc('[');
  const char *brand = "Hai-OS";
  while(*brand) consputc(*brand++);
  consputc(' ');
  consputc('h');
  printint(cpuid(), 10, 0);
  consputc(' ');
  consputc('t');
  printint((long long)r_time(), 10, 0);
  consputc(' ');
  for(const char *p = tag; *p; p++)
    consputc(*p);
  consputc(']');
  consputc(' ');
}
