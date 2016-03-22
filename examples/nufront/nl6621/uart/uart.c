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

static void delay(uint32_t count)
{

    while(count--){
        __asm__ __volatile__("nop");
    }
}

static void __attribute__((noinline)) uart_putc(const char data)
{
    while((UART_LSR & UART_LSR_THRE) == 0)
        ;

    UART_THR = data;
}

static void __attribute__((noinline)) uart_puts(const char *str) 
{
    bool cr_sent;

    cr_sent = false;
    while(*str != '\0'){
        if(*str == '\n' && cr_sent == false){
            uart_putc('\r');
            cr_sent = true;
            continue;
        }

        cr_sent = false;
        if(*str == '\r'){
            cr_sent = true;
        }

        uart_putc(*str++);
    }
}

static uint8_t uart_getc(void)
{
    uint8_t data;

    while((UART_LSR & UART_LSR_DR) == 0)
        ;

    data = UART_RBR & 0xff;

    return data;
}


int main(void)
{

    uint16_t uart_div;

    SYSCFG_MODE = SYSCFG_MODE_BW_20M | SYSCFG_MODE_BW_EN | SYSCFG_MODE_CLK_160M 
                | SYSCFG_MODE_CLK_EN | SYSCFG_MODE_SIMD_RST;

    SYSCFG_CLK_CTRL = SYSCFG_CLK_APB_40M | SYSCFG_CLK_WLAN_GATE_EN 
                    | SYSCFG_CLK_APB1_GATE_EN | SYSCFG_CLK_APB2_GATE_EN;


    SYSCFG_MUX &= ~(GPIO_UART_EN | GPIO_I2S_EN);
    SYSCFG_MUX |= (SYSCFG_MUX_UART | SYSCFG_MUX_JTAG);


    UART_LCR = (UART_LCR_DLAB | UART_LCR_STOP | UART_LCR_DLS_8BIT);
    delay(10);

    uart_div = (APB_CLOCK + (UART_BAUD * 8)) / (UART_BAUD * 16);
    UART_DLH = (uart_div >> 8) & 0xff;
    delay(10);

    UART_DLL = uart_div & 0xff;
    delay(10);

    UART_LCR = UART_LCR_DLS_8BIT;
    delay(10);

    UART_FCR = UART_FCR_FIFO_EN;
    delay(10);

    while(1){
        uart_puts("NL6621 running...\n");
        delay(CPU_CLOCK / 5);
    }
    
    return 0;
}
