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

#include "khial.h"

#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/in.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <net/sock.h>
#include <net/sch_generic.h>
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#else
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

extern void khial_set_ethtool_ops(struct net_device *);

struct pcap_pkthdr {
  struct timeval ts;
  uint32_t caplen;
  uint32_t len;
};

struct khial {
  struct net_device *dev;
  struct cdev cdev;
  rwlock_t lock;
  struct net_device_stats net_stats;
  struct list_head khial_list;
  khial_direction_t direction;
  int packet_minor;
};

static int packet_major = 0;
static int packet_minor = 0;

/* Number of khial devices to be set up by this module. */
static int num_khial_devs = 1;
module_param(num_khial_devs, int, 0);
MODULE_PARM_DESC(num_khial_devs, "Number of khial pseudo network devices");

LIST_HEAD(khial_dev_list);

static int khial_open(struct inode *inode, struct file *filp)
{
  struct khial *dev;

  dev = container_of(inode->i_cdev, struct khial, cdev);

  filp->private_data = dev;

  /* reset all stats */
  write_lock(&dev->lock);
  dev->net_stats.rx_packets = 0;
  dev->net_stats.rx_bytes = 0;
  dev->net_stats.tx_packets = 0;
  dev->net_stats.tx_bytes = 0;
  write_unlock(&dev->lock);
  return 0;
}

static ssize_t khial_write(struct file *filp, const char __user *buf, size_t count, loff_t *fpos)
{
  ssize_t ret = 0;
  struct pcap_pkthdr hdr;
  struct khial *dev;
  struct sk_buff *skb;

  dev = filp->private_data;

  if (copy_from_user(&hdr, buf, sizeof(struct pcap_pkthdr))) {
    ret = -EFAULT;
    goto out;
  }

  skb = netdev_alloc_skb(dev->dev, hdr.len + NET_IP_ALIGN);
  if (unlikely(!skb)) {
    ret = -ENOMEM;
    goto out;
  }

  buf += sizeof(struct pcap_pkthdr);

  if (copy_from_user(skb->data, buf, hdr.caplen)) {
    ret = -EFAULT;
    goto out;
  }

  skb_put(skb, hdr.len);

  if (dev->direction == KHIAL_DIRECTION_RX) {
    skb->protocol = eth_type_trans(skb, dev->dev);
    write_lock(&dev->lock);
    dev->net_stats.rx_packets++;
    dev->net_stats.rx_bytes += hdr.len;
    write_unlock(&dev->lock);
    netif_receive_skb(skb);
  } else if (dev->direction == KHIAL_DIRECTION_TX) {
    skb_reset_network_header(skb);
    skb->priority = 1;
    dev_queue_xmit(skb);
  }

  *fpos += count;
  ret = count;

out:
  return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
static int khial_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#else
static long khial_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
  struct khial *dev = filp->private_data;
  int retval = 0;
  char __user *argp = (char __user *)arg;

  switch (cmd) {
    case KHIAL_SET_DIRECTION:
      if (copy_from_user(&dev->direction, argp, sizeof(khial_direction_t)))
        retval = -EFAULT;

      break;

    case KHIAL_GET_DIRECTION:
      if (copy_to_user(argp, &dev->direction, sizeof(khial_direction_t)))
        retval = -EFAULT;

      break;

    default:
      retval = -1;
      break;
  }

  return retval;
}

const struct file_operations khial_fops = {
  .open = khial_open,
  .write = khial_write,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,11))
  .ioctl = khial_ioctl
#else
  .unlocked_ioctl = khial_ioctl
#endif
};

static int khial_set_address(struct net_device *dev, void *p)
{
  struct sockaddr *sa = p;

  if (!is_valid_ether_addr(sa->sa_data))
    return -EADDRNOTAVAIL;

  memcpy(dev->dev_addr, sa->sa_data, ETH_ALEN);
  return 0;
}

/* fake multicast ability */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
static void set_multicast_list(struct net_device *dev)
{
  return;
}
#else
static void set_rx_mode(struct net_device *dev)
{
  return;
}
#endif

static netdev_tx_t khial_xmit(struct sk_buff *skb, struct net_device *dev)
{
  struct khial *adapter = netdev_priv(dev);

  dev_kfree_skb_any(skb);

  write_lock(&adapter->lock);
  adapter->net_stats.tx_bytes += skb->len;
  adapter->net_stats.tx_packets ++;
  write_unlock(&adapter->lock);

  return NETDEV_TX_OK;
}

static struct net_device_stats *khial_get_stats(struct net_device *dev)
{
  struct khial *adapter = netdev_priv(dev);
  return &adapter->net_stats;
}

