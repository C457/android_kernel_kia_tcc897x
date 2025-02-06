/****************************************************************************
 * arch/arm/mach-tcc893x/pm.c
 * Copyright (C) 2014 Telechips Inc.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 USA
****************************************************************************/

#include <linux/module.h>
#include <linux/cpu_pm.h>		// cpu_pm_enter(), cpu_pm_exit()
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/cacheflush.h>		// local_flush_tlb_all(), flush_cache_all();
#include <asm/suspend.h>
#include <asm/cputype.h>
#include <mach/bsp.h>
#include <mach/gpio.h>
#include <mach/pm.h>
#include <mach/sram_map.h>
#include <mach/iomap.h>
#include <plat/pmu_wakeup.h>
#include <plat/suspend.h>
#ifdef CONFIG_ARM_TRUSTZONE
#include <mach/smc.h>
#endif

#if defined(CONFIG_DAUDIO_LOW_BATTERY)
//D-Audio low battery irq event management//
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <mach/gpio.h>
#include <mach/daudio.h>
#include <mach/daudio_info.h>
#include <linux/workqueue.h>
////////////////////////////////////////////
#endif

#ifdef CONFIG_LK_DEBUG_LOG_BUF
#include <mach/lk_debug_logbuf.h>
#endif


#define pmu_wakeup_param_phys(x) (*(volatile unsigned *)(PMU_WAKEUP_ADDR+(4*x)))

extern TCC_REG *RegRepo;

#define PMU_WDTCTRL	0x008
#define PMU_WKUP0	0x020
#define PMU_WKUP1	0x024
#define PMU_WKPOL0	0x028
#define PMU_WKPOL1	0x02C
#define PMU_WKCLR0	0x030
#define PMU_WKCLR1	0x034
#define PMU_WKSTS0	0x038
#define PMU_WKSTS1	0x03C
#define PMU_PWRDN_TOP	0x050
#define PMU_PWRDN_XIN	0x098
#define PMU_WKMOD0	0x120
#define PMU_WKMOD1	0x124

#define CKC_CPU0	0x000
#define CKC_CPU1	0x004
#define CKC_MEM		0x008
#define CKC_IO		0x014
#define CKC_HSIO	0x020
#define CKC_SMU		0x024
#define CKC_CM4		0x02C
#define CKC_PLL0	0x040
#define CKC_PLL1	0x044
#define CKC_PLL2	0x048
#define CKC_PLL3	0x04C
#define CKC_PLL4	0x050

#define EDI_CTRL	0x000
#define EDI_CSNCFG0	0x004
#define EDI_CSNCFG1	0x008
#define EDI_OENCFG	0x00C
#define EDI_WENCFG	0x010
#define EDI_RDYCFG	0x014

#define NFC_CACYC	0x044
#define NFC_WRCYC	0x048
#define NFC_RECYC	0x04C
#define NFC_CTRL	0x050
#define NFC_IRQCFG	0x064
#define NFC_IRQ		0x068
#define NFC_RFWBASE	0x070

struct pwrctrl_gpio {
	int			gpio;
	enum of_gpio_flags	flag;
};

struct pm_power_gpios {
	int			count;
	struct pwrctrl_gpio	*pwrctrl;
};

static struct pm_power_gpios pwrctrl_gpios;

#if defined(CONFIG_DAUDIO_LOW_BATTERY)
void low_battery_irq_event_manager(void);	//140801, mobis, hklee, det_batt_low_4.5V
static struct dummy_dev_{
	int irq;
}dummy_dev;
#endif

//------------------------------------------------------------
// local function
static void gps_ant_pwr_force_on(void);
//------------------------------------------------------------

#ifdef CONFIG_LK_DEBUG_LOG_BUF
static void tcc_lk_log_resume(void)
{
	//at the booting, we do not need lk_log
	//lk_log_print_show();
	return;
}
#endif // CONFIG_LK_DEBUG_LOG_BUF


