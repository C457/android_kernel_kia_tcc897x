/*
 * linux/drivers/serial/tcc_serial.c
 *
 * Based on: drivers/serial/s3c2410.c and driver/serial/bfin_5xx.c
 * Author:  <linux@telechips.com>
 * Created: June 10, 2008
 * Description: Driver for onboard UARTs on the Telechips TCC Series
 *
 * Copyright (C) 2008-2016 Telechips
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA V
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/serial_reg.h>
#include <linux/gpio.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <mach/gpio.h>
#include <mach/hardware.h>
#include <mach/bsp.h>
#include <plat/serial.h>
#include <mach/tca_serial.h>
#include <mach/iomap.h>
#ifdef CONFIG_DAUDIO
#include <mach/daudio_debug.h>
#endif

#include <linux/dma-mapping.h>
#include <linux/cpufreq.h>

#define DRV_NAME "tcc-uart"

#define TIME_STEP                   1//(1*HZ)

//#define PRINT_TTY_BUF

/*#if defined(PRINT_TTY_BUF)
#define PRINT_TX_CH (2)
#define PRINT_RX_CH PRINT_TX_CH
#endif
*/

#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/printk.h>
int debug_port = (int)NULL;

void *kerneltimer_timeover(void *arg );

#define TCC_BT_UART 1
#define TCC_GPS_UART 3

#define TIMEOUT         jiffies_to_msecs(1000)

#define UART_IER_ELSI   UART_IER_RLSI

#define UART_FLOW_CONTROL 1
#define UART_NOT_FLOW_CONTROL 0


/* UARTx IIR Masks */
#define IIR_IDD         0x0E    /* Interrupt ID, Source, Reset */
#define IIR_IPF         Hw0     /* Interrupt Flag   */

// Interrupt Ident. Register IID value
#define IIR_THRE 		0x01 	/* Transmitter holding register empty */
#define IIR_RDA 		0x02 	/* Received data available */
#define IIR_RLS 		0x03 	/* Receiver line status */
#define IIR_CTI 		0x06 	/* Character timeout indication */

/* UARTx_FCR Masks                                  */
#define FCR_FE          Hw0     /* FIFO Enable      */
#define FCR_RXFR        Hw1     /* RX FIFO Reset    */
#define FCR_TXFR        Hw2     /* TX FIFO Reset    */
#define FCR_DRTE        Hw3     /* DMA Mode Select  */

/* UARTx_LCR Masks                                                  */
#define LCR_WLS(x)      (((x)-5) & 0x03)    /* Word Length Select   */
#define LCR_STB         Hw2                 /* Stop Bits            */
#define LCR_PEN         Hw3                 /* Parity Enable        */
#define LCR_EPS         Hw4                 /* Even Parity Select   */
#define LCR_SP          Hw5                 /* Stick Parity         */
#define LCR_SB          Hw6                 /* Set Break            */
#define LCR_DLAB        Hw7                 /* Divisor Latch Access */

/* UARTx_MCR Mask                                                   */
#define MCR_RTS         Hw1     /* Request To Sent                  */
#define MCR_LOOP        Hw4     /* Loopback Mode Enable             */
#define MCR_AFE         Hw5     /* Auto Flow Control Enable         */
#define MCR_RS          Hw6     /* RTS Deassert Condition Control   */

/* UARTx_LSR Masks                                          */
#define LSR_DR          Hw0     /* Data Ready               */
#define LSR_OE          Hw1     /* Overrun Error            */
#define LSR_PE          Hw2     /* Parity Error             */
#define LSR_FE          Hw3     /* Framing Error            */
#define LSR_BI          Hw4     /* Break Interrupt          */
#define LSR_THRE        Hw5     /* THR Empty                */
#define LSR_TEMT        Hw6     /* TSR and UART_THR Empty   */

/* UARTx_IER Masks */
#define IER_ERXI        Hw0
#define IER_ETXI        Hw1
#define IER_ELSI        Hw2

/* UARTx UCR Masks */
#define UCR_TxDE        Hw0
#define UCR_RxDE        Hw1
#define UCR_TWA         Hw2
#define UCR_RWA         Hw3

#define OFFSET_THR    0x00	/* Transmit Holding register            */
#define OFFSET_RBR    0x00	/* Receive Buffer register              */
#define OFFSET_DLL    0x00	/* Divisor Latch (Low-Byte)             */
#define OFFSET_IER    0x04	/* Interrupt Enable Register            */
#define OFFSET_DLM    0x04	/* Divisor Latch (High-Byte)            */
#define OFFSET_IIR    0x08	/* Interrupt Identification Register    */
#define OFFSET_FCR    0x08	/* FIFO Control Register                */
#define OFFSET_LCR    0x0C	/* Line Control Register                */
#define OFFSET_MCR    0x10	/* Modem Control Register               */
#define OFFSET_LSR    0x14	/* Line Status Register                 */
#define OFFSET_MSR    0x18	/* Modem Status Register                */
#define OFFSET_SCR    0x1C	/* SCR Scratch Register                 */
#define OFFSET_AFT    0x20	/* AFC Trigger Level Register           */
#define OFFSET_UCR    0x24	/* UART Control Register                */

#define portaddr(port, reg) ((port)->membase + (reg))

#define rd_regb(port, reg) (__raw_readb(portaddr(port, reg)))
#define rd_regl(port, reg) (__raw_readl(portaddr(port, reg)))

#define wr_regl(port, reg, val) \
	do { __raw_writel(val, portaddr(port, reg)); } while(0)
#define wr_regb(port, reg, val) \
	do { __raw_writeb(val, portaddr(port, reg)); } while(0)


/* configuration defines */
#if 0
#define dbg(fmt,arg...) printk("==== tcc uart: "fmt, ##arg);
#define dbg_on 1
#else /* no debug */
#define dbg(x...) do {} while(0)
#define dbg_on 0
#endif
/* uart pm debug */
#if 0
#define pm_dbg(fmt,arg...) printk("==== tcc uart pm: "fmt, ##arg);
#else /* no debug */
#define pm_dbg(x...) do {} while(0)
#endif

/* UART name and device definitions */

#if defined(CONFIG_DAUDIO_TTY_NAME_LEGACY)
#define TCC_SERIAL_NAME	    "ttyTCC"
#else
#define TCC_SERIAL_NAME	    "ttyS"
#endif
#define TCC_SERIAL_MAJOR	204
#define TCC_SERIAL_MINOR	64

/* conversion functions */
#define tcc_dev_to_port(__dev) (struct uart_port *)dev_get_drvdata(__dev)
#define tx_enabled(port)	((port)->unused[0])
#define rx_enabled(port)	((port)->unused[1])
#define port_used(port)		((port)->unused1)

#if defined(CONFIG_GPS)
extern int gps_k_flag;
#endif

#if defined(CONFIG_BT)
static int bt_used = 0;
#endif
EXPORT_SYMBOL(bt_used);

static DEFINE_SPINLOCK(ext_irq_lock);

static struct device tcc_uart_dev[8];
static unsigned long uartPortCFG0, uartPortCFG1;

void kerneltimer_registertimer(struct timer_list* ptimer,
		unsigned long timeover,struct uart_port *port)
{
	init_timer( ptimer );
	ptimer->expires  = get_jiffies_64() + timeover;
	ptimer->data     = (unsigned long)port;
	ptimer->function = (void *)kerneltimer_timeover;
	add_timer( ptimer);
}

