/****************************************************************************
 *   FileName    : tca_tccserial.c
 *   Description :
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips, Inc.
 *   ALL RIGHTS RESERVED
 *
 ****************************************************************************/

/*****************************************************************************
*
* Include
*
******************************************************************************/
#include <linux/kernel.h>
#include <mach/bsp.h>
#include <mach/io.h>
#include <plat/serial.h>
#include <mach/tca_serial.h>
#include <asm/mach-types.h>
#include <linux/clk.h>
#include <linux/gpio.h>

/*****************************************************************************
*
* Defines
*
******************************************************************************/
#ifndef CONFIG_OF
extern int uart_port_map_selection[8][5];
#endif
	
struct tcc_uart_port tcc_serial_ports[NR_PORTS] = {
	[0] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(tcc_uart_port[0].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= INT_UART,
			.uartclk	= 0,
			.fifosize	= FIFOSIZE,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 0,
		},
		.base_addr      = tcc_p2v(TCC_PA_UART0),
        .name = "uart0"
	},
	[1] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(tcc_uart_port[1].port.lock),
			.iotype		= UPIO_MEM,
			.uartclk	= 0,
			.irq		= INT_UART,
			.fifosize	= FIFOSIZE,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 1,
		},
        .base_addr      = tcc_p2v(TCC_PA_UART1),
        .name = "uart1"
	},
	[2] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(tcc_uart_port[2].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= INT_UART,
			.uartclk	= 0,
			.fifosize	= FIFOSIZE,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 2,
		},
        .base_addr      = tcc_p2v(TCC_PA_UART2),
        .name = "uart2"
	},
	[3] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(tcc_uart_port[3].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= INT_UART,
			.uartclk	= 0,
			.fifosize	= FIFOSIZE,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 3,
		},
        .base_addr      = tcc_p2v(TCC_PA_UART3),
        .name = "uart3"
	},
	[4] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(tcc_uart_port[4].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= INT_UART,
			.uartclk	= 0,
			.fifosize	= FIFOSIZE,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 4,
		},
        .base_addr      = tcc_p2v(TCC_PA_UART4),
        .name = "uart4"
	},
	[5] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(tcc_uart_port[5].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= INT_UART,
			.uartclk	= 0,
			.fifosize	= FIFOSIZE,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 5,
		},
        .base_addr      = tcc_p2v(TCC_PA_UART5),
        .name = "uart5"
	},
	[6] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(tcc_uart_port[6].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= INT_UART,
			.uartclk	= 0,
			.fifosize	= FIFOSIZE,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 6,
		},
        .base_addr      = tcc_p2v(TCC_PA_UART6),
        .name = "uart6"
	},
	[7] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(tcc_uart_port[7].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= INT_UART,
			.uartclk	= 0,
			.fifosize	= FIFOSIZE,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 7,
		},
        .base_addr      = tcc_p2v(TCC_PA_UART7),
        .name = "uart7"
	},	

};

#ifndef CONFIG_OF
/*****************************************************************************
* Function Name : tca_serial_portinit(int nCh, int nPort, unsigned long* pvGpioAddr, unsigned long* pvPortMuxAddr)
******************************************************************************
* Desription    : tca_serial_portinit
* Parameter     : int nCh, int nPort, unsigned long* pvGpioAddr, unsigned long* pvPortMuxAddr
* Return        : None
******************************************************************************/
void tca_serial_portinit(int nFlag_fc, int nPort)
{
	tca_serial_gpio_setting(nFlag_fc, nPort);
}

void tca_serial_gpio_setting(int nFlag_fc, int nPort)
{
	//volatile PUARTPORTCFG pPORT = (volatile PUARTPORTCFG)tcc_p2v(TCC_PA_UARTPORTCFG);
	//volatile PGPIO pGPIO = (volatile PGPIO)tcc_p2v(HwGPIO_BASE);
	//unsigned int phy_port;
	unsigned int hw_flow_ctrl;

	if (nPort > 7)
		return;

	#if defined(INCLUDE_XM)
	if (nPort == 4)
	{
	       printk("%s() skipped due to SXM requirement\n", __FUNCTION__);
	       return;
	}	
	#endif

	if (nFlag_fc == 0)
		hw_flow_ctrl = 0;
	else
		hw_flow_ctrl = 1;

	if(uart_port_map_selection[nPort][0] != -1){
		tcc_gpio_config(uart_port_map_selection[nPort][0], uart_port_map_selection[nPort][4]); // tx
		tcc_gpio_config(uart_port_map_selection[nPort][1], uart_port_map_selection[nPort][4]); // rx
		if(hw_flow_ctrl == 1){
			if(uart_port_map_selection[nPort][2] != 0){
				tcc_gpio_config(uart_port_map_selection[nPort][2], uart_port_map_selection[nPort][4]); //rts
				tcc_gpio_config(uart_port_map_selection[nPort][3], uart_port_map_selection[nPort][4]); //cts
			}
		}
	}


}
#endif

