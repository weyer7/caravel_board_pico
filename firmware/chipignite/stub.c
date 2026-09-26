/*
 * SPDX-FileCopyrightText: 2020 Efabless Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <defs.h>

void putchar(char c)
{
    if (c == '\n') {
        while (reg_uart_txfull == 1);
        reg_uart_data = '\r';
    }
    while (reg_uart_txfull == 1);
    reg_uart_data = c;
}

void print(const char *p)
{
    while (*p) {
        putchar(*p++);
    }
}

void print_hex(uint32_t v, int digits)
{
    if (digits < 1) digits = 1;
    if (digits > 8) digits = 8;

    for (int i = digits - 1; i >= 0; i--) {
        uint32_t nibble = (v >> (i * 4)) & 0xF;
        putchar(nibble < 10 ? '0' + nibble : 'a' + (nibble - 10));
    }
}

void print_dec(uint32_t v)
{
    if (v == 0) {
        putchar('0');
        return;
    }

    // Fixed power-of-10 lookup table (NO software division or modulo needed)
    static const uint32_t powers_of_10[] = {
        1000000000U,
         100000000U,
          10000000U,
           1000000U,
            100000U,
             10000U,
              1000U,
               100U,
                10U,
                 1U
    };

    int leading_zero = 1;

    for (int i = 0; i < 10; i++) {
        uint32_t p = powers_of_10[i];
        uint8_t count = 0;

        // Subtract power of 10 using simple RV32I integer subtraction
        while (v >= p) {
            v -= p;
            count++;
        }

        // Suppress leading zeroes (unless it's the last digit)
        if (count > 0 || !leading_zero || i == 9) {
            putchar('0' + count);
            leading_zero = 0;
        }
    }
}

void print_digit(uint32_t v)
{
    v &= 0xF;
    putchar(v < 10 ? '0' + v : 'a' + (v - 10));
}