static void tcc_nfc_suspend(unsigned *nfc)
{
	void __iomem *reg = (void __iomem *)io_p2v(HwNFC_BASE);

	nfc[PM_NFC_CACYC] = pm_readl(reg + NFC_CACYC);
	nfc[PM_NFC_WRCYC] = pm_readl(reg + NFC_WRCYC);
	nfc[PM_NFC_RECYC] = pm_readl(reg + NFC_RECYC);
	nfc[PM_NFC_CTRL] = pm_readl(reg + NFC_CTRL);
	nfc[PM_NFC_IRQCFG] = pm_readl(reg + NFC_IRQCFG);
	nfc[PM_NFC_RFWBASE] = pm_readl(reg + NFC_RFWBASE);
}

static void tcc_nfc_resume(unsigned *nfc)
{
	void __iomem *reg = (void __iomem *)io_p2v(HwNFC_BASE);
	void __iomem *edi_reg = (void __iomem *)io_p2v(HwEDI_BASE);

	pm_writel((pm_readl(edi_reg+EDI_RDYCFG) & ~(0x000FFFFF)) | 0x32104, edi_reg+EDI_RDYCFG);
	pm_writel((pm_readl(edi_reg+EDI_CSNCFG0) & ~(0x0000FFFF)) | 0x8765, edi_reg+EDI_CSNCFG0);
	pm_writel((pm_readl(edi_reg+EDI_OENCFG) & ~(0x0000000F)) | 0x1, edi_reg+EDI_OENCFG);
	pm_writel((pm_readl(edi_reg+EDI_WENCFG) & ~(0x0000000F)) | 0x1, edi_reg+EDI_WENCFG);

	pm_writel(nfc[PM_NFC_CACYC], reg + NFC_CACYC);
	pm_writel(nfc[PM_NFC_WRCYC], reg + NFC_WRCYC);
	pm_writel(nfc[PM_NFC_RECYC], reg + NFC_RECYC);
	pm_writel(nfc[PM_NFC_CTRL], reg + NFC_CTRL);
	pm_writel(nfc[PM_NFC_IRQCFG], reg + NFC_IRQCFG);
	pm_writel(0xFFFFFFFF, reg + NFC_IRQ);
	pm_writel(nfc[PM_NFC_RFWBASE], reg + NFC_RFWBASE);

        //gps ant pwr force on
        gps_ant_pwr_force_on();
}

#ifdef CONFIG_PM_CONSOLE_NOT_SUSPEND
static void tcc_pm_uart_suspend(unsigned *uart, unsigned *portcfg)
{
	void __iomem *reg = (void __iomem *)io_p2v(TCC_PA_UART0);
	void __iomem *portcfg_reg = (void __iomem *)io_p2v(TCC_PA_UARTPORTCFG);
	
	/* backup uart portcfg */
	portcfg[0] = pm_readl(portcfg_reg + 0x00);
	portcfg[1] = pm_readl(portcfg_reg + 0x04);

	/* backup uart0 registers */
	uart[IER] = pm_readl(reg+0x4*IER);
	pm_writel(pm_readl(reg+0x4*IER) & ~(1<<2), reg+0x4*IER);	/* disable interrupt : ELSI */
	uart[LCR] = pm_readl(reg+0x04*LCR);
	pm_writel(pm_readl(reg+0x4*LCR) | (1<<7), reg+0x4*LCR);	/* DLAB = 1 */
	uart[DLL] = pm_readl(reg + 0x4*DLL);
	uart[DLM] = pm_readl(reg + 0x4*DLM);
	uart[MCR] = pm_readl(reg + 0x4*MCR);
	uart[AFT] = pm_readl(reg + 0x4*AFT);
	uart[UCR] = pm_readl(reg + 0x4*UCR);
}

static void tcc_pm_uart_resume(unsigned *uart, unsigned *portcfg)
{
	void __iomem *reg = (void __iomem *)io_p2v(TCC_PA_UART0);
	void __iomem *portcfg_reg = (void __iomem *)io_p2v(TCC_PA_UARTPORTCFG);
 
	/* restore uart portcfg */
	pm_writel(portcfg[0], portcfg_reg + 0x00);
	pm_writel(portcfg[1], portcfg_reg + 0x04);

	/* restore uart0 registers */
	pm_writel(pm_readl(reg+0x4*IER) & ~(1<<2), reg+0x4*IER);	/* disable interrupt : ELSI */
	pm_writel(pm_readl(reg+0x4*LCR) | (1<<7), reg+0x4*LCR);	/* DLAB = 1 */
	pm_writel(0x7, reg+0x4*FCR);
	pm_writel(uart[DLL], reg + 0x4*DLL);
	pm_writel(uart[DLM], reg + 0x4*DLM);
	pm_writel(uart[MCR], reg + 0x4*MCR);
	pm_writel(uart[AFT], reg + 0x4*AFT);
	pm_writel(uart[UCR], reg + 0x4*UCR);
	pm_writel(uart[LCR], reg + 0x4*LCR);
	pm_writel(uart[IER], reg + 0x4*IER);

	printk("\n======== wakeup source ========\n");
	printk("wakeup0_reg:0x%08x\n", (unsigned int)pmu_wakeup_param(PMU_WAKEUP_WUSTS0));
	printk("wakeup1_reg:0x%08x\n\n\n", (unsigned int)pmu_wakeup_param(PMU_WAKEUP_WUSTS1));
}
#endif

