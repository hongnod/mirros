#ifndef _PRINTK_H
#define _PRINTK_H

int uart_puts(char *buf);
int printk(const char *fmt,...);
int level_printk(const char *fmt,...);

#define	debug(module, fmt, ...)	level_printk("3""<D>" module fmt, ##__VA_ARGS__)
#define info(module, fmt, ...)	level_printk("2""<I>" module fmt, ##__VA_ARGS__)
#define error(module, fmt, ...)	level_printk("1""<E>" module fmt, ##__VA_ARGS__)
#define fatal(module, fmt, ...)	level_printk("0""<F>" module fmt, ##__VA_ARGS__)

#define kernel_debug(fmt,...)	debug("[ KERN: ]", fmt, ##__VA_ARGS__)
#define kernel_info(fmt,...)	info("[ KERN: ]", fmt, ##__VA_ARGS__)
#define kernel_error(fmt,...)	error("[ KERN: ]", fmt, ##__VA_ARGS__)
#define kernel_fatal(fmt,...)	fatal("[ KERN: ]", fmt, ##__VA_ARGS__)

#endif
