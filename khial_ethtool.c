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

#include <linux/string.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>

struct khial_stats {
  char stat_string[ETH_GSTRING_LEN];
  int sizeof_stat;
  int stat_offset;
};

static const char *khial_driver_name = "khial faux nic";
static const char *khial_driver_version = "JOEXTREEM v1";
static const char *fw_version = "12.19.84";

static void khial_get_drvinfo(struct net_device *netdev,
    struct ethtool_drvinfo *drvinfo)
{
  strncpy(drvinfo->driver,  khial_driver_name, 32);
  strncpy(drvinfo->version, khial_driver_version, 32);
  strncpy(drvinfo->fw_version, fw_version, 32);
}

static const struct ethtool_ops khial_ethtool_ops = {
  .get_drvinfo            = khial_get_drvinfo,
};

void khial_set_ethtool_ops(struct net_device *netdev)
{
  SET_ETHTOOL_OPS(netdev, &khial_ethtool_ops);
}
