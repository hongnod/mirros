#include <os/io.h>
#include <os/types.h>
#include <os/string.h>
#include <os/interrupt.h>
#include <os/io.h>
#include <os/init.h>
#include <os/errno.h>
#include <os/printk.h>
#include "include/s3c2440_reg.h"
#include <asm/asm_interrupt.h>
#include "include/s3c2440_irq.h"

extern int os_tick_handler(void *arg);

static void *irq_base = NULL;
static void *timer_base = NULL;
static void *clk_base = NULL;
static void *uart_base = NULL;

u32 HCLK,FCLK,PCLK,UCLK;	//the clock of the system

/*
 *code for s3c2440 clock management
 */
void platform_clk_init(void)
{
	u32 fin = 12000000;
	u32 tmp,m,s,p;

	tmp = ioread32(clk_base + MPLLCON);
	m = (tmp >> 12) & 0xff;
	p = (tmp >> 4) & 0x3f;
	s = tmp & 0x03;
	
	FCLK = ((m+8)*(fin/100)*2)/((p+2)*(1<<s))*100;

	tmp = ioread32(clk_base + CLKDIVN);
	m = (tmp >> 1) & 0x03;
	p = tmp & 0x01;
	tmp = ioread32(clk_base + CAMDIVN);
	s = tmp >> 8;

	switch(m){
		case 0:
			HCLK = FCLK;
			break;

		case 1:
			HCLK = FCLK >> 1;
			break;
		case 2:
		{
			if(s & 2){
				HCLK = FCLK >> 3;
			}
			else{
				HCLK = FCLK >> 2;
			}

			break;
		}

		case 3:
		{
			if( s & 0x01){
				HCLK = FCLK/6;
			}
			else{
				HCLK = FCLK/3;
			}

			break;
		}

	}

	if(p)
		PCLK = HCLK >> 1;
	else
		PCLK = HCLK;

	tmp = ioread32(clk_base + UPLLCON);
	m = (tmp >> 12) & 0xff;
	p = (tmp >> 4) & 0x3f;
	s = tmp & 0x03;

	UCLK = ((m +8) * fin) / ((p + 2) * (1 << s));
}

/*
 *code for platform timer help function
 */
irq_t platform_timer_tick_handler(void *arg)
{
	os_tick_handler(arg);

	return 0;
}

int timer_tick_init(int ms)
{
	timer_base = request_io_mem(TIMER_BASE);
	if(timer_base == NULL){
		kernel_error("timer tick init failed\n");
		return -ENOMEM;
	}

	register_irq(TIMER0,platform_timer_tick_handler,NULL);
	
	iowrite32(249,timer_base + TCFG0);
	iowrite32(0,timer_base+TCFG1);
	iowrite32(64536,timer_base +TCNTB0);
	iowrite32(2,timer_base + TCON);
	iowrite32(9,timer_base + TCON);

	return 0;
}

/*
 *code for s3c2440 irq handler help function
 */
int platform_get_irq_id(void)
{
	return ioread32(irq_base+INTOFFSET);
}

int platform_irq_init(void)
{
	irq_base = request_io_mem(IRQ_BASE);
	if(irq_base == NULL){
		kernel_error("request 0x%x failed\n",IRQ_BASE);
		return -ENOMEM;
	}

	/*
	 *mask all interrupt,and set all int to irq mode
	 */
	iowrite32(0xffffffff,irq_base + INTMSK);
	iowrite32(0xffffffff,irq_base + INTSUBMSK);
	iowrite32(0,irq_base + INTMOD);

	return 0;
}

int platform_disable_irq(int nr)
{
	u32 int_msk = ioread32(irq_base + INTMSK);
	u32 int_submsk = ioread32(irq_base + INTSUBMSK);

	/*
	 *get the intmsk and int mode
	 */
	int_msk |= bit(nr);

	/*
	 *get the value of submsk
	 */
	switch(nr){
		case WDT_AC97:
			int_submsk |= (bit(AC_97) | bit(WDT));
			break;

		case UART0:
			int_submsk |= (bit(RXD0) | bit(TXD0) | bit(ERR0));
			break;

		case UART1:
			int_submsk |= ((bit(RXD1)) | (bit(TXD1)) | (bit(ERR1)));
			break;

		case UART2:
			int_submsk |= (bit(RXD2) | bit(TXD2) | bit(ERR2));
			break;
			
		case ADC:
			int_submsk |= ((bit(ADC_S)) | (bit(TC)));
			break;

		case CAM:
			int_submsk |= ((bit(CAM_P)) | (bit(CAM_C)));
			break;

		default:
			break;
	}

	iowrite32(int_msk,irq_base + INTMSK);
	iowrite32(int_submsk,irq_base + INTSUBMSK);

	return 0;
}