void my_uart_rx_process(unsigned long arg)
{
	volatile unsigned long current_tail,current_head, i, rts_state;
	struct uart_port *port;
	struct tcc_uart_port *tcc_port;
	unsigned int ch = 0;
	unsigned uerstat = 0, flag;
	unsigned char *buffer;
	unsigned long cur_addr;
	unsigned long start_addr;

	port = (struct uart_port *)arg;
	tcc_port = (struct tcc_uart_port *)port;

	cur_addr = tca_dma_dmacurrentaddress(tcc_port->rx_dma_buffer.dma_ch, (unsigned long *)tcc_port->rx_dma_buffer.dma_core);
	start_addr = tca_dma_dmadeststartaddress(tcc_port->rx_dma_buffer.dma_ch, (unsigned long *)tcc_port->rx_dma_buffer.dma_core);
	buffer = tcc_port->rx_dma_buffer.addr;
	current_tail = (tcc_port->rx_dma_tail - start_addr) % SERIAL_RX_DMA_BUF_SIZE;
	current_head = (cur_addr - start_addr) % SERIAL_RX_DMA_BUF_SIZE;

	if (current_head != current_tail) {
		if (current_head > current_tail)
			rts_state = current_head - current_tail;
		else
			rts_state = SERIAL_RX_DMA_BUF_SIZE - current_tail + current_head;

		if (rts_state > (SERIAL_RX_DMA_BUF_SIZE/2)) {
			//printk("RTS HIGH[0x%x : 0x%x]\n", cur_addr, current_tail);
			wr_regl(port, OFFSET_MCR, rd_regl(port, OFFSET_MCR) & ~Hw1);
		}

		spin_lock(&tcc_port->rx_lock);

		flag = TTY_NORMAL;
		port->icount.rx++;

		uerstat = rd_regl(port, OFFSET_LSR);

		if (uerstat & UART_LSR_BI) {
			port->icount.brk++;
			goto out;
		}

		if (uerstat & UART_LSR_FE)
			port->icount.frame++;
		if (uerstat & UART_LSR_OE)
			port->icount.overrun++;

		uerstat &= port->read_status_mask;

		if (uerstat & UART_LSR_BI)
			flag = TTY_BREAK;
		else if (uerstat & UART_LSR_PE)
			flag = TTY_PARITY;
		else if (uerstat & ( UART_LSR_FE | UART_LSR_OE))
			flag = TTY_FRAME;

		if (current_head < current_tail)
			current_head += (SERIAL_RX_DMA_BUF_SIZE);

		for (i=current_tail;i<current_head;i++) {
			if (tcc_port->timer_state == 0)
				break;
			if (i >= SERIAL_RX_DMA_BUF_SIZE)
				ch = buffer[i-SERIAL_RX_DMA_BUF_SIZE];
			else
				ch = buffer[i];

			if (uart_handle_sysrq_char(port, ch))
				goto out;
			/* put the received char into UART buffer */
			uart_insert_char(port, uerstat, UART_LSR_OE, ch, flag);
			tty_flip_buffer_push(&port->state->port);
			tcc_port->rx_dma_tail++;
#if defined(PRINT_TTY_BUF)
			if (port->line == 2){
					printk(KERN_INFO "[0x%x],[%c]",ch, ch);
			}
#endif
        }

#if defined(PRINT_TTY_BUF)
		if (port->line == 2)
			printk(KERN_INFO "%s end\n", __func__);
#endif

		if (rts_state > SERIAL_RX_DMA_BUF_SIZE/2) {
			printk("RTS LOW\n");
			wr_regl(port, OFFSET_MCR, rd_regl(port, OFFSET_MCR) | Hw1);
		}
out:
		spin_unlock(&tcc_port->rx_lock);

		dbg("[%p : 0x%x : 0x%x : 0x%x]\n", buffer, cur_addr, current_head, current_tail);
#if defined(CONFIG_UART_DEBUG_BUS_SYSFS)
    port->icount.rx_count += current_head - current_tail;
#endif
	}
}

void *kerneltimer_timeover(void *arg )
{
	struct tcc_uart_port *tcc_port = arg;

	unsigned long cur_addr = tca_dma_dmacurrentaddress(tcc_port->rx_dma_buffer.dma_ch, (unsigned long *)tcc_port->rx_dma_buffer.dma_core);

	if (tcc_port->dma_start_value == cur_addr) {
		if (tcc_port->dma_start_value == 0x20000000) my_uart_rx_process((unsigned long)arg);
	} else {
		if (tcc_port->dma_start_value != 0x20000000) {
			tcc_port->rx_dma_tail = (unsigned long) tcc_port->rx_dma_buffer.dma_addr;
			tcc_port->dma_start_value = 0x20000000;
		}
		my_uart_rx_process((unsigned long)arg);
	}

	if (tcc_port->timer_state == 1)
		kerneltimer_registertimer( tcc_port->dma_timer, TIME_STEP,arg );

	return 0;
}

int kerneltimer_init(struct uart_port *port)
{
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);
	tcc_port->dma_timer= kmalloc( sizeof( struct timer_list ), GFP_KERNEL );
	if(tcc_port->dma_timer == NULL )
		return -ENOMEM;
	memset( tcc_port->dma_timer, 0, sizeof( struct timer_list) );
	kerneltimer_registertimer( tcc_port->dma_timer,TIME_STEP,port );

	return 0;
}


/* translate a port to the device name */
static inline const char *tcc_serial_portname(struct uart_port *port)
{
	return to_platform_device(port->dev)->name;
}


void tcc_serial_uart_putchar(struct uart_port *port, int ch)
{
	while (!(rd_regl(port, OFFSET_LSR) & LSR_THRE))
		cpu_relax();

	wr_regb(port, OFFSET_THR, ch);
}

static void tcc_serial_rx(struct uart_port *port, unsigned int lsr) 
{
	unsigned int flag;
	unsigned int ch = 0;

	ch = rd_regb(port, OFFSET_RBR);

	flag = TTY_NORMAL;
	port->icount.rx++;
	if (lsr & UART_LSR_BI) {
		port->icount.brk++;
		return;
	}

	if (lsr & UART_LSR_FE)
		port->icount.frame++;

	if (lsr & UART_LSR_OE){
		port->icount.overrun++;
	}

	lsr &= port->read_status_mask;

	if (lsr & UART_LSR_BI)
		flag = TTY_BREAK;
	else if (lsr & UART_LSR_PE)
		flag = TTY_PARITY;
	else if (lsr & ( UART_LSR_FE | UART_LSR_OE))
		flag = TTY_FRAME;

	if (uart_handle_sysrq_char(port, ch))
		return ;
	/* put the received char into UART buffer */
	uart_insert_char(port, lsr, UART_LSR_OE, ch, flag);

}

//static void tcc_serial_dma_tx(struct uart_port *port)
//{
	//struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);
//}

static void tcc_serial_tx(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);

	if(tcc_port->tx_dma_use)
		tcc_port->tx_done = 0;

	tcc_port->fifosize = uart_circ_chars_pending(xmit);
	if(tcc_port->fifosize > FIFOSIZE) {
		tcc_port->fifosize = FIFOSIZE;
		if((xmit->tail + tcc_port->fifosize) > UART_XMIT_SIZE ) {
			tcc_port->fifosize = UART_XMIT_SIZE - xmit->tail;
		}
	}

	if (tcc_port->tx_dma_use && (uart_circ_empty(xmit) || uart_tx_stopped(port))) {
		wr_regl(port, OFFSET_IER, (rd_regl(port, OFFSET_IER) & ~IER_ETXI));
		tcc_port->tx_done = 1;	//// 100510

		if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS) {
			uart_write_wakeup(port);
		}
		return;
	}

	if (port->x_char) {
		if(tcc_port->fifosize > 1)
			wr_regl(port, OFFSET_IER, (rd_regl(port, OFFSET_IER) & ~IER_ETXI));

		tcc_serial_uart_putchar(port, port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		tcc_port->fifosize--;
	}

#if defined(PRINT_TTY_BUF)
	if(tcc_port->fifosize > 0){
		if (port->line == PRINT_TX_CH){
		printk(KERN_INFO "%s start [%s] \n", __func__, tcc_port->tx_dma_use?"DMA":"No DMA");
		int i;
		for (i = 0; i < tcc_port->fifosize; i++)
				printk(KERN_INFO "[0x%x],[%c]", (xmit->buf+xmit->tail)[i], (xmit->buf+xmit->tail)[i]);
		printk(KERN_INFO "%s end\n", __func__);
		}
	}
#endif

	if(tcc_port->tx_dma_use)
	{
		if(tcc_port->fifosize > 0)
		{
			wr_regl(port, OFFSET_UCR, rd_regl(port, OFFSET_UCR) & ~Hw0);
			memcpy(tcc_port->tx_dma_buffer.addr, (xmit->buf+xmit->tail), tcc_port->fifosize );

			//DMA Setting
			tca_dma_setconfig(tcc_port->tx_dma_buffer.dma_ch,
					(void *)tcc_port->tx_dma_buffer.dma_addr,
					0x1, /* src Param */
					portaddr(port, 0x0) - IO_OFFSET,
					0x0, /* dest Param */
					HwCHCTRL_SYNC_EN        |
					HwCHCTRL_TYPE_SL        |
					HwCHCTRL_BSIZE_1        |
					HwCHCTRL_WSIZE_8        |
					HwCHCTRL_IEN_ON         |
					HwCHCTRL_FLAG           ,
					tcc_port->fifosize,
					tcc_port->tx_dma_buffer.dma_port,
					tcc_port->tx_dma_buffer.dma_mode, /* mode */
					(unsigned long *)tcc_port->tx_dma_buffer.dma_core);

			wr_regl(port, OFFSET_UCR, rd_regl(port, OFFSET_UCR) |Hw0);
		}
	}
	else {
		if (uart_circ_empty(xmit)|| uart_tx_stopped(port))
			wr_regl(port, OFFSET_IER, (rd_regl(port, OFFSET_IER) & ~IER_ETXI));

		while (tcc_port->fifosize > 0) {
			if(!(rd_regl(port, OFFSET_LSR) & LSR_THRE))
				break;
			wr_regb(port, OFFSET_THR, xmit->buf[xmit->tail]);
			xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
			port->icount.tx++;
			tcc_port->fifosize--;
		} //// while

		if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
			uart_write_wakeup(port);
	}
}