static void tcc_pm_register_backup(void)
{
#ifndef CONFIG_ARM_TRUSTZONE
	void __iomem *sram_reg = (void __iomem *)sram_p2v(SRAM_BOOT_ADDR);
#endif
	void __iomem *ckc_reg = (void __iomem *)io_p2v(TCC_PA_CKC);
	int i;
	unsigned a, b;

	/* set CNTVOFF (counter offset) to CNTVCT (curr. counter) */
	isb();
	asm volatile("mrrc p15, 1, %0, %1, c14" : "=r" (a), "=r" (b));
	pm_writel(~b, (void __iomem *)sram_p2v(ARM_TIMER_BACKUP_ADDR));
	pm_writel(~a, (void __iomem *)sram_p2v(ARM_TIMER_BACKUP_ADDR+0x4));

	cpu_pm_enter();
	cpu_cluster_pm_enter();

	memcpy(RegRepo->timer, (unsigned *)io_p2v(TCC_PA_TIMER), PM_TIMER_SIZE);
	memcpy(RegRepo->vic, (unsigned *)io_p2v(TCC_PA_VIC), PM_VIC_SIZE);
	memcpy(RegRepo->pic, (unsigned *)io_p2v(TCC_PA_PIC), PM_PIC_SIZE);

	/* GPIO Reg. Back-up */
	memcpy((void*)sram_p2v(GPIO_REPOSITORY_ADDR), (void *)io_p2v(TCC_PA_GPIO), GPIO_REPOSITORY_SIZE);

	/* backup pll,cpu,mem clocks for restore before exit sdram self-refresh */
	pm_writel(pm_readl(ckc_reg + CKC_PLL4), (void *)sram_p2v(CPU_DATA_REPOSITORY_ADDR+0x10));
	pm_writel(pm_readl(ckc_reg + CKC_MEM), (void *)sram_p2v(CPU_DATA_REPOSITORY_ADDR+0x14));

	tcc_ckc_backup_regs(1);

	/* clear secondary cpu's config values. */
	for (i=0 ; i<4 ; i++) {
#ifdef CONFIG_ARM_TRUSTZONE
		_tz_smc(SMC_CMD_SMP_SECONDARY_CFG, i, 0, 0);
#else
		pm_writel(0x0, sram_reg + SEC_VALID + (i*0x4));
#endif
	}

#ifdef CONFIG_SHUTDOWN_MODE
	{
		unsigned int mpidr = read_cpuid_mpidr();
		unsigned int cpu = ((mpidr>>8)&0xF)*2 + (mpidr&0xF);

		/* set main(resume) cpu's info. */
#ifdef CONFIG_ARM_TRUSTZONE
		_tz_smc(SMC_CMD_SMP_SECONDARY_CFG, cpu, 1, 0);
		_tz_smc(SMC_CMD_SMP_SECONDARY_ADDR, cpu, SRAM_RESUME_FUNC_ADDR, 0);
#else
		pm_writel(1, sram_reg + SEC_VALID + (cpu*0x4));
		pm_writel(SRAM_RESUME_FUNC_ADDR, sram_reg + SEC_START + (cpu*0x4));
#endif
	}
#endif

	if (pwrctrl_gpios.count) {
		for (i=0 ; i<pwrctrl_gpios.count; i++)
			gpio_direction_output(pwrctrl_gpios.pwrctrl[i].gpio, pwrctrl_gpios.pwrctrl[i].flag);
	}

	/* backup cpu offset */
	asm volatile ( "mrc p15, 0, %0, c13, c0, 4" : "=r" (RegRepo->TPIDRPRW));

#if defined(CONFIG_ARM_TRUSTZONE) && defined(CONFIG_SHUTDOWN_MODE)
	_tz_smc(SMC_CMD_SHUTDOWN, 0, 0, 0);
#endif
}

