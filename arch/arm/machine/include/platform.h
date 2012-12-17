#ifndef _PLATFORM_H
#define _PLATFORM_H

void platform_clk_init(void);

int platform_get_irq_id(void);
int platform_irq_init(void);
int platform_disable_irq(int nr);
int platform_enable_irq(int nr);
void platform_irq_clean_pending(void);

int platform_uart0_init(void);
void platform_uart0_send_byte(u16 ch);

#endif
