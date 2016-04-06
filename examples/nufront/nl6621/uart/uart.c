/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Tido Klaassen <tido@4gh.eu>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/nufront/syscfg.h>
#include <libopencm3/nufront/gpio.h>
#include <libopencm3/nufront/uart.h>

#define CPU_CLOCK   160000000u
#define APB_CLOCK    40000000u
#define UART_BAUD       19200u

int main(void)
{
    char rcvd, last;
    syscfg_clk_init(cpuclk_160, apbclk_40, gpiobw_20);

    uart_init(19200);

    uart_puts("NL6621 running...\n");

    last = '\0';
    while(1){
        rcvd = uart_getc();

        if(rcvd != '\n' || last != '\r'){
            uart_putc(rcvd);
        }

        if(rcvd == '\r'){
            uart_putc('\n');
        }

        last = rcvd;
    }
    
    return 0;
}