static void tcc_pm_register_restore(void)
{
	/* restore cpu offset */
	asm volatile ( "mcr p15, 0, %0, c13, c0, 4" :: "r" (RegRepo->TPIDRPRW));

	tcc_ckc_restore_regs();
	tcc_pm_nop_delay(100);

	__cpu_early_init();

 	/* SMU & PMU */
	memcpy((unsigned *)io_p2v(TCC_PA_PIC), RegRepo->pic, PM_PIC_SIZE);
	memcpy((unsigned *)io_p2v(TCC_PA_VIC), RegRepo->vic, PM_VIC_SIZE);
	memcpy((unsigned *)io_p2v(TCC_PA_TIMER), RegRepo->timer, PM_TIMER_SIZE);

	cpu_cluster_pm_exit();
	cpu_pm_exit();
}

void tcc_resume_from_sram(void)
{
	void __iomem *pmu_reg = (void __iomem *)TCC_PA_PMU;
	void __iomem *ckc_reg = (void __iomem *)TCC_PA_CKC;
	unsigned *src, *dest;
	unsigned int cnt, mode;

#ifdef CONFIG_SHUTDOWN_MODE
	FuncPtr pSdramFunc = (FuncPtr)SDRAM_INIT_FUNC_ADDR;
	mode = 1;
#else
	FuncPtr pSdramFunc = (FuncPtr)(SDRAM_SELF_REF_EXIT_ADDR);
	mode = 2;
#endif

	pmu_wakeup_param_phys(PMU_WAKEUP_WUSTS0) = pm_readl(pmu_reg + PMU_WKSTS0);
	pmu_wakeup_param_phys(PMU_WAKEUP_WUSTS1) = pm_readl(pmu_reg + PMU_WKSTS1);

	/* GPIO restore */
	src = (unsigned*)GPIO_REPOSITORY_ADDR;
	dest = (unsigned*)TCC_PA_GPIO;
	for(cnt=0 ; cnt<(GPIO_REPOSITORY_SIZE/4) ; cnt++)
		dest[cnt] = src[cnt];

	/* clear & disable wake-up */
	pm_writel(0x00000000, pmu_reg + PMU_WKUP0);
	pm_writel(0x00000000, pmu_reg + PMU_WKUP1);
	pm_writel(0xFFFFFFFF, pmu_reg + PMU_WKCLR0);
	pm_writel(0xFFFFFFFF, pmu_reg + PMU_WKCLR1);

	/* SSTL & IO Retention Disable */
	while (!(pm_readl(pmu_reg + PMU_PWRDN_XIN) & (1<<8)))	/* DMC0 */
		pm_writel(pm_readl(pmu_reg + PMU_PWRDN_XIN) | (1<<8), pmu_reg + PMU_PWRDN_XIN);
	while (!(pm_readl(pmu_reg + PMU_PWRDN_XIN) & (1<<9)))	/* DMC1 */
		pm_writel(pm_readl(pmu_reg + PMU_PWRDN_XIN) | (1<<9), pmu_reg + PMU_PWRDN_XIN);
	while (!(pm_readl(pmu_reg + PMU_PWRDN_XIN) & (1<<4)))
		pm_writel(pm_readl(pmu_reg + PMU_PWRDN_XIN) | (1<<4), pmu_reg + PMU_PWRDN_XIN);

	/* set cpu clock */
	pm_writel(0x86012bc3, ckc_reg+CKC_PLL3);		/* 700MHz@0.9 for CPU */
	while((pm_readl(ckc_reg+CKC_PLL3)&(1<<23)) == 0);	/* check lock status */
	pm_writel(0x00001F09, ckc_reg+CKC_CPU0);		/* CPU0 (Cortex-A7MP) */
	while(pm_readl(ckc_reg+CKC_CPU0)&(1<<29));

	/* restore MEM clock */
	pm_writel(0x00001f00, ckc_reg + CKC_MEM);	// MEM: XIN/2
	pm_writel(pm_readl((void *)(CPU_DATA_REPOSITORY_ADDR+0x10)), ckc_reg+CKC_PLL4);
	tcc_pm_nop_delay(0x1000);	
	pm_writel(pm_readl((void *)(CPU_DATA_REPOSITORY_ADDR+0x14)), ckc_reg+CKC_MEM);

	/* shudown: sdram re-init, sleep: self-refreah exit */
	tcc_pm_nop_delay(0x1000);
	pSdramFunc();
	tcc_pm_nop_delay(0x1000);

//	asm volatile ("resume_debug:");
//	asm volatile ("b resume_debug");

#ifdef CONFIG_ARM_TRUSTZONE
	asm volatile (
		"stmfd	sp!, {r0, lr} \n"
		"mov	r0, %1 \n"
		"mov	lr, pc \n"
		"mov	pc, %0 \n"
		"ldmfd	sp!, {r0, lr} \n"
		 :: "r" (TZ_SECUREOS_BASE), "r" (mode)
	);
#endif

	/* call tcc_cpu_resume() */
	asm volatile (
		"ldr	r0, [%0] \n"
		"mov	pc, r0 \n"
		 :: "r" (CPU_DATA_REPOSITORY_ADDR+0x04)
	);
	while(1);
}

