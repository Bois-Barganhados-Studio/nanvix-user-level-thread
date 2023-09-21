/*
 * Copyright(C) 2023 Vinicius G. Santos <viniciusgsantos@protonmail.com>
 *
 * This file is part of Nanvix.
 *
 * Nanvix is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nanvix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nanvix. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * @brief Non-local jumps.
 */

#ifndef SETJMP_H_
#define SETJMP_H_

/**
 * @brief A buffer where the context at the `setjmp()` call is saved at.
 * Each position contains the data stored in the
 * registers in the following order:
 * ebx, esi, edi, eflags, ebp, esp, eip
*/
typedef unsigned long jmp_buf[7];

/**
 * @brief Saves current context in `buf`. 
 * Returns zero.
*/
int setjmp (jmp_buf buf);

/**
 * @brief Jumps to the context saved in `buf`.
 * Changes the `setjmp()` return to `val` or 1 if `val` is 0.
*/
void longjmp (jmp_buf buf, int val);

#endif // SETJMP_H_