static void tcc_serial_stop_tx(struct uart_port *port)
{
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);

	dbg("%s\n", __func__);

	tcc_port->tx_done = 1;

	while(!((rd_regl(port, OFFSET_LSR)) & LSR_TEMT))
		continue;

	if(tx_enabled(port))
		tx_enabled(port) = 0;
}

static void tcc_serial_start_tx(struct uart_port *port)
{
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);

	if(!tx_enabled(port))
		tx_enabled(port) = 1;

	if (tcc_port->tx_dma_use) {
		while (!(rd_regl(port, OFFSET_LSR) & LSR_THRE))
			cpu_relax();

		if(tcc_port->tx_done) {
			tcc_serial_tx(port);
			wr_regl(port, OFFSET_IER, rd_regl(port, OFFSET_IER) | IER_ETXI);
		}
	}
	else {
		wr_regl(port, OFFSET_IER, rd_regl(port, OFFSET_IER) | IER_ETXI);
	}
}

static void tcc_serial_stop_rx(struct uart_port *port)
{
	u32 ier;

	dbg("%s  line[%d] irq[%d]\n", __func__, port->line, port->irq);

	if (rx_enabled(port)) {
		ier = rd_regl(port, OFFSET_IER);
		wr_regl(port, OFFSET_IER, (ier & ~IER_ERXI));

		mdelay(10);
		rx_enabled(port) = 0;
	}
}

static void tcc_serial_enable_ms(struct uart_port *port)
{
	dbg("%s\n", __func__);
}

static irqreturn_t tcc_serial_interrupt_dma(int irq, void *id)
{
	struct uart_port *port = id;
	struct tcc_uart_port *tcc_port = id;
	struct circ_buf *xmit = &port->state->xmit;

	spin_lock(&port->lock);
	if (tcc_port->tx_dma_use) {
		/* XXX */
		xmit->tail = (xmit->tail + tcc_port->fifosize) & (UART_XMIT_SIZE - 1);
		port->icount.tx+= tcc_port->fifosize;
		tcc_port->fifosize = 0;

		tca_dma_clrien(tcc_port->tx_dma_buffer.dma_ch, (unsigned long *)tcc_port->tx_dma_buffer.dma_core);
		tca_serial_dmaclrinterrupt(tcc_port->tx_dma_buffer.dma_ch, (unsigned long *)tcc_port->tx_dma_buffer.dma_core);

		tcc_serial_tx(port);

		if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS){
			uart_write_wakeup(port);
		}
	}
	spin_unlock(&port->lock);
	return IRQ_HANDLED;
}

static irqreturn_t tcc_serial_interrupt(int irq, void *id)
{
	void __iomem *ists_reg = (void __iomem *)io_p2v(TCC_PA_UARTPORTCFG + 0x0C /* ISTS */);
	struct uart_port *port = id;
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);
	unsigned int lsr_data = 0;
	unsigned int iir_data = 0;
	unsigned long flags;

	if ((__raw_readl(ists_reg) & (1<<(port->line))) == 0)
		return IRQ_NONE;

	spin_lock_irqsave(&port->lock,flags);

	iir_data = rd_regl(port, OFFSET_IIR);
	iir_data = (iir_data & 0x0E) >> 1;

	lsr_data = rd_regl(port, OFFSET_LSR);

	if (iir_data == IIR_RDA || iir_data == IIR_CTI) {

		if(!tcc_port->rx_dma_use)
		{
			tcc_serial_rx(port, lsr_data);
			tty_flip_buffer_push(&port->state->port);
		}
	}
	else if (iir_data == IIR_THRE) {
		if (tcc_port->tx_dma_use) {
			while (!(rd_regl(port, OFFSET_LSR) & LSR_THRE))
				cpu_relax();
		}
		else {
			tcc_serial_tx(port);
		}
	}

	spin_unlock_irqrestore(&port->lock,flags);	

	return IRQ_HANDLED;
}

static unsigned int tcc_serial_tx_empty(struct uart_port *port)
{
	unsigned short lsr;

	lsr = rd_regl(port, OFFSET_LSR);
	if (lsr & LSR_TEMT)
		return TIOCSER_TEMT;
	else
		return 0;

}

static unsigned int tcc_serial_get_mctrl(struct uart_port *port)
{
	dbg("%s\n", __func__);
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void tcc_serial_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	dbg("%s\n", __func__);
	/* todo - possibly remove AFC and do manual CTS */
}

static void tcc_serial_break_ctl(struct uart_port *port, int break_state)
{
	dbg("%s\n", __func__);
}

static void tcc_serial_shutdown(struct uart_port *port)
{
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);

	// clock control for BT
	if(tcc_port->bt_use) {
#if defined(CONFIG_BT)
		bt_used = 0;
#endif
	}

	dbg("%s\n", __func__);
	if(tcc_port->rx_dma_use)
	{
		tcc_port->timer_state=0;
		mdelay(1);
		del_timer(tcc_port->dma_timer);
		kfree(tcc_port->dma_timer);
	}

	wr_regl(port, OFFSET_IER, 0x0);
	free_irq(port->irq, port);

	if(tcc_port->tx_dma_use)
		free_irq(tcc_port->tx_dma_buffer.dma_intr, port);

	//    tcc_serial_input_buffer_set(tcc_port->port.line, 0);

	port_used(port) = 0;	// for suspend/resume

	dbg("%s   line[%d] out...\n", __func__, port->line);
}


static int tcc_serial_rx_dma_probe(struct uart_port *port)
{
	struct tcc_uart_port *tp = container_of(port, struct tcc_uart_port, port);
	unsigned long cur_addr;
		
	cur_addr = tca_dma_dmacurrentaddress(tp->rx_dma_buffer.dma_ch, (unsigned long *)tp->rx_dma_buffer.dma_core);

	dbg("rx_dma_tail : %x\n",tcc_port->rx_dma_tail);

	wr_regl(port, OFFSET_MCR, rd_regl(port, OFFSET_MCR) | MCR_RTS);
	wr_regl(port, OFFSET_AFT, 0x00000021);

	// Set Source Address & Source Parameter (mask + increment)
	wr_regl(port, OFFSET_UCR, rd_regl(port, OFFSET_UCR) & ~UCR_RxDE);

	tca_dma_setconfig(tp->rx_dma_buffer.dma_ch,
			portaddr(port, 0x0) - IO_OFFSET,
			0x0, /* src Param */
			(void *)tp->rx_dma_buffer.dma_addr,
			0x1, /* dest Param */
			//HwCHCTRL_CONT_C		|
			HwCHCTRL_SYNC_EN		|
			//HwCHCTRL_BST_BURST	|
			HwCHCTRL_HRD_WR		|
			HwCHCTRL_TYPE_SL		|
			HwCHCTRL_BSIZE_1		|
			HwCHCTRL_WSIZE_8		|
			//HwCHCTRL_IEN_ON		|
			HwCHCTRL_REP_EN		|
			HwCHCTRL_FLAG			,
			tp->rx_dma_buffer.buf_size,
			tp->rx_dma_buffer.dma_port,
			tp->rx_dma_buffer.dma_mode, /* mode */
			(unsigned long *)tp->rx_dma_buffer.dma_core);

	wr_regl(port, OFFSET_UCR, rd_regl(port, OFFSET_UCR) | UCR_RxDE);

	tp->dma_start_value = cur_addr;
	if (cur_addr != 0x20000000)
		tp->rx_dma_tail = cur_addr;
	else
		tp->rx_dma_tail = (unsigned long) tp->rx_dma_buffer.dma_addr;
	tp->timer_state=1;

	kerneltimer_init(port);

	return 0;
}