void tca_serial_intrinit(void)
{
/*
#if (1)
	#warning: Please do not control directly system registers.
	WARN(1, "Please do not control directly system registers.\n");
#else
    volatile PPIC pPIC = (volatile PPIC)tcc_p2v(HwPIC_BASE);

    pPIC->SEL1.bREG.UART = 1;
    pPIC->INTMSK1.bREG.UART = 1;
    pPIC->MODE1.bREG.UART = 1; // Level trigger
#endif
*/
}

int tca_serial_port_pullup(int nPort, int enable, unsigned long long uartPortCFG)
{
#ifndef CONFIG_OF
	#define PORT_NUM 8
	int i;
	unsigned char phy_port[PORT_NUM];

	for(i=0 ; i<PORT_NUM ; i++)
		phy_port[i] = (uartPortCFG>>(i*8))&0xFF;

	if(uart_port_map_selection[nPort][0]  == -1){
		printk("uart_port_map is not assigned\n");
		return -1;
	}
	
	if(enable)
		tcc_gpio_config(uart_port_map_selection[nPort][0], GPIO_PULLUP);
	else
		tcc_gpio_config(uart_port_map_selection[nPort][0], GPIO_PULLDOWN);
#endif
	return 0;
}
/*****************************************************************************
* Function Name : tca_serial_dmaclrinterrupt(unsigned nDmanum, unsigned long* pVirtualDmaAddr)
******************************************************************************
* Desription    : tca_serial_dmaclrinterrupt
* Parameter     : unsigned nDmanum, unsigned long* pVirtualDmaAddr
* Return        : success(SUCCESS)
******************************************************************************/
unsigned int tca_serial_dmaclrinterrupt(unsigned nDmanum, unsigned long* pVirtualDmaAddr)
{
	PGDMACTRL pDMA = (PGDMACTRL)pVirtualDmaAddr;
	switch(	nDmanum ){
		case 0:
			pDMA->CHCTRL0.nREG |= Hw3;
			break;
		case 1:
			pDMA->CHCTRL1.nREG |= Hw3;
			break;
		case 2:
			pDMA->CHCTRL2.nREG |= Hw3;
			break;
		default:
			return 0;
	}
	return 1;
}

