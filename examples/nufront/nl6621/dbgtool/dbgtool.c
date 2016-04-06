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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <libopencm3/nufront/syscfg.h>
#include <libopencm3/nufront/gpio.h>
#include <libopencm3/nufront/uart.h>
#include <libopencm3/nufront/flash.h>

#define UART_BAUD       19200u

uint8_t read_buf[256];

void hexdump(uint32_t base_addr, const uint8_t buf[], size_t len);
void handle_flash(void);
void handle_dump(void);
void handle_uart(void);
void show_help(void);
int _write(int fd, char *ptr, int len);
int _read(int fd, char *ptr, int len);
void get_buffered_line(void);

void hexdump(uint32_t base_addr, const uint8_t buf[], size_t len)
{
    size_t printed;
    uint8_t xor;

    printed = 0;
    xor = 0;
    while(printed < len){
        if(printed % 16 == 0){
            iprintf("%08lx:", base_addr + printed);
        }

        iprintf(" %02x", buf[printed]);
        xor ^= buf[printed];

        ++printed;

        if(printed % 16 == 0){
            iprintf(" : %02x\n", xor);
            xor = 0;
        }
    }
}

enum flash_cmd {
    cmd_read,
    cmd_write,
    cmd_erase,
    cmd_dump,
};

void handle_flash(void)
{
    unsigned long fl_addr, ram_addr;
    size_t len, chunk;
    char *token;
    enum flash_cmd cmd;
    int result;
    
    token = strtok(NULL, " ");
    if(token == NULL){
        goto err_out;
    }

    if(!strcmp(token, "read")){
        cmd = cmd_read;
    } else if(!strcmp(token, "write")){
        cmd = cmd_write;
    } else if(!strcmp(token, "erase")){
        cmd = cmd_erase;
    } else if(!strcmp(token, "dump")){
        cmd = cmd_dump;
    } else {
        show_help();
        goto err_out;
    }       

    token = strtok(NULL, " ");
    if(token == NULL){
        goto err_out;
    }

    errno = 0;
    fl_addr = strtoul(token, NULL, 0);
    if(errno != 0){
        goto err_out;
    }

    token = strtok(NULL, " ");
    if(token == NULL){
        goto err_out;
    }

    errno = 0;
    len = strtoul(token, NULL, 0);
    if(errno != 0){
        goto err_out;
    }

    ram_addr = 0;
    if(cmd == cmd_read || cmd == cmd_write){ 
        token = strtok(NULL, " ");
        if(token == NULL){
            goto err_out;
        }

        errno = 0;
        ram_addr = strtoul(token, NULL, 0);
        if(errno != 0){
            goto err_out;
        }
    }

    result = flash_init();
    if(result != 0){
        iprintf("flash_init() failed\n");
        goto err_out;
    }

    switch(cmd){
    case cmd_read:
        result = flash_read(fl_addr, (uint8_t *) ram_addr, len);
        break;
    case cmd_write:
        result = flash_write(fl_addr, (const uint8_t *) ram_addr, len);
        break;
    case cmd_erase:
        result = flash_erase(fl_addr, len);
        break;
    case cmd_dump:
        while(len > 0){
            chunk = len < sizeof(read_buf) ? len : sizeof(read_buf);
            result = flash_read(fl_addr, read_buf, chunk);
            if(result != 0){
                goto err_out;
            }

            hexdump(fl_addr, read_buf, chunk);
            fl_addr += chunk;
            len -= chunk;
        }
        break;
    }

err_out:
    return;
}

void handle_dump(void)
{
    uintptr_t addr;
    size_t len;
    char *token;

    token = strtok(NULL, " ");
    if(token == NULL){
        goto err_out;
    }
    
    errno = 0;
    addr = strtoul(token, NULL, 0);
    if(errno != 0){
        goto err_out;
    }

    token = strtok(NULL, " ");
    if(token == NULL){
        goto err_out;
    }
    
    errno = 0;
    len = strtoul(token, NULL, 0);
    if(errno != 0){
        goto err_out;
    }

    hexdump(addr, (uint8_t *) addr, len);

err_out:
    return;
}

void handle_uart(void)
{
    uint32_t rate;
    char *token;

    token = strtok(NULL, " ");
    if(token == NULL){
        goto err_out;
    }

    errno = 0;
    rate = strtoul(token, NULL, 0);
    if(errno != 0){
        goto err_out;
    }

    iprintf("\nswitching to %ld\n", rate);
    uart_init(rate);
    
err_out:
    return;
}

