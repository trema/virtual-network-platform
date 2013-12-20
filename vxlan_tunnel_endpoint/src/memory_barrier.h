/*
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2013 NEC Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef MEMORY_BARRIER_H
#define MEMORY_BARRIER_H


#if defined __i386__ || defined __amd64__
#define memory_barrier() asm volatile( "mfence" ::: "memory" )
#define read_memory_barrier() asm volatile( "lfence" ::: "memory" )
#define write_memory_barrier() asm volatile( "sfence" ::: "memory" )
#else
#define memory_barrier() __sync_synchronize()
#define read_memory_barrier() __sync_synchronize()
#define write_memory_barrier() __sync_synchronize()
#endif


#endif // MEMORY_BARRIER_H
