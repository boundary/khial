/*
 * Copyright 2012, Boundary
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <net/if.h>

#include "khial.h"

int main(int argc, char *argv[])
{
  int fd = 0;
  struct ifreq ifr;
  int err = 0;

  unsigned long go = 696969;

  /* open a control socket */
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1) {
    perror("socket");
    exit(1);
  }

  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, "test0", IFNAMSIZ);

  /* clear all stats */
  ioctl(fd, KHIAL_PKT_CLEAR_ALL);

  /* increment RX packet count by 1 */
  err = ioctl(fd, KHIAL_PKT_RX_INCR, &ifr);
  if (err == -1) {
    perror("ioctl");
    exit(1);
  }

  /* increment TX packet count by 1 */
  ioctl(fd, KHIAL_PKT_TX_INCR, &ifr);

  /* increment TX packet count by 1 */
  ioctl(fd, KHIAL_PKT_TX_INCR, &ifr);

  /* increment the TX byte count by the value of 'go' */
  ifr.ifr_data = (caddr_t) &go;
  err = ioctl(fd, KHIAL_BYTE_TX_INCR, &ifr);
  if (err == -1) {
    perror("ioctl");
    exit(1);
  }

  /* incremenet the RX byte count byte the value of 'go' */
  go = 31337;
  ifr.ifr_data = (caddr_t) &go;
  err = ioctl(fd, KHIAL_BYTE_RX_INCR, &ifr);
  if (err == -1) {
    perror("ioctl");
    exit(1);
  }

  return 0;
}