/* 
 * while application opening the console device, this function will invoked
 * This function will initialize the interrupt handling
 */
static int tcc_serial_startup(struct uart_port *port)
{
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);
	int retval=0;
	unsigned int lcr;
	unsigned long flags;

	dbg("%s() line[%d] in...\n", __func__, port->line);

	if(tcc_port->bt_use){
#if defined(CONFIG_BT)
		bt_used = 1;
#endif
	}

	tx_enabled(port) = 1;
	rx_enabled(port) = 1;
	port_used(port) = 1;	// for suspend/resume

	// clock control for BT

#ifndef CONFIG_OF
#if defined(CONFIG_TCC_BT_DEV)
	if(tcc_port->bt_suspend != 1){
		if(port->line == 0)
			tca_serial_portinit(UART_NOT_FLOW_CONTROL , port->line);
		else
			tca_serial_portinit(UART_FLOW_CONTROL, port->line);
	}
#else
	if(port->line == 0)
		tca_serial_portinit(UART_NOT_FLOW_CONTROL , port->line);
	else
		tca_serial_portinit(UART_FLOW_CONTROL , port->line);
#endif
#endif

	/* clear interrupt */
	wr_regl(port, OFFSET_IER, 0x0);

	lcr = rd_regl(port, OFFSET_LCR);
	wr_regl(port, OFFSET_LCR, (lcr | LCR_DLAB));

	wr_regl(port, OFFSET_FCR, (FCR_TXFR|FCR_RXFR|FCR_FE));    /* FIFO Enable, Rx/Tx FIFO reset */
	tcc_port->fifosize = FIFOSIZE;
	tcc_port->reg.bFCR = 0x07;          /* for resume restore */

	if(tcc_port->tx_dma_use)
		tcc_port->tx_done  = 1;

	wr_regl(port, OFFSET_LCR, (lcr & (~LCR_DLAB)));

	rd_regl(port, OFFSET_IIR);

	retval = request_irq(port->irq, tcc_serial_interrupt, IRQF_SHARED, tcc_port->name , port);
	dbg("request serial irq:%d,retval:%d\n", port->irq, retval);

	if(tcc_port->tx_dma_use) {
		retval = request_irq(tcc_port->tx_dma_buffer.dma_intr, tcc_serial_interrupt_dma, IRQF_SHARED, "uart1_dma" , port);
		dbg("request serial dma irq:%d,retval:%d\n", tcc_port->tx_dma_buffer.dma_intr, retval);
	}

	/* Rx DMA */
	if(tcc_port->rx_dma_use) 
		tcc_serial_rx_dma_probe(port);

	spin_lock_irqsave(&port->lock, flags);
	wr_regl(port, OFFSET_IER, IER_ERXI);
	spin_unlock_irqrestore(&port->lock, flags);

	if(tcc_port->rx_dma_use) {
		dbg("line %d IER tx Only\n", port->line);
		wr_regl(port, OFFSET_IER, 0);
	}

	tcc_port->opened = 1;

	dbg(" %s() out...\n", __func__);
	return retval;
}


/* power power management control */
static void tcc_serial_pm(struct uart_port *port, unsigned int level, unsigned int old)
{
}

static void tcc_serial_set_baud(struct tcc_uart_port *tcc_port, unsigned int baud)
{
	/* Set UARTx peripheral clock */
	switch(baud) {
		case 921600:
			if (tcc_port->fclk)
				clk_set_rate(tcc_port->fclk, 103219200);		// 103.219MHz
			break;
		case 2500000:
			if (tcc_port->fclk)
				clk_set_rate(tcc_port->fclk, 120*1000*1000);	// 120MHz
			break;
		default:
			if (tcc_port->fclk)
				clk_set_rate(tcc_port->fclk, 96*1000*1000);	// 96MHz
			break;
	}
}

#if defined(CONFIG_HIBERNATION) && defined(CONFIG_SERIAL_TCC_CONSOLE)
extern unsigned int do_hibernate_boot;
#endif

static void tcc_serial_set_termios(struct uart_port *port, struct ktermios *termios,
		struct ktermios *old)
{
	unsigned long flags;
	unsigned int baud, quot;
	unsigned int ulcon;
	unsigned int umcon, lsr;
	int uart_clk = 0;
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);

	/*
	 * We don't support modem control lines.
	 */
	termios->c_cflag &= ~(HUPCL | CMSPAR);
	termios->c_cflag |= CLOCAL;

	/* Ask the core to calculate the baud rate. */
	baud = uart_get_baud_rate(port, termios, old, 0, 3500000);

#if defined(CONFIG_HIBERNATION) && defined(CONFIG_SERIAL_TCC_CONSOLE)
	if(unlikely(do_hibernate_boot && tcc_port->port.line == CONSOLE_PORT)) {
		tcc_serial_set_baud(tcc_port, baud);
		port->uartclk = baud;
	}
#endif

	if(tcc_port->port.line != CONSOLE_PORT) {
		if(baud != port->uartclk)
			tcc_serial_set_baud(tcc_port, baud);
		port->uartclk = baud;
	}

	/*
	 * Ask the core to calculate the divisor for us.
	 */

	if (tcc_port->fclk)
		uart_clk = clk_get_rate(tcc_port->fclk);
	quot = (uart_clk + ((16*baud)>>1))/(16*baud);

	/*
	 * set byte size
	 */
	switch (termios->c_cflag & CSIZE) {
		case CS5:
			dbg("config: 5bits/char  cflag[0x%x]\n", termios->c_cflag );
			ulcon = 0;
			break;
		case CS6:
			dbg("config: 6bits/char  cflag[0x%x]\n", termios->c_cflag );
			ulcon = 1;
			break;
		case CS7:
			dbg("config: 7bits/char  cflag[0x%x]\n", termios->c_cflag );
			ulcon = 2;
			break;
		case CS8:
		default:
			dbg("config: 8bits/char  cflag[0x%x]\n", termios->c_cflag );
			ulcon = 3;
			break;
	}

	/* preserve original lcon IR settings */
	if (termios->c_cflag & CSTOPB)
		ulcon |= LCR_STB;   /* HwUART_LCR_STB_ONE */

	umcon = (termios->c_cflag & CRTSCTS) ? (MCR_AFE|MCR_RTS) : MCR_RTS; /* HwUART_MCR_RTS_ON */

	if (termios->c_cflag & PARENB) {
		if (termios->c_cflag & PARODD)
			ulcon |= (LCR_PEN);
		else
			ulcon |= (LCR_EPS|LCR_PEN);
	} else {
		ulcon &= ~(LCR_EPS|LCR_PEN);
	}

	printk("[UART%02d] setting ulcon: %08x, umcon: %08x, brddiv to %d, baud %d, uart_clk %d\n", 
			port->line, ulcon, umcon, quot, baud, uart_clk);
	spin_lock_irqsave(&port->lock, flags);

	do {
		lsr = rd_regl(port, OFFSET_LSR);
	} while (!(lsr & LSR_TEMT));

	//	tcc_serial_input_buffer_set(tcc_port->port.line, 1);

	wr_regl(port, OFFSET_MCR, umcon);
	wr_regl(port, OFFSET_LCR, (ulcon | LCR_DLAB));
	if(quot > 0xFF) {
		wr_regl(port, OFFSET_DLL, quot & 0x00FF);
		wr_regl(port, OFFSET_DLM, quot >> 8);
	} else if (quot > 0) {
		wr_regl(port, OFFSET_DLL, quot);
		wr_regl(port, OFFSET_DLM, 0x0);
	}
	wr_regl(port, OFFSET_LCR, (ulcon & (~LCR_DLAB)));

	/*
	 * Update the per-port timeout.
	 */
	uart_update_timeout(port, termios->c_cflag, baud);

	/*
	 * Which character status flags are we interested in?
	 */
	port->read_status_mask = 0;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= 0;

	/*
	 * Which character status flags should we ignore?
	 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= 0;
	if (termios->c_iflag & IGNBRK && termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= 0;

	/*
	 * Ignore all characters if CREAD is not set.
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= 0;

	spin_unlock_irqrestore(&port->lock, flags);
}

static const char *tcc_serial_type(struct uart_port *port)
{
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);

	return tcc_port->name;
}

#define MAP_SIZE (0x100)

static void tcc_serial_release_port(struct uart_port *port)
{
	/* TODO */
	//release_mem_region(port->mapbase, MAP_SIZE);
}

static int tcc_serial_request_port(struct uart_port *port)
{
	return 0;
	/*
	   return request_mem_region(port->mapbase, MAP_SIZE, "tcc7901") ? 0 : -EBUSY;
	   */
}