void show_help(void)
{
    iprintf("Available commands:\n");
    iprintf("flash read <flash addr> <len> <RAM addr>\n");
    iprintf("  copy <len> bytes from <flash address> to <RAM addr>\n\n");

    iprintf("flash write <flash addr> <len> <RAM addr>\n");
    iprintf("  write <len> bytes from <RAM address> to <flash addr>\n\n");

    iprintf("flash erase <start addr> <len>\n");
    iprintf("  erase flash from eraseblock containing <start address> \n");
    iprintf("  to eraseblock containing <start address> + <len> \n");
    iprintf("  WARNING: unless <start address> and <len> are aligned to\n");
    iprintf("  0x%x, this will erase more than you think!\n\n", 
              flash_get_erasesize());

    iprintf("flash dump <addr> <len>\n");
    iprintf("  print hexdump of length <len> from <addr>\n\n");

    iprintf("uart <speed>\n");
    iprintf("  set UART to <speed>\n\n");

    iprintf("dump <addr> <len>\n");
    iprintf("  print hexdump of length <len> from memory at <addr>\n\n");
}

int main(void)
{
    char *token;

    syscfg_clk_init(cpuclk_160, apbclk_40, gpiobw_20);

    uart_init(19200);
    iprintf("NL6621 debug tool ready.\n");

    while(1){
        iprintf(">");
        fflush(stdout);
        fgets((char *) read_buf, sizeof(read_buf) - 1, stdin);
        read_buf[sizeof(read_buf) - 1] = '\0';

        token = strtok((char *) read_buf, " ");
        if(token == NULL){
            continue;
        }

        if(!strcmp(token, "flash")){
            handle_flash();
        } else if(!strcmp(token, "dump")){
            handle_dump();
        } else if(!strcmp(token, "uart")){
            handle_uart();
        } else {
            show_help();
        }
    }

    return 0;
}


/*
 * TKL: line editing code stolen from 
 *        stm32/f4/nucleo-f411re/usart-stdio/usart-stdio.c
 *
 * To implement the STDIO functions you need to create
 * the _read and _write functions and hook them to the
 * USART you are using. This example also has a buffered
 * read function for basic line editing.
 */

/*
 * This is a pretty classic ring buffer for characters
 */
#define BUFLEN 127

static uint16_t start_idx;
static uint16_t end_idx;
static char line_buf[BUFLEN+1];
#define buf_len ((end_idx - start_idx) % BUFLEN)
static inline int next_idx(int n) { return ((n + 1) % BUFLEN); }
static inline int prev_idx(int n) { return (((n + BUFLEN) - 1) % BUFLEN); }


/* back up the cursor one space */
static inline void back_up(void)
{
    end_idx = prev_idx(end_idx);
    uart_putc('\010');
    uart_putc(' ');
    uart_putc('\010');
}

/*
 * A buffered line editing function.
 */
void get_buffered_line(void) {
    char    c;

    if (start_idx != end_idx) {
        return;
    }
    do{
        c = uart_getc();

        switch(c){
        case '\r':
            line_buf[end_idx] = '\n';
            end_idx = next_idx(end_idx);
            line_buf[end_idx] = '\0';
            uart_putc('\r');
            uart_putc('\n');
            break;
         
        /* ^H or DEL erase a character */
        case '\010':
        case '\177':
            if(buf_len == 0){
                uart_putc('\a');
            }else{
                back_up();
            }
            break;

        /* ^W erases a word */
        case 0x17:
            while((buf_len > 0) && !(isspace((int) line_buf[end_idx]))){
                back_up();
            }
            break;

        /* ^U erases the line */
        case 0x15:
            while(buf_len > 0){
                back_up();
            }
            break;

        /* Non-editing character so insert it */
        default:
            if(buf_len == (BUFLEN - 1)){
                uart_putc('\a');
            }else{
                line_buf[end_idx] = c;
                end_idx = next_idx(end_idx);
                uart_putc(c);
            }
        }
    }while(c != '\r');
}

/*
 * Called by libc stdio fwrite functions
 */
int _write(int fd, char *ptr, int len)
{
    bool cr_sent;
    int sent;

    if(fd > 2){
        return -1;
    }

    sent = len;
    cr_sent = false;
    while(len > 0){
        if(*ptr == '\n' && cr_sent == false){
            uart_putc('\r');
            cr_sent = true;
            continue;
        }

        cr_sent = false;
        if(*ptr == '\r'){
            cr_sent = true;
        }

        uart_putc(*ptr++);
        --len;
    }

    return sent;
}


/*
 * Called by the libc stdio fread fucntions
 *
 * Implements a buffered read with line editing.
 */
int _read(int fd, char *ptr, int len)
{
    int my_len;

    if(fd > 2){
        return -1;
    }

    get_buffered_line();
    
    my_len = 0;
    while((buf_len > 0) && (len > 0)){
        *ptr++ = line_buf[start_idx];
        start_idx = next_idx(start_idx);
        my_len++;
        len--;
    }

    return my_len; /* return the length we got */
}

