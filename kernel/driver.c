#include "types.h"
#include "param.h"
#include "riscv.h"
#include "defs.h"
#include "hai_sysinfo.h"

// 简单的驱动注册/初始化框架，提供统一日志与按 hart 初始化。

enum driver_class {
  DRV_IRQ = 0,
  DRV_BLOCK = 1,
  DRV_SERIAL = 2,
  DRV_OTHER = 3,
};

struct driver {
  const char *name;
  enum driver_class cls;
  int (*init)(void);
  int (*hart_init)(void);
  int inited;
};

#define MAX_DRIVERS 8
static struct driver drivers[MAX_DRIVERS];
static int ndrivers = 0;
static int builtins_registered = 0;

static int
plic_init_wrapper(void)
{
  plicinit();
  return 0;
}

static int
plic_hart_init_wrapper(void)
{
  plicinithart();
  return 0;
}

static int
virtio_init_wrapper(void)
{
  virtio_disk_init();
  return 0;
}

// driver-specific telemetry hooks (weak if driver未实现)。
__attribute__((weak)) void virtio_driver_stats(struct hai_driver *d) { (void)d; }
__attribute__((weak)) void uart_driver_stats(struct hai_driver *d) { (void)d; }

static void
register_driver(const char *name, enum driver_class cls, int (*init)(void), int (*hart_init)(void))
{
  if(ndrivers >= MAX_DRIVERS)
    panic("driver: overflow");
  drivers[ndrivers].name = name;
  drivers[ndrivers].cls = cls;
  drivers[ndrivers].init = init;
  drivers[ndrivers].hart_init = hart_init;
  drivers[ndrivers].inited = 0;
  ndrivers++;
}

void
register_builtin_drivers(void)
{
  if(builtins_registered)
    return;
  builtins_registered = 1;
  register_driver("plic", DRV_IRQ, plic_init_wrapper, plic_hart_init_wrapper);
  register_driver("virtio-blk", DRV_BLOCK, virtio_init_wrapper, 0);
}

void
init_drivers(void)
{
  for(int i = 0; i < ndrivers; i++){
    if(drivers[i].inited)
      continue;
    if(drivers[i].init){
      int r = drivers[i].init();
      klog(LOG_INFO, "driver: init name=%s class=%d status=%d", drivers[i].name, drivers[i].cls, r);
    }
    drivers[i].inited = 1;
  }
}

void
hart_init_drivers(void)
{
  for(int i = 0; i < ndrivers; i++){
    if(drivers[i].hart_init){
      drivers[i].hart_init();
      klog(LOG_INFO, "driver: hart-init name=%s", drivers[i].name);
    }
  }
}

// 将驱动表快照导出给用户态 devinfo。
void
driver_snapshot(struct hai_devinfo *info)
{
  int count = (ndrivers < HAI_MAX_DRIVERS) ? ndrivers : HAI_MAX_DRIVERS;
  info->count = count;
  for(int i = 0; i < count; i++){
    safestrcpy(info->devs[i].name, drivers[i].name, sizeof(info->devs[i].name));
    info->devs[i].cls = drivers[i].cls;
    info->devs[i].inited = drivers[i].inited;
    if(strncmp(drivers[i].name, "virtio-blk", 16) == 0)
      virtio_driver_stats(&info->devs[i]);
    else if(strncmp(drivers[i].name, "uart", 16) == 0)
      uart_driver_stats(&info->devs[i]);
  }
}