static void tcc_serial_config_port(struct uart_port *port, int flags)
{
#if !defined(CONFIG_TCC_CODESONAR_BLOCKED)
#else
	if (flags & UART_CONFIG_TYPE && tcc_serial_request_port(port) == 0)
#endif
		port->type = PORT_TCC;
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int tcc_serial_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);

	if(tcc_port->info == NULL)
		return -EINVAL;

	if (ser->type != PORT_UNKNOWN && ser->type != tcc_port->info->type)
		return -EINVAL;
	return 0;
}

static void tcc_wake_peer(struct uart_port *port)
{
	/*
	   struct tcc_uart_port *tcc_port = (struct tcc_uart_port *)port;

	   if (tcc_port->wake_peer)
	   tcc_port->wake_peer(tcc_port);
	   */
}

int tcc_uart_enable(int port_num )
{
	int ret;

	ret = pm_runtime_get_sync(&tcc_uart_dev[port_num]);

	pm_dbg("%s ret : %d\n", __func__, ret);

	return 0;
}

int tcc_uart_disable(int port_num)
{
	int ret;

	ret = pm_runtime_put_sync_suspend(&tcc_uart_dev[port_num]);

	pm_dbg("%s ret : %d\n", __func__, ret);

	return 0;
}
EXPORT_SYMBOL(tcc_uart_disable);
EXPORT_SYMBOL(tcc_uart_enable);

#ifdef CONFIG_SERIAL_TCC_CONSOLE
static struct console tcc_serial_console;
#define TCC_SERIAL_CONSOLE      &tcc_serial_console
#else
#define TCC_SERIAL_CONSOLE      NULL
#endif

static struct uart_ops tcc_serial_ops = {
	.pm		= tcc_serial_pm,
	.tx_empty	= tcc_serial_tx_empty,
	.get_mctrl	= tcc_serial_get_mctrl,
	.set_mctrl	= tcc_serial_set_mctrl,
	.stop_tx	= tcc_serial_stop_tx,
	.start_tx	= tcc_serial_start_tx,
	.stop_rx	= tcc_serial_stop_rx,
	.enable_ms	= tcc_serial_enable_ms,
	.break_ctl	= tcc_serial_break_ctl,
	.startup	= tcc_serial_startup,
	.shutdown	= tcc_serial_shutdown,
	.set_termios	= tcc_serial_set_termios,
	.wake_peer	= tcc_wake_peer,
	.type		= tcc_serial_type,
	.release_port	= tcc_serial_release_port,
	.request_port	= tcc_serial_request_port,
	.config_port	= tcc_serial_config_port,
	.verify_port	= tcc_serial_verify_port,
};

static struct uart_driver tcc_uart_drv = {
	.owner          = THIS_MODULE,
	.dev_name       = TCC_SERIAL_NAME,
	.nr             = NR_PORTS,
	.cons           = TCC_SERIAL_CONSOLE,
	.driver_name    = DRV_NAME,
	.major          = TCC_SERIAL_MAJOR,
	.minor          = TCC_SERIAL_MINOR,
};

/*  initialise  serial port information */
/* cpu specific variations on the serial port support */
static struct tcc_uart_info tcc_uart_inf = {
	.name		= "Telechips UART",
	.type		= PORT_TCC,
	.fifosize	= FIFOSIZE,
};

static void *tcc_free_dma_buf(tcc_dma_buf_t *dma_buf)
{
	dbg("%s\n", __func__);
	if (dma_buf) {
		if (dma_buf->dma_addr != 0) {
			dma_free_writecombine(0, dma_buf->buf_size, dma_buf->addr, dma_buf->dma_addr);
		}
		memset(dma_buf, 0, sizeof(tcc_dma_buf_t));
	}
	return NULL;
}

static void *tcc_malloc_dma_buf(tcc_dma_buf_t *dma_buf, int buf_size)
{
	dbg("%s\n", __func__);
	if (dma_buf) {
		tcc_free_dma_buf(dma_buf);
		dma_buf->buf_size = buf_size;
		dma_buf->addr = dma_alloc_writecombine(0, dma_buf->buf_size,
				&dma_buf->dma_addr, GFP_KERNEL);
		dbg("Malloc DMA buffer @0x%X(Phy=0x%X), size:%d\n",
				(unsigned int)dma_buf->addr,
				(unsigned int)dma_buf->dma_addr,
				dma_buf->buf_size);
		return dma_buf->addr;
	}
	return NULL;
}


static int tcc_serial_remove(struct platform_device *dev)
{
	struct uart_port *port = tcc_dev_to_port(&dev->dev);
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);

#if !defined(CONFIG_TCC_CODESONAR_BLOCKED)
	if (!port)
		return 0;
	uart_remove_one_port(&tcc_uart_drv, port);
#else
	if (port)
		uart_remove_one_port(&tcc_uart_drv, port);
#endif

	if(tcc_port->tx_dma_use)
		tcc_free_dma_buf(&(tcc_port->tx_dma_buffer));

	if (tcc_port->rx_dma_use) {
		tcc_free_dma_buf(&(tcc_port->rx_dma_buffer));
		wr_regl(port, OFFSET_UCR, rd_regl(port, OFFSET_UCR) & ~Hw1);

		// DMA channel is disabled.
		tca_dma_clren(tcc_port->rx_dma_buffer.dma_ch, (unsigned long *)tcc_port->rx_dma_buffer.dma_core);
	}

	if(tcc_port->fclk) {
		clk_disable_unprepare(tcc_port->fclk);
		clk_put(tcc_port->fclk);
		tcc_port->fclk = NULL;
	}

	if(tcc_port->hclk) {
		clk_disable_unprepare(tcc_port->hclk);
		clk_put(tcc_port->hclk);
		tcc_port->hclk = NULL;
	}

	return 0;
}

/* UART power management code */
#define SERIAL_IRQ_MAX 6
static int serial_irq_count = 0;
static int g_serial_irq = 0;
struct work_struct serial_irq_work;

static void serial_irq_wq(struct work_struct *work)
{
	unsigned long flag;
	spin_lock_irqsave(&ext_irq_lock, flag);
	disable_irq(g_serial_irq);
	g_serial_irq = 0;
	printk("kernel serial_irq_count max count reached(%d).\n", serial_irq_count);
	spin_unlock_irqrestore(&ext_irq_lock, flag);	
}

static irqreturn_t serial_irq_handler(int irq, void *_dev)
{
	struct device *dev = (struct device*)_dev;
    	unsigned long flag;
	struct pinctrl *p;

	spin_lock_irqsave(&ext_irq_lock, flag);
	serial_irq_count++;
	
	if(!gpio_get_value(debug_port))
	{
		printk("%s : get_gpio_value_debug LOG_OFF !![%d], count=%d\n",__func__, gpio_get_value(debug_port), serial_irq_count);
		irq_set_irq_type(irq, IRQF_TRIGGER_RISING);
		dev->pins->p = pinctrl_get_select(dev, "idle");
		console_loglevel = minimum_console_loglevel;
	}
	else
	{
		irq_set_irq_type(irq, IRQF_TRIGGER_FALLING);
		dev->pins->p = pinctrl_get_select(dev, "active");

		if(dev->pins->p ){
			printk("%s failed to activate idle pinctrl state\n",__func__);
		}else
			printk("%s success activating idle pinctrl state\n",__func__);

		console_loglevel = default_console_loglevel;
		printk("%s : get_gpio_value_debug LOG_ON !![%d], count=%d\n",__func__, gpio_get_value(debug_port), serial_irq_count);
	}

	spin_unlock_irqrestore(&ext_irq_lock, flag);
	if (serial_irq_count >= SERIAL_IRQ_MAX)
	{
		schedule_work(&serial_irq_work);
	}
	
	return IRQ_HANDLED;
}


static int serial_loglevel_set(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int error = 0;
	int id =0,irq;

	if (np)
		id = of_alias_get_id(np, "serial");

	printk("==================%s: id = %d\n", __func__, id);

	if(id == CONSOLE_PORT)
	{
		debug_port = of_get_named_gpio(np, "debug-gpio", 0);

		if(debug_port < 0)
			printk("%s : console debug-port parsing error\n",__func__);

		if(gpio_is_valid(debug_port)){
			gpio_request(debug_port, "debug_port");
			gpio_direction_input(debug_port);
		}
		else
		{
			printk("%s : err to get gpios : ret:%x\n", __func__, debug_port);
			debug_port = -1;
		}

		if(gpio_get_value(debug_port))
		{

			dev->pins->p = pinctrl_get_select(dev, "active");
			console_loglevel = default_console_loglevel;
		}
		else
		{
			dev->pins->p = pinctrl_get_select(dev, "idle");
			console_loglevel = minimum_console_loglevel;
		}
	}
	return error;
}


