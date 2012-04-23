### khial

khial is a kernel module that pretends to be a network device and exposes a few ways for userland programs to interact with the driver, pass packets through it, gather statistics information, or reset internal counters.

### why khial?

i wanted a more realistic way to test applications which:

  - gather/examine network interface statistics from /proc/net/dev
  - sniff packets from network devices (specifically the Boundary flow meter, but any application, really)

it turns out that a lot (but certainly not all) of testing in this area is focused on reading packet traces out of files, which is great for certain types of testing.

i wanted a way to test for strange bugs in different network configurations to detect weird bugs before shipping to customers (see also: http://blog.boundary.com/2012/04/04/a-deep-dive-to-find-a-nasty-bug/). khial helps to do this, beacuse you can ask khial to create several test devices and assign them various IP addresses, MAC addresses, and so on.

### supported operating systems and kernels

khial has been tested on:

  - 64bit Ubuntu 10.04 2.6.32-40-generic #87 SMP

other operating systems may require modifications to the source for the driver to work properly.

### how to build, load, and unload

you will need to follow the directions for building kernel modules on your linux distribution. usually, you'll just need to install some sort of kernel header file package (on Ubuntu, it might be called something like linux-headers-$(uname -r)).

once you've gotten the build enviroment set up (in the source directory):

  - make
  - sudo ./loaddriver.sh

to unload simply run:

  - sudo rmmod khial

### how to use khial

once it has been loaded, khial will expose a network device. you should be able to see it by running `ifconfig -a`.

khial exposes a character device that gets mknod'd in loaddriver.sh and appears in /dev/char/, named testpacket0, testpacket1, etc.

see output_packet.c for an example of how to write packets to khial's character device.

### bugs

i don't know much about writing network device drivers for linux, so there are probably better/more correct ways to write this driver. pull requests are happily accepted.
