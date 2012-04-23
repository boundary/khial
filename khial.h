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

#ifndef __KHIAL_H__
#define __KHIAL_H__

/* these are the file_operations ioctls */
#define KHIAL_SET_DIRECTION  (0x1)
#define KHIAL_GET_DIRECTION  (0x2)

/* these are the directions */
typedef enum {
  KHIAL_DIRECTION_RX = 0,
  KHIAL_DIRECTION_TX,
} khial_direction_t;

/* these are the netdev ioctls */
#define KHIAL_PKT_CLEAR_ALL  (0x89F0)
#define KHIAL_PKT_RX_INCR    (0x89F1)
#define KHIAL_PKT_TX_INCR    (0x89F2)
#define KHIAL_BYTE_TX_INCR   (0x89F3)
#define KHIAL_BYTE_RX_INCR   (0x89F4)

#endif /* __KHIAL_H__ */