static int serial_loglevel_interrupt(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int error = 0;
	int id =0,irq;

	if (np)
		id = of_alias_get_id(np, "serial");

	printk("==================%s: id = %d\n", __func__, id);

	if(id == CONSOLE_PORT)
	{
		debug_port = of_get_named_gpio(np, "debug-gpio", 0);

		if(debug_port < 0)
			printk("%s : console debug-port parsing error\n",__func__);

		if(gpio_is_valid(debug_port)){
			gpio_request(debug_port, "debug_port");
			gpio_direction_input(debug_port);
		}
		else
		{
			printk("%s : err to get gpios : ret:%x\n", __func__, debug_port);
			debug_port = -1;
		}

		irq = gpio_to_irq(debug_port);
		g_serial_irq = irq;

		if(gpio_get_value(debug_port))
		{		

			dev->pins->p = pinctrl_get_select(dev, "active");
			console_loglevel = default_console_loglevel;
			error = request_irq(irq,serial_irq_handler, IRQF_TRIGGER_FALLING,"log_leve",dev);

			if (error < 0) {
				printk( "IRQF_TRIGGER_RISING error \n");
				error = request_irq(irq,serial_irq_handler,0,"log_leve",dev);
			}
		}
		else
		{
			dev->pins->p = pinctrl_get_select(dev, "idle");
			console_loglevel = minimum_console_loglevel;
			error = request_irq(irq,serial_irq_handler,IRQF_TRIGGER_RISING ,"log_leve",dev);

			if (error < 0) {
				printk( "IRQF_TRIGGER_FALLING error\n");
				error = request_irq(irq,serial_irq_handler,0,"log_leve",dev);
			}
		}
	}
	
	return error;
}

#ifdef CONFIG_PM
/*-------------------------------------------------
 * TODO: handling DMA_PORT suspend/resume (TCC79X)
 *       DMA stop and start ...
 *-------------------------------------------------*/
static int tcc_serial_suspend(struct device *dev)
{
	struct uart_port *port = tcc_dev_to_port(dev);
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);
	int irq;

	uartPortCFG0 = *(volatile unsigned long *)tcc_p2v(TCC_PA_UARTPORTCFG);
	uartPortCFG1 = *(volatile unsigned long *)tcc_p2v(TCC_PA_UARTPORTCFG + 0x4);

	dbg("%s in...\n", __func__);

	if (port) {

	//port->suspended = 1;

	if(port->line == CONSOLE_PORT){
		printk("%s:console_port[%d]\n",__func__,port->line);
		debug_port = of_get_named_gpio(dev->of_node, "debug-gpio", 0);

		if(debug_port < 0)
		printk("%s : console debug-port parsing error\n",__func__);

		if(gpio_is_valid(debug_port))
		{
			gpio_request(debug_port, "debug_port");
			gpio_direction_input(debug_port);
		}
		else
		{
			printk("%s : err to get gpios : ret:%x\n", __func__, debug_port);
			debug_port = -1;
		}

		irq = gpio_to_irq(debug_port);

		if(!gpio_get_value(debug_port))
		{
			irq_set_irq_type(irq, IRQF_TRIGGER_RISING);
			printk("%s : get_gpio_value[%d]\n",__func__, gpio_get_value(debug_port));
			dev->pins->p = pinctrl_get_select(dev, "idle");

			if(IS_ERR(dev->pins->p))
			{
				printk("%s failed to activate idle pinctrl state\n",__func__);
			}
			else
				printk("%s success activating idle pinctrl state\n",__func__);

			console_loglevel = minimum_console_loglevel;
			// uartPortCFG0 = uartPortCFG0 & 0xFFFFFF00; // disable
		}
		else
		{
			irq_set_irq_type(irq, IRQF_TRIGGER_FALLING);
			printk("%s : get_gpio_value_debug[%d]\n",__func__, gpio_get_value(debug_port));
			dev->pins->p = pinctrl_get_select(dev, "active");

			console_loglevel = default_console_loglevel;
			// uartPortCFG0 = uartPortCFG0 | 0x00000016; // enable
		}		
	}
	
	uart_suspend_port(&tcc_uart_drv, port);
	if(port->line == TCC_GPS_UART){
#if defined(CONFIG_GPS)
	if(gps_k_flag){
		//gpio_direction_output(TCC_GPEXT1(6), 0);
	}
#endif
	}
	
	if(tcc_port->tx_dma_use) {
		//tcc_pca953x_setup(PCA9539_U3_SLAVE_ADDR, BT_ON, OUTPUT, LOW, SET_DIRECTION|SET_VALUE);
	}


#if defined(CONFIG_PM_CONSOLE_NOT_SUSPEND)
	if(!port->cons || (port->cons->index != port->line)){
#endif

	if (tcc_port->opened == 1)
		tca_serial_port_pullup(port->line , 1, uartPortCFG0|((unsigned long long)uartPortCFG1<<32));

	if (tcc_port->fclk)
		clk_disable_unprepare(tcc_port->fclk);
	if (tcc_port->hclk)
		clk_disable_unprepare(tcc_port->hclk);

#if defined(CONFIG_PM_CONSOLE_NOT_SUSPEND)
	}
#endif

	}
	dbg("%s out...\n", __func__);
	return 0;
}

static int tcc_serial_resume(struct device *dev)
{
	struct uart_port *port = tcc_dev_to_port(dev);
	struct tcc_uart_port *tcc_port = container_of(port, struct tcc_uart_port, port);
	int irq;

	printk("%s in...\n", __func__);

	if(port->line == CONSOLE_PORT)
	{
		printk("%s:console_port[%d]\n",__func__,port->line);
		debug_port = of_get_named_gpio(dev->of_node, "debug-gpio", 0);

		if(debug_port < 0)
		printk("%s : console debug-port parsing error\n",__func__);

		if(gpio_is_valid(debug_port)){
			gpio_request(debug_port, "debug_port");
			gpio_direction_input(debug_port);
		}
		else
		{
			printk("%s : err to get gpios : ret:%x\n", __func__, debug_port);
			debug_port = -1;
		}

		irq = gpio_to_irq(debug_port);

		if(!gpio_get_value(debug_port))
		{
			irq_set_irq_type(irq, IRQF_TRIGGER_RISING);
			printk("%s : get_gpio_value[%d]\n",__func__, gpio_get_value(debug_port));
			dev->pins->p = pinctrl_get_select(dev, "idle");

			if(IS_ERR(dev->pins->p)){
				printk("%s failed to activate idle pinctrl state\n",__func__);
			}else
				printk("%s success activating idle pinctrl state\n",__func__);

			console_loglevel = minimum_console_loglevel;
			// uartPortCFG0 = uartPortCFG0 & 0xFFFFFF00; // disable
		}
		else
		{
			irq_set_irq_type(irq, IRQF_TRIGGER_FALLING);

			printk("%s : get_gpio_value_debug[%d]\n",__func__, gpio_get_value(debug_port));
			dev->pins->p = pinctrl_get_select(dev, "active");

			console_loglevel = default_console_loglevel;
			// uartPortCFG0 = uartPortCFG0 | 0x00000016; // enable
		}
	}

#if defined(CONFIG_PM_CONSOLE_NOT_SUSPEND)
	if(!port->cons || (port->cons->index != port->line)){
#endif
	if(tcc_port->opened == 1){
		tca_serial_port_pullup(port->line , 0, uartPortCFG0|((unsigned long long)uartPortCFG1<<32));
	}

	if (tcc_port->hclk)
		clk_prepare_enable(tcc_port->hclk);
	if (tcc_port->fclk)
		clk_prepare_enable(tcc_port->fclk);

#if defined(CONFIG_PM_CONSOLE_NOT_SUSPEND)
	}
#endif

	*(volatile unsigned long *)tcc_p2v(TCC_PA_UARTPORTCFG) = uartPortCFG0;
	*(volatile unsigned long *)tcc_p2v(TCC_PA_UARTPORTCFG + 0x4) = 	uartPortCFG1;

#ifdef CONFIG_TCC_CODESONAR_BLOCKED
	if (port)
#endif /* CONFIG_TCC_CODESONAR_BLOCKED */
	{
		if (port->suspended) {

#if defined(CONFIG_TCC_BT_DEV)
		if(tcc_port->bt_use == 1)
			tcc_port->bt_suspend = 1;
#endif
		if(tcc_port->tx_dma_use){
			//tcc_pca953x_setup(PCA9539_U3_SLAVE_ADDR, BT_ON, OUTPUT, HIGH, SET_DIRECTION|SET_VALUE);
		}

		uart_resume_port(&tcc_uart_drv, port);

		if(port->line == TCC_GPS_UART){
#if defined(CONFIG_GPS)
			if(gps_k_flag){
				//gpio_direction_output(TCC_GPEXT1(6), 1);
			}
#endif
		}

#if defined(CONFIG_TCC_BT_DEV)
		if(tcc_port->bt_use == 1)
			tcc_port->bt_suspend = 0;
#endif

		port->suspended = 0;
		}
	}

	dbg("%s out...\n", __func__);
	return 0;
}

