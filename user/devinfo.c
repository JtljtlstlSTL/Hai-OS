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
    if(d->cls == HAI_DRV_BLOCK){
      printf("- %-12s class=%s inited=%d submits=%d completes=%d max_inflight=%d\n",
             d->name, cls_name(d->cls), d->inited, (int)d->metric0, (int)d->metric1, (int)d->metric2);
    } else if(d->cls == HAI_DRV_SERIAL){
      printf("- %-12s class=%s inited=%d tx_bytes=%d rx_bytes=%d flowctl=%d\n",
             d->name, cls_name(d->cls), d->inited, (int)d->metric0, (int)d->metric1, (int)d->metric2);
    } else {
      printf("- %-12s class=%s inited=%d m0=%d m1=%d m2=%d\n",
             d->name, cls_name(d->cls), d->inited, (int)d->metric0, (int)d->metric1, (int)d->metric2);
    }
  }
  exit(0);
}
