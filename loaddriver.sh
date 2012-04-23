#!/bin/bash
set -x
# Copyright 2012, Boundary
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

module="khial"
device="testpackets"
mode="664"

num_devs=$1
if [ -z $num_devs ]; then
  num_devs=1
fi

/sbin/insmod ./$module.ko num_khial_devs=$num_devs || exit 1

# remove stale nodes
rm -f /dev/char/${device}*

major=$(awk "\$2==\"$device\" {print \$1}" /proc/devices)

for (( i = 0; i < $num_devs; i++ ))
do
  mknod /dev/char/${device}${i} c $major $i
done

# give appropriate group/permissions, and change the group.
# Not all distributions have staff, some have "wheel" instead.
group="root"
grep -q '^staff:' /etc/group || group="wheel"

chgrp $group /dev/char/${device}0
chmod $mode  /dev/char/${device}0