#else
#define tcc_serial_suspend NULL
#define tcc_serial_resume  NULL
#endif
	
#if defined(CONFIG_UART_DEBUG_BUS_SYSFS)
/**
 * show_info - attribute show func
 * @dev : device
 * @attr: device attribute
 * @buf : buffer to copy message
 * dsplay uart transfer information
 */
static ssize_t show_debug(struct device *dev,struct device_attribute *attr, char *buf)
{
	struct uart_port *port = dev_get_drvdata(dev);

	//CLOG(CLL_TRACE, "%s\n", __func__);

	if(!port)
		return 0;

	return sprintf(buf,"\n%s %d\n"
			"send bytes    : %d bytes\n"
			"recv bytes    : %d bytes\n"
			"Frame Error   : %d\n"
			"Overrun Error : %d\n",
			DRV_NAME, port->line,
			port->icount.tx, port->icount.rx_count,
			port->icount.frame,port->icount.overrun);
}
/**
  * clear_all_rec_idx - clear all record index
  */
static void clear_all_rec_idx(struct uart_port *port){

	port->icount.tx = port->icount.rx_count = port->icount.frame = port->icount.overrun = 0;

}
/**
 * store_debug - attribute store func
 * @dev : device
 * @attr: device attribute
 * @buf : buffer from user massage
 * process user command
 */
static ssize_t store_debug(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct uart_port *port = dev_get_drvdata(dev);

	if(!port)
		return 0;

	if(!strncmp(buf, "init", strlen("init"))){
		clear_all_rec_idx(port);
	}

	return count;
}

/* Register sysfs files */
static DEVICE_ATTR(debug, S_IWUSR | S_IRUGO, show_debug, store_debug);


#endif

static const struct dev_pm_ops tcc_serial_pm_ops = {
	.suspend = tcc_serial_suspend,
	.resume = tcc_serial_resume,
	.freeze = tcc_serial_suspend,
	.thaw = tcc_serial_resume,
	.restore = tcc_serial_resume,
};

extern void bcm_bt_lpm_exit_lpm_locked(struct uart_port *uport);

static void tcc_serial_parse_dt(struct device_node *np, struct tcc_uart_platform_data *tcc_platform_data)
{
	volatile PIOBUSCFG pIOBUSCFG = (volatile PIOBUSCFG)tcc_p2v(HwIOBUSCFG_BASE);

#if defined(CONFIG_ARCH_TCC896X)
	// Switch DMA3: UART0 -> UART4, UART2 -> UART6
	// Switch DMA4: UART1 -> UART5, UART3 -> UART7
	pIOBUSCFG->DMAREQSEL3.bREG.SEL |= 0x0C000300;
	pIOBUSCFG->DMAREQSEL4.bREG.SEL |= 0x60000C00;
#elif defined(CONFIG_ARCH_TCC897X)
	// Switch DMA3: UART0 -> UART4, UART1 -> UART5
 	// Switch DMA4: UART2 -> UART6, UART3 -> UART7
	pIOBUSCFG->DMAREQSEL3.bREG.SEL |= 0x6C000000;
	pIOBUSCFG->DMAREQSEL4.bREG.SEL |= 0x00000F00;
#else
	// Switch DMA0: UART0 -> UART4
	pIOBUSCFG->DMAREQSEL0.bREG.SEL |= 0x0C000000;
#endif

	of_property_read_u32(np, "bt_use", &tcc_platform_data->bt_use);
	of_property_read_u32(np, "rx_dma_use", &tcc_platform_data->rx_dma_use);
	of_property_read_u32(np, "rx_dma_buf_size", &tcc_platform_data->rx_dma_buf_size);
	of_property_read_u32(np, "rx_dma_base", &tcc_platform_data->rx_dma_base);
	of_property_read_u32(np, "rx_dma_ch", &tcc_platform_data->rx_dma_ch);
	of_property_read_u32(np, "rx_dma_intr", &tcc_platform_data->rx_dma_intr);
	of_property_read_u32(np, "rx_dma_mode", &tcc_platform_data->rx_dma_mode);
}


static int tcc_serial_probe(struct platform_device *dev)
{
	int error;
	struct resource *mem;
	int irq;
	struct tcc_uart_port *tcc_port;
	struct uart_port *port;
	struct device_node *np = dev->dev.of_node;
	struct tcc_uart_platform_data *tcc_platform_data;
	int id;
	int ret;

	if (np)
		id = of_alias_get_id(np, "serial");
	else
		id = dev->id;
	dbg("%s: id = %d\n", __func__, id);

#if !defined(CONFIG_TCC_CODESONAR_BLOCKED)
	if (id >= NR_PORTS || id < 0)
	{
		dev_err(&dev->dev, "%s : invalid id\n", __func__, id);
		return -ENOMEM;
	}
#endif /* !defined(CONFIG_TCC_CODESONAR_BLOCKED) */


	tcc_port = &tcc_serial_ports[id];
	port = &tcc_port->port;

	mem = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&dev->dev, "[UART%d] no memory resource?\n", id);
		return -EINVAL;
	}
	port->membase = devm_ioremap_resource(&dev->dev, mem);
	if (!port->membase) {
		dev_err(port->dev, "failed to ioremap\n");
		return -ENOMEM;
	}

	port->mapbase  = mem->start;
	irq = platform_get_irq(dev, 0);
	if (irq < 0) {
		dev_err(port->dev, "[UART%d] no irq resource?\n", id);
		return -ENODEV;
	}

	tcc_platform_data = devm_kzalloc(&dev->dev, sizeof(struct tcc_uart_platform_data), GFP_KERNEL);
	if (!tcc_platform_data)
		return -ENOMEM;

	tcc_serial_parse_dt(np, tcc_platform_data);

	/* Bus Clock Enable of UARTx */
	tcc_port->hclk = of_clk_get(np, 0);
	if (IS_ERR(tcc_port->hclk))
		tcc_port->hclk = NULL;
	else
		clk_prepare_enable(tcc_port->hclk);

	tcc_port->fclk = of_clk_get(np, 1);
	if (IS_ERR(tcc_port->fclk))
		tcc_port->fclk = NULL;
	else
		clk_prepare_enable(tcc_port->fclk);

	/* Set Interrrupt */
	tca_serial_intrinit();

	dbg("initialising uart ports...\n");

	tcc_port->bt_use     = 0;
	tcc_port->tx_dma_use = 0;
	tcc_port->rx_dma_use = 0;

	port->iotype	= UPIO_MEM;
	port->irq	= irq;
	port->uartclk	= 0;
	port->flags	= UPF_BOOT_AUTOCONF;
	port->ops	= &tcc_serial_ops;
	port->fifosize	= FIFOSIZE;
	port->line	= id;
	port->dev	= &dev->dev;
	tcc_uart_dev[id] 	= dev->dev;
	tcc_port->port.type     = PORT_TCC;
	tcc_port->port.irq      = irq;
	tcc_port->baud          = 0;
	tcc_port->info          = &tcc_uart_inf;
#if defined(CONFIG_TCC_BCM4330_LPM)
	if(tcc_port->port.line == TCC_BT_UART)
		tcc_port->wake_peer = bcm_bt_lpm_exit_lpm_locked;
#endif

	init_waitqueue_head(&(tcc_port->wait_q));

	ret = uart_add_one_port(&tcc_uart_drv, &tcc_port->port);
	if (ret) {
		dev_err(port->dev, "uart_add_one_port failure\n");
		goto probe_err;
	}

	spin_lock_init(&tcc_port->rx_lock);