int platform_enable_irq(int nr)
{
	int int_msk = ioread32(irq_base + INTMSK);
	int int_submsk = ioread32(irq_base + INTSUBMSK);

	/*
	 *get the intmsk and int mode
	 */
	int_msk &= ~bit(nr);

	/*
	 *get the value of submsk
	 */
	switch(nr){
		case WDT_AC97:
			int_submsk &= ((~bit(AC_97)) & (~bit(WDT)));
			break;

		case UART0:
			int_submsk &= ((~bit(RXD0)) & (~bit(TXD0)) & (~bit(ERR0)));
			break;

		case UART1:
			int_submsk &= ((~bit(RXD1)) & (~bit(TXD1)) & (~bit(ERR1)));
			break;

		case UART2:
			int_submsk &= ( (~bit(RXD2)) & (~bit(TXD2)) & (~bit(ERR2)));
			break;
			
		case ADC:
			int_submsk &= ((~bit(ADC_S)) & (~bit(TC)));
			break;

		case CAM:
			int_submsk &= ((~bit(CAM_P)) & (~bit(CAM_C)));
			break;

		default:
			break;
	}

	iowrite32(int_msk,irq_base + INTMSK);
	iowrite32(int_submsk,irq_base + INTSUBMSK);

	return 0;
}

void platform_irq_clean_pending(void)
{
	iowrite32(ioread32(irq_base + SUBSRCPND),irq_base + SUBSRCPND);
	iowrite32(ioread32(irq_base + SRCPND),irq_base + SRCPND);
	iowrite32(ioread32(irq_base + INTPND),irq_base + INTPND);
}

/*
 *code for s3c2440 uart
 */
int platform_uart0_init(void)
{
	u32 val = ( (int)(PCLK/16./115200+0.5) -1);

	iowrite32(0x1,uart_base + UFCON0);
	iowrite32(0x0,uart_base + UMCON0);
	iowrite32(0x03,uart_base + ULCON0);
	iowrite32(0x345,uart_base + UCON0);
	iowrite32(val,uart_base + UBRDIV1);
	iowrite32(val,uart_base + UBRDIV0);

	return 0;
}

void platform_uart0_send_byte(u16 ch)
{
#if 1
	if(ch == '\n'){
//		while(!(ioread32(uart_base + UTRSTAT0) & 0x02));
		
		iowrite8('\r',uart_base + UTXH0);
	}

//	while(!(ioread32(uart_base + UTRSTAT0) & 0x02));
	iowrite8(ch,uart_base + UTXH0);
#else
	volatile unsigned long *paddr = (unsigned long *)0x50000020;

	*paddr = ch;
#endif
}

/*
 *at boot stage we need to init early console to 
 *support printk.
 */
int console_early_init(void)
{
	/*
	 *because at boot stage mmu has not enable
	 *we can use the plat mode address to write
	 *value to the address.
	 */
	clk_base = (void *)CLK_BASE;
	uart_base = (void *)UART_BASE;
	platform_clk_init();
	platform_uart0_init();

	return 0;
}

int console_late_init(void)
{
	/*
	 *in this function we can init other module if needed
	 *but first we need get clk_base and uart_base again
	 *since the mmu tlb has changed,and the io memory mapped
	 *in boot stage can not be used any longer;in this function
	 *we can not call printk.and clk_base and uart_base must get
	 *correct value.
	 */
	clk_base = request_io_mem(CLK_BASE);
	uart_base = request_io_mem(UART_BASE);

	return 0;
}

int uart_puts(char *buf)
{
	while(*buf){
		platform_uart0_send_byte(*buf++);
	}
	
	platform_uart0_send_byte(0x0d);
	platform_uart0_send_byte(0x0a);

	return strlen(buf);
}
