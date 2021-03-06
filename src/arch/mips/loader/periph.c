#include "common/include/data.h"
#include "common/include/memlayout.h"
#include "loader/periph.h"


static u8 io_read8(ulong addr)
{
    volatile u8 *ptr = (u8 *)(SEG_DIRECT + addr);
    return *ptr;
}

static void io_write8(ulong addr, u8 val)
{
    volatile u8 *ptr = (u8 *)(SEG_DIRECT + addr);
    *ptr = val;
}


/*
 * UART
 */
void uart_init()
{
    u32 data = 0;   // 8-bit data
    u32 parity = 0; // No parity
    u32 stop = 0;   // 1-bit stop
    
    u32 baud = 9600;
    u32 divisor = UART_MAX_BAUD / baud;
    
    // Disable interrupts
    io_write8(UART_INT_ENABLE_ADDR, 0);

    // Set up buad rate
    io_write8(UART_LINE_CTRL_ADDR, 0x80);
    io_write8(UART_DIV_LSB_ADDR, divisor & 0xff);
    io_write8(UART_DIV_MSB_ADDR, (divisor & 0xff00)>>8);
    io_write8(UART_LINE_CTRL_ADDR, 0x0);

    // Set data format
    io_write8(UART_LINE_CTRL_ADDR, data | parity | stop);
}

char uart_read()
{
    u8 ready = 0;
    while (!ready) {
        ready = io_read8(UART_LINE_STAT_ADDR) & 0x1;
    }
    
    return (char)io_read8(UART_DATA_ADDR);
}

void uart_write(char ch)
{
    u32 ready = 0;
    while (!ready) {
        ready = io_read8(UART_LINE_STAT_ADDR) & 0x20;
    }
    
    io_write8(UART_DATA_ADDR, (u8)ch & 0xff);
}


/*
 * GPIO
 */


/*
 * Print
 */
static void print_char(char ch)
{
    uart_write(ch);
}

static void print_string(char *str)
{
    char *s = str;
    
    while (*s) {
        print_char(*s);
        s++;
    }
}

static void uint_div(u32 a, u32 b, u32 *qout, u32 *rout)
{
    u32 q = 0, r = a;
    
    while (r >= b) {
        q++;
        r -= b;
    }
    
    if (qout) {
        *qout = q;
    }
    
    if (rout) {
        *rout = r;
    }
}

static void print_num_u32(char fmt, u32 num)
{
    int i;
    u32 value;
    int started = 0;
    
    switch (fmt) {
    case 'b':
        for (i = 31; i >= 0; i--) {
            print_char(num & (0x1 << i) ? '1' : '0');
        }
        break;
    case 'x':
        print_string("0x");
    case 'h':
        if (!num) {
            print_char('0');
        } else {
            for (i = 0; i < sizeof(u32) * 8; i += 4) {
                value = (num << i) >> 28;
                if (value) {
                    started = 1;
                }
                if (started) {
                    print_char(value > 9 ? (u8)(value + 0x30 + 0x27) : (u8)(value + 0x30));
                }
            }
        }
        break;
    case 'd':
        if (num & (0x1 << 31)) {
            print_char('-');
            num = ~num + 1;
        }
    case 'u':
        if (!num) {
            print_char('0');
            break;
        } else {
            int digits = 10;
            u32 dividers[] = { 1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1 };
            u32 q, r;
            
            for (i = 0; i < digits; i++) {
                uint_div(num, dividers[i], &q, &r);
                
                if (q) {
                    started = 1;
                }
                
                if (started) {
                    print_char('0' + q);
                }
                
                num = r;
            }
        }
        break;
    default:
        print_string("__unknown_format_");
        print_char(fmt);
        print_string("__");
        break;
    }
}

static void print_num_u64(char fmt, u64 num)
{
    int i;
    
    switch (fmt) {
    case 'b':
        for (i = 63; i >= 0; i--) {
            print_char(num & (0x1 << i) ? '1' : '0');
        }
        break;
    case 'x':
        print_string("0x");
    case 'h':
        if (!num) {
            print_char('0');
        } else {
            int started = 0;
            for (i = 0; i < sizeof(u64) * 8; i += 4) {
                u64 value = (num << i) >> 60;
                if (value) {
                    started = 1;
                }
                if (started) {
                    print_char(value > 9 ? (u8)(value + 0x30 + 0x27) : (u8)(value + 0x30));
                }
            }
        }
        break;
    default:
        print_string("__unknown_format_");
        print_char(fmt);
        print_string("__");
        break;
    }
}

static void print_num_ulong(char fmt, ulong num)
{
#if ARCH_WIDTH == 32
    print_num_u32(fmt, (u32)num);
#else
    print_num_u64(fmt, (u64)num);
#endif
}

void lprintf(char *fmt, ...)
{
    char *s = fmt;
    char token = 0;
    
    char *va_str;
    char va_ch;
    u32 va_u32;
    ulong va_ulong;
    
    va_list va;
    va_start(va, fmt);
    
    while (*s) {
        switch (*s) {
        case '%':
            s++;
            token = *s;
            
            switch (token) {
            case 'd':
            case 'u':
            case 'x':
                va_u32 = va_arg(va, u32);
                print_num_u32(token, va_u32);
                break;
            case 'l':
                s++;
                token = *s;
                
                switch (token) {
                case 'd':
                case 'u':
                case 'x':
                    va_ulong = va_arg(va, ulong);
                    print_num_ulong(token, va_ulong);
                    break;
                default:
                    print_char(token);
                    break;
                }
                break;
            case 's':
                va_str = va_arg(va, char *);
                print_string(va_str);
                break;
            case 'c':
                va_ch = va_arg(va, int);
                print_char(va_ch);
                break;
            case '%':
            default:
                print_char(token);
            }
            
            break;
        default:
            print_char(*s);
            break;
        }
        
        s++;
    }
    
    va_end(va);
}


/*
 * Search
 */
void scan_area(ulong start, ulong len)
{
    ulong end = start + len;
    ulong addr;
    
    lprintf("Scan area @ start: %x, end: %x\n", start, end);
    
    for (addr = start; addr < end; addr += 4) {
        char ch = (char)io_read8(addr);
        
        if (ch) {
            lprintf("%x: %c\n", addr, ch);
        }
    }
    
    lprintf("\nScan done\n");
}


/*
 * Initialization
 */
static void print_device_info()
{
    print_string("Peripherals initialized\n");
}

void init_periph()
{
    print_device_info();
}