#if defined(CONFIG_UART_DEBUG_BUS_SYSFS)
	error = device_create_file(&dev->dev, &dev_attr_debug);
	if(error)
		goto probe_err;

//	if( device_create_file_clog(&dev->dev) ){
//		goto probe_err;
//	}
#endif
	platform_set_drvdata(dev, &tcc_port->port);

	tcc_port->bt_use = tcc_platform_data->bt_use;

#if defined(CONFIG_TCC_BT_DEV)
	if(tcc_port->bt_use == 1) goto set_dma;
#endif

	INIT_WORK(&serial_irq_work, serial_irq_wq);
#if 0
	error = serial_loglevel_interrupt(&dev->dev);
#else
	error = serial_loglevel_set(&dev->dev);
#endif
	if (error < 0) {
		dev_err(port->dev, "serial_loglevel_interrupt failure\n");
	}
	
set_dma:

#if 0
	if(tcc_platform_data->tx_dma_use) {
		if (!tcc_malloc_dma_buf(&(tcc_port->tx_dma_buffer), tcc_platform_data->tx_dma_buf_size)) {
			dbg("Unable to attach UART TX DMA 1 channel\n");
			ret = -ENOMEM;
			goto probe_err;
		}

		tcc_port->tx_dma_use = 1;
		tcc_port->tx_dma_buffer.dma_core = io_p2v(tcc_platform_data->tx_dma_base);
		tcc_port->tx_dma_buffer.dma_ch   = tcc_platform_data->tx_dma_ch;
		tcc_port->tx_dma_buffer.dma_port = id;
		tcc_port->tx_dma_buffer.dma_intr = tcc_platform_data->tx_dma_intr;
		tcc_port->tx_dma_buffer.dma_mode = tcc_platform_data->tx_dma_mode;
		tcc_port->tx_dma_buffer.buf_size = tcc_platform_data->tx_dma_buf_size;

		init_waitqueue_head(&(tcc_port->wait_dma_q));
	}
#endif

	if (tcc_platform_data->rx_dma_use) {
		if(!tcc_malloc_dma_buf(&(tcc_port->rx_dma_buffer), tcc_platform_data->rx_dma_buf_size)) {
			dbg("Unable to attach UART RX DMA 1 channel\n");
			ret = -ENOMEM;
			goto probe_err;
		}

		tcc_port->rx_dma_use = 1;
		tcc_port->rx_dma_buffer.dma_core = io_p2v(tcc_platform_data->rx_dma_base);
		tcc_port->rx_dma_buffer.dma_ch   = tcc_platform_data->rx_dma_ch;
		tcc_port->rx_dma_buffer.dma_port = id;
		tcc_port->rx_dma_buffer.dma_intr = tcc_platform_data->rx_dma_intr;
		tcc_port->rx_dma_buffer.dma_mode = tcc_platform_data->rx_dma_mode;
		tcc_port->rx_dma_buffer.buf_size = tcc_platform_data->rx_dma_buf_size;
		tcc_port->rx_dma_tail = (unsigned long) tcc_port->rx_dma_buffer.dma_addr;
	}

	return ret;

probe_err:

	dbg("probe_err\n");

	if (tcc_port == NULL)
		return ret;

	if (tcc_port->fclk) {
		clk_disable_unprepare(tcc_port->fclk);
		clk_put(tcc_port->fclk);
		tcc_port->fclk = NULL;
	}
	if (tcc_port->hclk) {
		clk_disable_unprepare(tcc_port->hclk);
		clk_put(tcc_port->hclk);
		tcc_port->hclk = NULL;
	}
	if (tcc_port->tx_dma_use)
		tcc_free_dma_buf(&(tcc_port->tx_dma_buffer));
	if (tcc_port->rx_dma_use)
		tcc_free_dma_buf(&(tcc_port->rx_dma_buffer));

	kfree(tcc_platform_data);

	return ret;
}

#ifdef CONFIG_OF
static struct of_device_id tcc_uart_of_match[] = {
	{ .compatible = "telechips,tcc893x-uart" },
	{ .compatible = "telechips,tcc896x-uart" },
	{ .compatible = "telechips,tcc897x-uart" },
	{}
};
MODULE_DEVICE_TABLE(of, tcc_uart_of_match);
#endif

static struct platform_driver tcc_serial_drv = {
	.probe		= tcc_serial_probe,
	.remove		= tcc_serial_remove,
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
		.pm	= &tcc_serial_pm_ops,
		.of_match_table = of_match_ptr(tcc_uart_of_match),
	},
};

/* module initialisation code */
static int __init tcc_serial_modinit(void)
{
	int ret;

	ret = uart_register_driver(&tcc_uart_drv);
	if (ret < 0) {
		dbg(KERN_ERR "failed to register UART driver\n");
		return ret;
	}

	ret = platform_driver_register(&tcc_serial_drv);
	if(ret != 0) {
		uart_unregister_driver(&tcc_uart_drv);
		return ret;
	}

	return 0;
}

static void __exit tcc_serial_modexit(void)
{
	platform_driver_unregister(&tcc_serial_drv);
	uart_unregister_driver(&tcc_uart_drv);
}

module_init(tcc_serial_modinit);
module_exit(tcc_serial_modexit);

/*********************************************************************************************************
 *
 * The following is Console driver
 *
 *********************************************************************************************************/
#ifdef CONFIG_SERIAL_TCC_CONSOLE

static struct uart_port *cons_uart;

static void tcc_serial_console_putchar(struct uart_port *port, int ch)
{
	while (!(rd_regl(port, OFFSET_LSR) & LSR_THRE))
		cpu_relax();

	wr_regb(port, OFFSET_THR, ch);

}

static void tcc_console_write(struct console *co, const char *s,
		unsigned int count)
{
	struct uart_port *port;
	unsigned int t_ier, b_ier;
	unsigned long flags;
	int locked = 1;

	port = &tcc_serial_ports[co->index].port;

	local_irq_save(flags);
	if(port->sysrq)
		locked = 0;
	else if(oops_in_progress && !dbg_on)
		locked = spin_trylock(&port->lock);
	else{
		if(!dbg_on){
			spin_lock(&port->lock);
		}
	}

	if(!dbg_on){
		t_ier = rd_regl(port, OFFSET_IER);
		b_ier = t_ier;

		wr_regl(port, OFFSET_IER, t_ier & ~IER_ETXI);
	}

	uart_console_write(cons_uart, s, count, tcc_serial_console_putchar);

	if(!dbg_on)
		wr_regl(port, OFFSET_IER, b_ier);


	if(locked && !dbg_on)
		spin_unlock(&port->lock);
	local_irq_restore(flags);

}

static void __init tcc_serial_get_options(struct uart_port *port, int *baud,
		int *parity, int *bits)
{
}

static int __init tcc_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = CONSOLE_BAUDRATE;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	dbg("tcc_serial_console_setup: co=%p (%d), %s\n", co, co->index, options);

	/* is this a valid port */
	if (co->index == -1 || co->index >= NR_PORTS)
		co->index = CONSOLE_PORT;

	port = &tcc_serial_ports[co->index].port;
	port->ops = &tcc_serial_ops;

	/* is the port configured? */
	if (port->mapbase == 0x0) {
		dbg("port->mapbase is 0\n");
		port->mapbase = tcc_serial_ports[co->index].base_addr;
		port->membase = (unsigned char __iomem *)port->mapbase;
		port = &tcc_serial_ports[co->index].port;
		port->ops = &tcc_serial_ops;
	}

	cons_uart = port;

	if (options) {
		dbg("uart_parse_options\n");
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	} else {
		dbg("tcc_serial_get_options\n");
		tcc_serial_get_options(port, &baud, &parity, &bits);
	}

	dbg("tcc_serial_console_setup: port=%p (%d)\n", port, co->index);
	return uart_set_options(port, co, baud, parity, bits, flow);
}


static struct console tcc_serial_console = {
	.name		= TCC_SERIAL_NAME,
	.device		= uart_console_device,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.write		= tcc_console_write,
	.setup		= tcc_console_setup,
	.data		= &tcc_uart_drv,
};

/*
 * Initialise the console from one of the uart drivers
 */
static int tcc_console_init(void)
{
	dbg("%s\n", __func__);

	register_console(&tcc_serial_console);

	return 0;
}
console_initcall(tcc_console_init);
#endif /* CONFIG_SERIAL_TCC_CONSOLE */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("linux <linux@telechips.com>");
MODULE_DESCRIPTION("Telechips TCC Serial port driver");