void tcc_suspend_to_sram(unsigned mode)
{
	void __iomem	*pmu_reg = (void __iomem *)TCC_PA_PMU;
	void __iomem	*ckc_reg = (void __iomem *)TCC_PA_CKC;
	FuncPtr1	sdram_self_ref_enter = (FuncPtr1)SDRAM_SELF_REF_ENTER_ADDR;

	sdram_self_ref_enter(mode);
	tcc_pm_nop_delay(0x1000);

	/* Clock <- XIN, PLL <- OFF */
	pm_writel(0x0000FF00, ckc_reg + CKC_CPU0);
	pm_writel(0x00200014, ckc_reg + CKC_CPU1);
	pm_writel(0x00001F00, ckc_reg + CKC_MEM);
	pm_writel(0x00200014, ckc_reg + CKC_IO);
	pm_writel(0x00200014, ckc_reg + CKC_HSIO);
	pm_writel(0x00200014, ckc_reg + CKC_SMU);
	pm_writel(0x00200014, ckc_reg + CKC_CM4);

	pm_writel(pm_readl(ckc_reg + CKC_PLL0) & ~(1<<31), ckc_reg + CKC_PLL0);
	pm_writel(pm_readl(ckc_reg + CKC_PLL1) & ~(1<<31), ckc_reg + CKC_PLL1);
	pm_writel(pm_readl(ckc_reg + CKC_PLL2) & ~(1<<31), ckc_reg + CKC_PLL2);
	pm_writel(pm_readl(ckc_reg + CKC_PLL3) & ~(1<<31), ckc_reg + CKC_PLL3);
	pm_writel(pm_readl(ckc_reg + CKC_PLL4) & ~(1<<31), ckc_reg + CKC_PLL4);

	/* SSTL & IO Retention */
	while (pm_readl(pmu_reg + PMU_PWRDN_XIN) & (1<<8))	/* DMC0 */
		pm_writel(pm_readl(pmu_reg + PMU_PWRDN_XIN) & ~(1<<8), pmu_reg + PMU_PWRDN_XIN);
	while (pm_readl(pmu_reg + PMU_PWRDN_XIN) & (1<<9))	/* DMC1 */
		pm_writel(pm_readl(pmu_reg + PMU_PWRDN_XIN) & ~(1<<9), pmu_reg + PMU_PWRDN_XIN);
	while (pm_readl(pmu_reg + PMU_PWRDN_XIN) & (1<<4))
		pm_writel(pm_readl(pmu_reg + PMU_PWRDN_XIN) & ~(1<<4), pmu_reg + PMU_PWRDN_XIN);

	/*set wake-up trigger mode : edge */
	pm_writel(0xFFFFFFFF, pmu_reg + PMU_WKMOD0);
	pm_writel(0xFFFFFFFF, pmu_reg + PMU_WKMOD1);
	pm_writel(0x00000000, pmu_reg + PMU_WKUP0);
	pm_writel(0x00000000, pmu_reg + PMU_WKUP1);

	pm_writel(pmu_wakeup_param_phys(PMU_WAKEUP_WKPOL0), pmu_reg + PMU_WKPOL0);
	pm_writel(pmu_wakeup_param_phys(PMU_WAKEUP_WKUP0), pmu_reg + PMU_WKUP0);
	pm_writel(pmu_wakeup_param_phys(PMU_WAKEUP_WKPOL1), pmu_reg + PMU_WKPOL1);
	pm_writel(pmu_wakeup_param_phys(PMU_WAKEUP_WKUP1), pmu_reg + PMU_WKUP1);

	/* RTC Alarm Wake Up */
	pm_writel(pm_readl(pmu_reg + PMU_WKPOL0) & ~(1<<3), pmu_reg + PMU_WKPOL0);
	pm_writel(pm_readl(pmu_reg + PMU_WKUP0) | (1<<3), pmu_reg + PMU_WKUP0);

	/* Remocon Wake Up */
	pm_writel(pm_readl(pmu_reg + PMU_WKPOL1) & ~(1<<31), pmu_reg + PMU_WKPOL1);
	pm_writel(pm_readl(pmu_reg + PMU_WKUP1) | (1<<31), pmu_reg + PMU_WKUP1);

#ifdef CONFIG_SHUTDOWN_MODE
	while(1) {
		unsigned cnt;
		pm_writel(0x1, pmu_reg + PMU_PWRDN_TOP);
		tcc_pm_nop_delay(100);

		cnt = 0;
		if(cnt++ > 100){
			/* reboot */
			if (pm_readl(pmu_reg + PMU_WKSTS0) || pm_readl(pmu_reg + PMU_WKSTS1)) {
				pm_writel((1<<31)|(1<<0), pmu_reg + PMU_WDTCTRL);
				while(1);
			}
		}
	}
#else
	{
		FuncPtr pWakeUpFunc = (FuncPtr)SRAM_RESUME_FUNC_ADDR;
//		void __iomem *memcfg_reg = (void __iomem *)TCC_PA_MEMBUSCFG;

		/* Uniquify, Phy, SDRAM controller reset enable */
//		pm_writel(pm_readl(memcfg_reg + 0x4) & ~0x00700016, memcfg_reg + 0x4);

		// -------------------------------------------------------------------------
		// Enter Sleep !!
		////////////////////////////////////////////////////////////////////////////
		pm_writel(pm_readl(pmu_reg + PMU_PWRDN_XIN) | 0x1, pmu_reg + PMU_PWRDN_XIN);
		////////////////////////////////////////////////////////////////////////////

		/* Uniquify, Phy, SDRAM controller reset disable */
//		pm_writel(pm_readl(memcfg_reg + 0x4) | 0x00700016, memcfg_reg + 0x4);

		pWakeUpFunc();
	}
#endif
}

