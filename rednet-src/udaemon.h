#ifndef REDNET_UDAEMON_H
#define REDNET_UDAEMON_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

namespace rednet
{
class udaemon
{
  public:
    udaemon(const char *pidfile);
    ~udaemon();

    int init();
    int exit();

  private:
    int local_check_pid();
    int local_write_pid();
    int local_redirect_fds();

  private:
    char *pidfile__;
};

} // namespace rednet

#endif
