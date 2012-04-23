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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap/pcap.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "khial.h"

void usage(void)
{
  fprintf(stderr, "./output_packets pcap_file_name\n");
  return;
}

int main(int argc, char *argv[])
{
  size_t count = 0;
  char errbuf[PCAP_ERRBUF_SIZE];
  struct pcap_pkthdr header;
  const u_char *packet;
  pcap_t *pf;
  int fd = 0;
  FILE *file;
  int direction = 0;

  if (argc != 2) {
    usage();
    exit(1);
  }

  /* open a socket to the character device testpackets0.
   * this device corresponds to the test0 interface.
   */
  fd = open("/dev/char/testpackets0", O_WRONLY);
  if (fd < 0) {
    perror("open failed");
    exit(1);
  }
  file = fdopen(fd, "w+");

  /* set the direction as TX */
  direction = KHIAL_DIRECTION_TX;
  ioctl(fd, KHIAL_SET_DIRECTION, &direction);

  pf = pcap_open_offline(argv[1], errbuf);
  if (!pf) {
    fprintf(stderr, "Error opening pcap file %s: %s\n", argv[1], errbuf);
    exit(1);
  }

  while (packet = pcap_next(pf, &header)) {
    size_t bufsz = sizeof(header) + header.caplen;
    char *buffer = malloc(bufsz);
    if (!buffer) {
      fprintf(stderr, "not enough memory!\n");
      exit(1);
    }

    /* copy the header and the packet data into a buffer */
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer+sizeof(header), packet, header.caplen);

    /* write the blob to the character device */
    write(fd, buffer, bufsz);
    fflush(file);
    count++;
  }

  fprintf(stderr, "size of header: %zd\n", sizeof(header));
  fprintf(stderr, "packet count: %zd\n", count);
  close(fd);
  return 0;
}