static int powerkey_wakeup_idx;
static int wakeup_by_powerkey(void)
{
	if (powerkey_wakeup_idx > 31) {
		if (pmu_wakeup_param(PMU_WAKEUP_WUSTS1) & (1<<(powerkey_wakeup_idx-32)))
			return 1;
	} else {
		if (pmu_wakeup_param(PMU_WAKEUP_WUSTS0) & (1<<(powerkey_wakeup_idx)))
			return 1;
	}
	return 0;
}

static void suspend_board_config(void)
{
	struct device_node *np;
	int gpio, i;

	np = of_find_compatible_node(NULL, NULL, "telechips,pm");
	if (!np)
		return;

	/* set gpio wakeup source */
	gpio = of_get_named_gpio(np, "powerkey-gpio", 0);
	if (gpio_is_valid(gpio))
		powerkey_wakeup_idx = set_gpio_to_wakeup(gpio, WAKEUP_EN, WAKEUP_ACT_LO);

	/* get power-ctrl gpios */
	pwrctrl_gpios.count = of_gpio_named_count(np, "gpio-powers");
	if (pwrctrl_gpios.count > 0) {
		pwrctrl_gpios.pwrctrl = kzalloc(sizeof(struct pwrctrl_gpio)*(pwrctrl_gpios.count), GFP_KERNEL);

		for (i=0 ; i < pwrctrl_gpios.count ; i++)
			pwrctrl_gpios.pwrctrl[i].gpio = of_get_named_gpio_flags(np, "gpio-powers", i, &(pwrctrl_gpios.pwrctrl[i].flag));
	}
	else
		pwrctrl_gpios.count = 0;
}