static int khial_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
  struct khial *adapter = netdev_priv(dev);
  unsigned long len = 0;
  int retval = 0;

  write_lock(&adapter->lock);

  switch (cmd) {
    case KHIAL_PKT_CLEAR_ALL:
      adapter->net_stats.rx_packets = 0;
      adapter->net_stats.tx_packets = 0;
      adapter->net_stats.rx_bytes = 0;
      adapter->net_stats.tx_bytes = 0;
      break;
    case KHIAL_PKT_RX_INCR:
      adapter->net_stats.rx_packets++;
      break;
    case KHIAL_PKT_TX_INCR:
      adapter->net_stats.tx_packets++;
      break;
    case KHIAL_BYTE_TX_INCR:
      if (copy_from_user(&len, ifr->ifr_data, sizeof(len))) {
        retval = -EFAULT;
        goto out;
      }
      adapter->net_stats.tx_bytes += len;
      break;
    case KHIAL_BYTE_RX_INCR:
      if (copy_from_user(&len, ifr->ifr_data, sizeof(len))) {
        retval = -EFAULT;
        goto out;
      }
      adapter->net_stats.rx_bytes += len;
      break;
    default:
      retval = -EOPNOTSUPP;
  }

out:
  write_unlock(&adapter->lock);

  return 0;
}

static const struct net_device_ops khial_netdev_ops = {
  .ndo_start_xmit = khial_xmit,
  .ndo_validate_addr = eth_validate_addr,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
  .ndo_set_multicast_list = set_multicast_list,
#else
  .ndo_set_rx_mode = set_rx_mode,
#endif
  .ndo_set_mac_address = khial_set_address,
  .ndo_do_ioctl = khial_do_ioctl,
  .ndo_get_stats = khial_get_stats,
};

static void khial_setup(struct net_device *dev)
{
  struct khial *adapter;

  /* Initialize the device structure. */
  dev->netdev_ops = &khial_netdev_ops;

  /* Fill in device structure with ethernet-generic values. */
  ether_setup(dev);

  dev->tx_queue_len = 0;
  dev->mtu = ETH_DATA_LEN;
  dev->hard_header_len = ETH_HLEN;
  dev->addr_len = ETH_ALEN;

  /* make sure the device is broadcast, multicast, and UP */
  dev->flags |= (IFF_MULTICAST | IFF_BROADCAST);
  dev->features = NETIF_F_LLTX | NETIF_F_HW_CSUM;
  random_ether_addr(dev->perm_addr);
  random_ether_addr(dev->dev_addr);

  adapter = netdev_priv(dev);
  memset(adapter, 0, sizeof(struct khial));
  adapter->dev = dev;

  rwlock_init(&adapter->lock);

  list_add_tail(&adapter->khial_list, &khial_dev_list);

  return;
}

static int khial_init_one(struct khial **adapter)
{
  struct net_device *khial_dev;
  int err;

  khial_dev = alloc_netdev(sizeof(struct khial), "test%d", khial_setup);

  if (!khial_dev) {
    printk(KERN_WARNING "Error, can't alloc netdev!");
    err = -ENOMEM;
    goto out;
  }

  err = dev_alloc_name(khial_dev, khial_dev->name);
  if (err < 0) {
    err = -ENOMEM;
    goto out_free;
  }

  err = register_netdev(khial_dev);
  if (err < 0) {
    err = -ENOMEM;
    goto out_free;
  }

  khial_set_ethtool_ops(khial_dev);
  netif_carrier_on(khial_dev);
  *adapter = netdev_priv(khial_dev);
  return 0;

out_free:
  free_netdev(khial_dev);

out:
  return err;
}

static int setup_cdev(struct khial *adapter, int idx)
{
  dev_t devno = MKDEV(packet_major, packet_minor + idx);
  cdev_init(&(adapter->cdev), &khial_fops);

  adapter->cdev.owner = THIS_MODULE;
  adapter->cdev.ops = &khial_fops;

  return cdev_add(&adapter->cdev, devno, 1);
}

static void khial_dev_free(struct net_device *dev)
{
  dev_close(dev);
  unregister_netdev(dev);
  free_netdev(dev);
}

static void khial_free_all(void)
{
  struct khial *adapter, *next;

  list_for_each_entry_safe(adapter, next, &khial_dev_list, khial_list) {
    struct net_device *dev = adapter->dev;
    cdev_del(&adapter->cdev);
    khial_dev_free(dev);
    list_del(&adapter->khial_list);
  }

  return;
}

static int __init khial_init_module(void)
{
  int i, err = 0;
  dev_t dev = 0;
  struct khial *adapter;

  err = alloc_chrdev_region(&dev, 0, num_khial_devs, "testpackets");
  if (err < 0) {
    printk(KERN_WARNING "unable to allocate char devices for khial: %d\n", err);
    goto out;
  }

  packet_major = MAJOR(dev);

  for (i = 0; i < num_khial_devs && !err; i++) {
    err = khial_init_one(&adapter);
    if (err < 0) {
      printk("Unable to initialize device!");
      goto out_unregister;
    }


    err = setup_cdev(adapter, i);
    if (err < 0) {
      printk(KERN_WARNING "Error %d adding char dev!", err);
      goto out_deinit;
    }

    rtnl_lock();
    dev_open(adapter->dev);
    rtnl_unlock();
  }

out:
  return err;

out_deinit:
  khial_free_all();

out_unregister:
  unregister_chrdev_region(dev, num_khial_devs);
  return -1;
}

static void __exit khial_cleanup_module(void)
{
  dev_t devno = MKDEV(packet_major, 0);
  khial_free_all();
  unregister_chrdev_region(devno, num_khial_devs);
  return;
}

module_init(khial_init_module);
module_exit(khial_cleanup_module);
MODULE_LICENSE("GPL");