/*****************************************************************************
* Function Name : tca_dma_dmacurrentaddress(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
******************************************************************************
* Desription    : tca_dma_dmacurrentaddress
* Parameter     : int m_DmaNumber, unsigned long* pVirtualDmaAddr
* Return        :
******************************************************************************/
unsigned long tca_dma_dmacurrentaddress(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
{
	PGDMACTRL pDMA = (PGDMACTRL)pVirtualDmaAddr;
	switch(m_DmaNumber) {
		case 0:
			return  (pDMA->C_DADR0.nREG);
		case 1:
			return  (pDMA->C_DADR1.nREG);
		case 2:
			return  (pDMA->C_DADR2.nREG);
		default:
			return 0;
	}
}

/*****************************************************************************
* Function Name : tca_dma_dmadeststartaddress(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
******************************************************************************
* Desription    : tca_dma_dmadeststartaddress
* Parameter     : int m_DmaNumber, unsigned long* pVirtualDmaAddr
* Return        :
******************************************************************************/
unsigned long tca_dma_dmadeststartaddress(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
{
	PGDMACTRL pDMA = (PGDMACTRL)pVirtualDmaAddr;
	switch(m_DmaNumber){
		case 0:
			return  (pDMA->ST_DADR0.nREG);
		case 1:
			return  (pDMA->ST_DADR1.nREG);
		case 2:
			return  (pDMA->ST_DADR2.nREG);
		default:
			return 0;
	}
	return 0;
}

/*****************************************************************************
* Function Name : tca_dma_clrien(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
******************************************************************************
* Desription    : tca_dma_clrien
* Parameter     : int m_DmaNumber, unsigned long* pVirtualDmaAddr
* Return        :
******************************************************************************/
int tca_dma_clrien(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
{
	PGDMACTRL pDMA = (PGDMACTRL)pVirtualDmaAddr;
	switch(m_DmaNumber){
		case 0:
			pDMA->CHCTRL0.nREG &= ~Hw2;
			break;
		case 1:
			pDMA->CHCTRL1.nREG &= ~Hw2;
			break;
		case 2:
			pDMA->CHCTRL2.nREG &= ~Hw2;
			break;
		default:
			return 0;
	}
	return 0;
}

/*****************************************************************************
* Function Name : tca_dma_setien(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
******************************************************************************
* Desription    : tca_dma_setien
* Parameter     : int m_DmaNumber, unsigned long* pVirtualDmaAddr
* Return        :
******************************************************************************/
int tca_dma_setien(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
{
	PGDMACTRL pDMA = (PGDMACTRL)pVirtualDmaAddr;
	switch(m_DmaNumber){
		case 0:
			pDMA->CHCTRL0.nREG |= Hw2;
			break;
		case 1:
			pDMA->CHCTRL1.nREG |= Hw2;
			break;
		case 2:
			pDMA->CHCTRL2.nREG |= Hw2;
			break;
		default:
			return 0;
	}
	return 0;
}

/*****************************************************************************
* Function Name : tca_dma_clren(unsigned long m_DmaNumber, int m_DmaChNumber)
******************************************************************************
* Desription    : Clear DMA enable bit
* Parameter     : unsigned long m_DmaNumber, int m_DmaChNumber
* Return        :
******************************************************************************/
int tca_dma_clren(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
{
    volatile PGDMACTRL pDMA = (volatile PGDMACTRL)pVirtualDmaAddr;

    switch(	m_DmaNumber ){
        case 0:
            pDMA->CHCTRL0.nREG &= ~Hw0;
            break;
        case 1:
            pDMA->CHCTRL1.nREG &= ~Hw0;
            break;
        case 2:
            pDMA->CHCTRL2.nREG &= ~Hw0;
            break;
        default:
            return 0;
    }
    return 1;
}

/*****************************************************************************
* Function Name : tca_dma_seten(unsigned long m_DmaNumber, int m_DmaChNumber)
******************************************************************************
* Desription    : Set DMA enable bit
* Parameter     : unsigned long m_DmaNumber, int m_DmaChNumber
* Return        :
******************************************************************************/
int tca_dma_seten(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
{
    volatile PGDMACTRL pDMA = (volatile PGDMACTRL)pVirtualDmaAddr;

    switch(	m_DmaNumber ){
        case 0:
            pDMA->CHCTRL0.nREG |= Hw0;
            break;
        case 1:
            pDMA->CHCTRL1.nREG |= Hw0;
            break;
        case 2:
            pDMA->CHCTRL2.nREG |= Hw0;
            break;
        default:
            return 0;
    }
    return 1;
}

/*****************************************************************************
* Function Name : tca_dma_ctrl(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
******************************************************************************
* Desription    : tca_dma_ctrl
* Parameter     : int m_DmaNumber, unsigned long* pVirtualDmaAddr
* Return        :
******************************************************************************/
int    tca_dma_ctrl(int m_DmaNumber, unsigned long* pVirtualDmaAddr)
{
	PGDMACTRL pDMA = (PGDMACTRL)pVirtualDmaAddr;

	switch(m_DmaNumber){
	case 0:
		return  (pDMA->CHCTRL0.nREG);
	case 1:
		return  (pDMA->CHCTRL1.nREG);
	case 2:
		return  (pDMA->CHCTRL2.nREG);
	default:
		return 0;
	}
}


/*****************************************************************************
* Function Name : tca_dma_setconfig(
	iunsigned uCH,
	void *pSRT,
	unsigned uSPARAM,
	void *pDST,
	unsigned uDPARAM,
	unsigned	uCHCTRL,
	unsigned uSize,
	unsigned channel,
	unsigned mode,
	unsigned long* pVirtualDmaAddr
)
******************************************************************************
* Desription    : tca_dma_setconfig
* Parameter     :
* Return        :
******************************************************************************/
void tca_dma_setconfig(
    unsigned uCH,
	void* pSRT,
	unsigned uSPARAM,
	void* pDST,
	unsigned uDPARAM,
	unsigned	uCHCTRL,
	unsigned uSize,
	unsigned channel,
	unsigned mode,
	unsigned long* pVirtualDmaAddr
)
{
	PGDMACTRL pDMA = (PGDMACTRL)pVirtualDmaAddr;

	switch( uCH )   {
	case 0:
		if (uSize){
        	// Set Source Address & Source Parameter (mask + increment)
			 pDMA->ST_SADR0.nREG = (int)pSRT;
			 pDMA->SPARAM0.nREG = uSPARAM;

			 // Set Dest Address & Dest Parameter (mask + increment)
			 pDMA->ST_DADR0.nREG = (int)pDST;
			 pDMA->DPARAM0.nREG = uDPARAM;
			 pDMA->RPTCTRL0.nREG &= ~Hw31;


			 pDMA->EXTREQ0.nREG = tca_uart_channelselect(channel, mode);
			 pDMA->HCOUNT0.nREG = uSize;
		}

		 pDMA->CHCTRL0.nREG = uCHCTRL;
		 pDMA->CHCTRL0.nREG |= Hw0;
		break;
	case 1:
		if (uSize){
		// Set Source Address & Source Parameter (mask + increment)
			 pDMA->ST_SADR1.nREG = (int)pSRT;
			 pDMA->SPARAM1.nREG = uSPARAM;

        	// Set Dest Address & Dest Parameter (mask + increment)
			 pDMA->ST_DADR1.nREG = (int)pDST;
			 pDMA->DPARAM1.nREG = uDPARAM;
			 pDMA->RPTCTRL1.nREG &= ~Hw31;

			 pDMA->EXTREQ1.nREG = tca_uart_channelselect(channel, mode);
			 pDMA->HCOUNT1.nREG = uSize;
        	}

		 pDMA->CHCTRL1.nREG = uCHCTRL;
		 pDMA->CHCTRL1.nREG |= Hw0;
        break;
	case 2:
		if (uSize){
		// Set Source Address & Source Parameter (mask + increment)
			 pDMA->ST_SADR2.nREG = (int)pSRT;
			 pDMA->SPARAM2.nREG = uSPARAM;

        	// Set Dest Address & Dest Parameter (mask + increment)
			 pDMA->ST_DADR2.nREG = (int)pDST;
			 pDMA->DPARAM2.nREG = uDPARAM;
			 pDMA->RPTCTRL2.nREG &= ~Hw31;

			 pDMA->EXTREQ2.nREG = tca_uart_channelselect(channel, mode);
			 pDMA->HCOUNT2.nREG = uSize;
        	}

		 pDMA->CHCTRL2.nREG = uCHCTRL;
		 pDMA->CHCTRL2.nREG |= Hw0;
        break;
	}
}

/*****************************************************************************
* Function Name : tca_uart_channelselectint channel, int mode)
******************************************************************************
* Desription    : tca_serial_portinit
* Parameter     : int channel, int mode
* Return        :
******************************************************************************/
int tca_uart_channelselect(int channel, int mode)
{

	switch(channel){
	case 0:
		return (mode?HwEXTREQ_UART0_RX:HwEXTREQ_UART0_TX);
		break;
	case 1:
		return (mode?HwEXTREQ_UART1_RX:HwEXTREQ_UART1_TX);
		break;
	case 2:
		return (mode?HwEXTREQ_UART2_RX:HwEXTREQ_UART2_TX);
		break;
	case 3:
		return (mode?HwEXTREQ_UART3_RX:HwEXTREQ_UART3_TX);
		break;
	case 4:
		return (mode?HwEXTREQ_UART0_RX:HwEXTREQ_UART0_TX);
		break;
	case 5:
		return (mode?HwEXTREQ_UART1_RX:HwEXTREQ_UART1_TX);
		break;
	case 6:
		return (mode?HwEXTREQ_UART2_RX:HwEXTREQ_UART2_TX);
		break;
	case 7:
		return (mode?HwEXTREQ_UART3_RX:HwEXTREQ_UART3_TX);
	default :
		break;
	}
	return 0;
}