static struct tcc_suspend_ops suspend_ops = {
	.reg_backup = tcc_pm_register_backup,
	.reg_restore = tcc_pm_register_restore,
#ifdef CONFIG_PM_CONSOLE_NOT_SUSPEND
	.uart_suspend = tcc_pm_uart_suspend,
	.uart_resume = tcc_pm_uart_resume,
#endif
	.nfc_suspend = tcc_nfc_suspend,
	.nfc_resume = tcc_nfc_resume,
	.wakeup_by_powerkey = wakeup_by_powerkey,
	.remap = NULL,
#ifdef CONFIG_LK_DEBUG_LOG_BUF
	.lk_log_resume = tcc_lk_log_resume,
#endif
};

#define 			LOW_BATTERY_IRQ_MAX 6
static spinlock_t       low_battery_lock;
static int 			low_battery_count = 0;
struct work_struct low_battery_irq_work;
static void low_battery_irq_wq(struct work_struct *work);

static int __init tcc897x_suspend_init(void)
{
	suspend_board_config();
	tcc_suspend_set_ops(&suspend_ops);

//+[TCCQB] snapshot functions...
#ifdef CONFIG_SNAPSHOT_BOOT
	tcc_snapshot_set_ops(&suspend_ops);
#endif
//-[TCCQB]
//

#if defined(CONFIG_DAUDIO_LOW_BATTERY)
	/* register low battery irq */
	INIT_WORK(&low_battery_irq_work, low_battery_irq_wq);
	low_battery_irq_event_manager();	//140801, mobis, hklee, det_batt_low_4.5V
#endif
	return 0;
}
__initcall(tcc897x_suspend_init);

#if defined(CONFIG_DAUDIO_LOW_BATTERY)
/*===========================================================================
IRQ     (181212, mobis, hklee, ant_power_err)
===========================================================================*/
#define ANT_POWER_ERR           TCC_GPB(1)
#define GPS_ANT_PWR_ON          TCC_GPB(5)
#define MAX_NUM_OF_ANT_ERROR  20
static spinlock_t       gps_pwr_lock;
static DEFINE_MUTEX(ant_power_error);

static int	numof_ant_power_err = 0;
struct work_struct	ant_err_irq_work;


static void ant_power_err_wq(struct work_struct *work)
{
	mutex_lock(&ant_power_error);
	disable_irq(INT_EINT8);
	numof_ant_power_err++;
	gpio_set_value(GPS_ANT_PWR_ON, 0);
	msleep(1000);
	gpio_set_value(GPS_ANT_PWR_ON, 1);
	printk("GPS_ANT_PWR_ON(GPIO_B05) - LOW to HIGH!!, count=%d\n", numof_ant_power_err);
	msleep(200);
	enable_irq(INT_EINT8);
	mutex_unlock(&ant_power_error);
}

static irqreturn_t isr_ant_power_err(int irq, void* dev_id)
{
//	unsigned long flags;
//	spin_lock_irqsave(&gps_pwr_lock, flags);
        if(gpio_get_value(ANT_POWER_ERR))
                printk("ANT_POWER_ERR(GPIO_B01) - HIGH!!\n");
        else
		schedule_work(&ant_err_irq_work);
//	spin_unlock_irqrestore(&gps_pwr_lock, flags);
	return IRQ_HANDLED;
}

/*===========================================================================
IRQ	(140810, mobis, hklee, det_batt_low_4.5V)
===========================================================================*/

static void low_battery_irq_wq(struct work_struct *work)
{
	disable_irq(INT_EINT5);
	printk("kernel low_battery_count max count reached(%d).\n", low_battery_count);
}

