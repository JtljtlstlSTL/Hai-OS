#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/hai_sysinfo.h"
#include "user/user.h"

static const char *cls_name(int cls)
{
  switch(cls){
    case HAI_DRV_IRQ: return "irq";
    case HAI_DRV_BLOCK: return "block";
    case HAI_DRV_SERIAL: return "serial";
    default: return "other";
  }
}

int
main(int argc, char **argv)
{
  struct hai_devinfo di;
  if(devinfo(&di) < 0){
    printf("devinfo: syscall failed\n");
    exit(1);
  }
  printf("Drivers (%d):\n", di.count);
  for(int i = 0; i < di.count; i++){
    struct hai_driver *d = &di.devs[i];
    printf("- %-12s class=%s inited=%d\n", d->name, cls_name(d->cls), d->inited);
  }
  exit(0);
}