static irqreturn_t isr_low_battery_management_routine(int irq, void* dev_id)
{
	unsigned long flags;

	spin_lock_irqsave(&low_battery_lock, flags);
	low_battery_count++;
#if defined(INCLUDE_XM)
	//sxm shutdown
	gpio_set_value(SIRIUS_GPIO_RESET, 1);
	mdelay(6);
	tcc_gpio_config(TCC_GPF(13), GPIO_FN(0)|GPIO_OUTPUT|GPIO_LOW|GPIO_PULL_DISABLE);
	tcc_gpio_config(TCC_GPF(14), GPIO_FN(0)|GPIO_OUTPUT|GPIO_LOW|GPIO_PULL_DISABLE);
	gpio_set_value(SIRIUS_GPIO_SHDN, 0);
#endif
	spin_unlock_irqrestore(&low_battery_lock, flags);
	if (low_battery_count >= LOW_BATTERY_IRQ_MAX)
	{
		schedule_work(&low_battery_irq_work);
	}
	return IRQ_HANDLED;
}

void gps_ant_pwr_force_on(void)
{
	////////2. ant power_err
	gpio_request(ANT_POWER_ERR, "ant_power_err");
         gpio_request(GPS_ANT_PWR_ON, "gps_ant_pwr_on");

	printk("ANT_POWER_ERR Value(%s)\n", gpio_get_value(ANT_POWER_ERR)?"HIGH":"LOW");
	printk("GPS_ANT_PWR_ON Value(%s)\n", gpio_get_value(GPS_ANT_PWR_ON)?"HIGH":"LOW");

	gpio_set_value(GPS_ANT_PWR_ON, 1);
	printk("GPS_ANT_PWR_ON(GPIO_B05) - force HIGH!!\n");

        if(gpio_get_value(ANT_POWER_ERR) == 0)
        {
		gpio_set_value(GPS_ANT_PWR_ON, 0);
		mdelay(20);
		gpio_set_value(GPS_ANT_PWR_ON, 1);
		printk("GPS_ANT_PWR_ON(GPIO_B05) - LOW to HIGH!!\n");
        }
        return;
}

/*===========================================================================
FUNCTION	(140810, mobis, hklee, det_batt_low_4.5V)
===========================================================================*/
void low_battery_irq_event_manager(void)
{
	int main_ver = daudio_main_version();
	int ret;
	volatile PGPION pgpio = (PGPION)tcc_p2v(HwGPIOB_BASE);

#if defined(INCLUDE_XM)
	ret = gpio_request(SIRIUS_GPIO_SHDN, "sirius_shutdown");
	if(ret)
		printk("%s, there is some trouble on xm shdn\n", __func__);
	ret = gpio_request(SIRIUS_GPIO_RESET, "sirius_reset");
	if(ret)
		printk("%s, there is some trouble on xm reset\n", __func__);
#endif

	pgpio->GPIS.bREG.GP15 = 0x1;
	dummy_dev.irq = INT_EINT5;
	tcc_gpio_config_ext_intr(INT_EINT5, EXTINT_GPIOB_15);//TCC_GPB(15) : 4.5V Detect
	irq_set_irq_type(INT_EINT5, IRQF_TRIGGER_RISING);
	ret = request_irq(INT_EINT5, isr_low_battery_management_routine, IRQF_SHARED, "LOW_BAT_DET_4_5V", &dummy_dev);
	if(ret < 0)
		printk(KERN_INFO "%s failed to LOW_BATT_DET request_irq : %d, %d\n", __func__, INT_EINT5, ret);
	else
		printk(KERN_INFO "%s %dth board - succeed in LOW_BAT_DET request_irq\n", __func__, main_ver);

        //gps ant pwr force on
        gps_ant_pwr_force_on();

        INIT_WORK(&ant_err_irq_work, ant_power_err_wq);

        pgpio->GPIS.bREG.GP01 = 0x1;
        dummy_dev.irq = INT_EINT8;
        tcc_gpio_config_ext_intr(INT_EINT8, EXTINT_GPIOB_01);
        irq_set_irq_type(INT_EINT8, IRQF_TRIGGER_FALLING);
        ret = request_threaded_irq(INT_EINT8, NULL, isr_ant_power_err, IRQF_SHARED | IRQF_ONESHOT, "ANT_POWER_ERR", &dummy_dev);
        if(ret < 0)
                printk(KERN_INFO "%s failed to ANT_POWER_ERR request_irq : %d, %d\n", __func__, INT_EINT8, ret);
        else
                printk(KERN_INFO "%s %dth board - succeed in ANT_POWER_ERR request_irq\n", __func__, main_ver+3);
}

#endif
