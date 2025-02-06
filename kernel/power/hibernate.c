//+[TCCQB] Changed for QuickBoot
/*
 * kernel/power/hibernate.c - Hibernation (a.k.a suspend-to-disk) support.
 *
 * Copyright (c) 2003 Patrick Mochel
 * Copyright (c) 2003 Open Source Development Lab
 * Copyright (c) 2004 Pavel Machek <pavel@ucw.cz>
 * Copyright (c) 2009 Rafael J. Wysocki, Novell Inc.
 * Copyright (C) 2012 Bojan Smojver <bojan@rexursive.com>
 *
 * This file is released under the GPLv2.
 */

#include <linux/export.h>
#include <linux/suspend.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/async.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/pm.h>
#include <linux/console.h>
#include <linux/cpu.h>
#include <linux/freezer.h>
#include <linux/gfp.h>
#include <linux/syscore_ops.h>
#include <linux/ctype.h>
#include <linux/genhd.h>
#include <trace/events/power.h>

#include "power.h"

#include <mach/iomap.h>
#define PMU_USSTATUS        0x1C

#include <linux/qb_manager.h>	// For QuickBoot Definitions. ( Properties... )

#include <linux/sysrq.h> //[TCCQB] Making QB_DATA
/*
 *	================  Additional headers for snapshot boot.  =================
 */
#include <linux/kmod.h>	// call_usermodehelper( )
#include <linux/vmalloc.h>	// vmalloc( )

#ifdef CONFIG_LK_DEBUG_LOG_BUF
#include <mach/lk_debug_logbuf.h>
#endif

extern u32 read_usstatus(void);	// to read usstatus register ( LK boot time )


/*
 *	=================  For FileSystem with snapshot boot  ====================
 */
#include "../fs/mount.h"	// real_mount( )
#include <asm/io.h>			// umount( )
extern void mntput_no_expire(struct mount *mnt);
extern int do_umount(struct mount *mnt, int flags);
extern int kern_path(const char *name, unsigned int flags, struct path *path);
extern void thaw_flush_kernel_threads(void);

#if defined(CONFIG_ANDROID)
/*		QB UN-Mount List - ext4		*/
#if defined(CONFIG_DAUDIO_KK)
#if defined(PIO_WIDE_64GB_PARTITION)
static char *qb_um_list[] = {
	"/data/tombstones",
	"/data/system/dropbox",
	"/data/misc/dio",
	"/data",
	"/cache",
	"/storage/upgrade",
	"/storage/vr2",
	"/storage/log",
	"/oem_data"
};
#elif defined(PIO_WIDE_128GB_PARTITION)
static char *qb_um_list[] = {
	"/data/tombstones",
	"/data/system/dropbox",
	"/data/misc/dio",
	"/data",
	"/cache",
	"/storage/upgrade",
	"/storage/vr2",
	"/storage/log",
	"/oem_data"
};
#elif defined(PIO_WIDE_4GB_PARTITION)
static char *qb_um_list[] = {
	"/data/tombstones",
	"/data/system/dropbox",
	"/data/misc/dio",
	"/data",
	"/cache",
	"/storage/upgrade",
	"/oem_data",
	"/storage/log",
};
#elif defined(PIO_WIDE_8GB_PARTITION)
static char *qb_um_list[] = {
	"/data/tombstones",
	"/data/system/dropbox",
	"/data/misc/dio",
	"/data",
	"/cache",
	"/storage/upgrade",
	"/oem_data",
	"/storage/log",
};
#elif defined(PIO_WIDE_8GB_PARTITION2)
static char *qb_um_list[] = {
	"/data/tombstones",
	"/data/system/dropbox",
	"/data/misc/dio",
	"/data",
	"/cache",
	"/storage/upgrade",
	"/oem_data",	
	"/storage/vr1",
    "/storage/log",
};
#else
static char *qb_um_list[] = {
//	"/data/tombstones",
//	"/data/system/dropbox",
	"/data/misc/dio",
	"/data",
	"/cache",
	"/storage/upgrade",
	"/storage/mymusic_actual",
	"/storage/vr",
	"/storage/log",
};
#endif //PIO_WIDE_64GB_PARTITION
#else
static char *qb_um_list[] = {
	"/data",
	"/cache",
};
#endif
#endif


/*
 *	====================  For Time Log with snapshot boot  ====================
 */
int lk_boot_time = 0;
int kernel_boot_time = 0;
unsigned long long frameworks_boot_time = 0;

/*		Time for QuickBoot Restore.		*/
unsigned long long time_total_snapshot = 0;
unsigned long long time_core_resume = 0;
unsigned long long time_dpm_resume = 0;
unsigned long long time_task_resume = 0;
unsigned long long time_checkdisk = 0;
unsigned long long time_etc_resume = 0;


/*
 *	========================  QuickBoot WatchDog  ============================
 */
#ifdef CONFIG_QUICKBOOT_WATCHDOG
extern void tccwdt_qb_init(void);
extern void tccwdt_qb_exit(void);
extern void tccwdt_qb_kickdog(void);
extern void tccwdt_qb_reset_time(int wdt_sec);
#else
/* If CONFIG_QUICKBOOT_WATCHDOG is not enabled, followings are not decleared in watchdog driver.
 * So, dumy watchdog functions are decleared here.	*/
void tccwdt_qb_init(void) { }
void tccwdt_qb_exit(void) { }
void tccwdt_qb_kickdog(void) { }
void tccwdt_qb_reset_time(int wdt_sec) { }
#endif


/*
 *	======================  QuickBoot suspend / resume  ========================
 */
#ifdef CONFIG_SNAPSHOT_BOOT
extern void snapshot_mmu_switching(void);
extern void snapshot_state_store(void);
extern void snapshot_state_restore(void);
unsigned int reg[64];
extern void restore_snapshot_cpu_reg(void);
extern int save_cpu_reg_for_snapshot(unsigned int ptable, unsigned int pReg, void *);
#endif


/*
 *	============================  QuickBoot flags  ==============================
 */
#ifdef CONFIG_SNAPSHOT_BOOT
extern unsigned int get_do_snapshot_boot(void);
extern unsigned int set_do_snapshot_boot(unsigned int set_num);
#endif
extern int get_in_suspend(void);
extern int set_in_suspend(int set_num);



#ifdef CONFIG_SNAPSHOT_BOOT
	#ifdef CONFIG_NOCOMPRESS_SNAPSHOT
static int nocompress = 1;
	#else
static int nocompress = 0;
	#endif
#else
static int nocompress = 0;
#endif

static int noresume = 0;
static int nohibernate;
static int resume_wait;
static unsigned int resume_delay;
static char resume_file[256] = CONFIG_PM_STD_PARTITION;
dev_t swsusp_resume_device;
sector_t swsusp_resume_block;
//__visible int in_suspend __nosavedata;

extern unsigned int do_hibernation;
extern unsigned int do_hibernate_boot;



enum {
	HIBERNATION_INVALID,
	HIBERNATION_PLATFORM,
	HIBERNATION_SHUTDOWN,
	HIBERNATION_REBOOT,
#ifdef CONFIG_SUSPEND
	HIBERNATION_SUSPEND,
#endif
	/* keep last */
	__HIBERNATION_AFTER_LAST
};
#define HIBERNATION_MAX (__HIBERNATION_AFTER_LAST-1)
#define HIBERNATION_FIRST (HIBERNATION_INVALID + 1)

static int hibernation_mode = HIBERNATION_REBOOT;

bool freezer_test_done;

static const struct platform_hibernation_ops *hibernation_ops;

bool hibernation_available(void)
{
	return (nohibernate == 0);
}

/**
 * hibernation_set_ops - Set the global hibernate operations.
 * @ops: Hibernation operations to use in subsequent hibernation transitions.
 */
void hibernation_set_ops(const struct platform_hibernation_ops *ops)
{
	if (ops && !(ops->begin && ops->end &&  ops->pre_snapshot
	    && ops->prepare && ops->finish && ops->enter && ops->pre_restore
	    && ops->restore_cleanup && ops->leave)) {
		WARN_ON(1);
		return;
	}
	lock_system_sleep();
	hibernation_ops = ops;
	if (ops)
		hibernation_mode = HIBERNATION_PLATFORM;
	else if (hibernation_mode == HIBERNATION_PLATFORM)
		hibernation_mode = HIBERNATION_SHUTDOWN;

	unlock_system_sleep();
}
EXPORT_SYMBOL_GPL(hibernation_set_ops);

static bool entering_platform_hibernation;

bool system_entering_hibernation(void)
{
	return entering_platform_hibernation;
}
EXPORT_SYMBOL(system_entering_hibernation);

#ifdef CONFIG_PM_DEBUG
static void hibernation_debug_sleep(void)
{
	printk(KERN_INFO "hibernation debug: Waiting for 5 seconds.\n");
	mdelay(5000);
}

static int hibernation_test(int level)
{
	if (pm_test_level == level) {
		hibernation_debug_sleep();
		return 1;
	}
	return 0;
}
#else /* !CONFIG_PM_DEBUG */
static int hibernation_test(int level) { return 0; }
#endif /* !CONFIG_PM_DEBUG */

/**
 * platform_begin - Call platform to start hibernation.
 * @platform_mode: Whether or not to use the platform driver.
 */
static int platform_begin(int platform_mode)
{
	return (platform_mode && hibernation_ops) ?
		hibernation_ops->begin() : 0;
}

/**
 * platform_end - Call platform to finish transition to the working state.
 * @platform_mode: Whether or not to use the platform driver.
 */
static void platform_end(int platform_mode)
{
	if (platform_mode && hibernation_ops)
		hibernation_ops->end();
}

/**
 * platform_pre_snapshot - Call platform to prepare the machine for hibernation.
 * @platform_mode: Whether or not to use the platform driver.
 *
 * Use the platform driver to prepare the system for creating a hibernate image,
 * if so configured, and return an error code if that fails.
 */

static int platform_pre_snapshot(int platform_mode)
{
	return (platform_mode && hibernation_ops) ?
		hibernation_ops->pre_snapshot() : 0;
}

/**
 * platform_leave - Call platform to prepare a transition to the working state.
 * @platform_mode: Whether or not to use the platform driver.
 *
 * Use the platform driver prepare to prepare the machine for switching to the
 * normal mode of operation.
 *
 * This routine is called on one CPU with interrupts disabled.
 */
static void platform_leave(int platform_mode)
{
	if (platform_mode && hibernation_ops)
		hibernation_ops->leave();
}

/**
 * platform_finish - Call platform to switch the system to the working state.
 * @platform_mode: Whether or not to use the platform driver.
 *
 * Use the platform driver to switch the machine to the normal mode of
 * operation.
 *
 * This routine must be called after platform_prepare().
 */
static void platform_finish(int platform_mode)
{
	if (platform_mode && hibernation_ops)
		hibernation_ops->finish();
}

/**
 * platform_pre_restore - Prepare for hibernate image restoration.
 * @platform_mode: Whether or not to use the platform driver.
 *
 * Use the platform driver to prepare the system for resume from a hibernation
 * image.
 *
 * If the restore fails after this function has been called,
 * platform_restore_cleanup() must be called.
 */
static int platform_pre_restore(int platform_mode)
{
	return (platform_mode && hibernation_ops) ?
		hibernation_ops->pre_restore() : 0;
}

/**
 * platform_restore_cleanup - Switch to the working state after failing restore.
 * @platform_mode: Whether or not to use the platform driver.
 *
 * Use the platform driver to switch the system to the normal mode of operation
 * after a failing restore.
 *
 * If platform_pre_restore() has been called before the failing restore, this
 * function must be called too, regardless of the result of
 * platform_pre_restore().
 */
static void platform_restore_cleanup(int platform_mode)
{
	if (platform_mode && hibernation_ops)
		hibernation_ops->restore_cleanup();
}

/**
 * platform_recover - Recover from a failure to suspend devices.
 * @platform_mode: Whether or not to use the platform driver.
 */
static void platform_recover(int platform_mode)
{
	if (platform_mode && hibernation_ops && hibernation_ops->recover)
		hibernation_ops->recover();
}

/**
 * swsusp_show_speed - Print time elapsed between two events during hibernation.
 * @start: Starting event.
 * @stop: Final event.
 * @nr_pages: Number of memory pages processed between @start and @stop.
 * @msg: Additional diagnostic message to print.
 */
void swsusp_show_speed(struct timeval *start, struct timeval *stop,
			unsigned nr_pages, char *msg)
{
	u64 elapsed_centisecs64;
	unsigned int centisecs;
	unsigned int k;
	unsigned int kps;

	elapsed_centisecs64 = timeval_to_ns(stop) - timeval_to_ns(start);
	/*
	 * If "(s64)elapsed_centisecs64 < 0", it will print long elapsed time,
	 * it is obvious enough for what went wrong.
	 */
	do_div(elapsed_centisecs64, NSEC_PER_SEC / 100);
	centisecs = elapsed_centisecs64;
	if (centisecs == 0)
		centisecs = 1;	/* avoid div-by-zero */
	k = nr_pages * (PAGE_SIZE / 1024);
	kps = (k * 100) / centisecs;
	printk(KERN_INFO "PM: %s %u kbytes in %u.%02u seconds (%u.%02u MB/s)\n",
			msg, k,
			centisecs / 100, centisecs % 100,
			kps / 1000, (kps % 1000) / 10);
}

//+[TCCQB] Making QB_DATA
#define MK_QB_DATA_CMD		"--extract_qb_data_only\n--locale=en_US"
#define RECOVERY_CMD_FILE	"/cache/recovery/command"
#define RECOVERY_CMD_PATH	"/cache/recovery/"
static int qb_data_cmd(void)
{
	int cmd_fd = -1;
	mm_segment_t old_fs_seg;
	int ret = 0;

	old_fs_seg = get_fs();
	set_fs(KERNEL_DS);

	/*	Open kmsg File	*/
	cmd_fd = sys_open(RECOVERY_CMD_FILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (cmd_fd < 0) {
		printk(KERN_INFO "RECOVERY_CMD_FILE open FAILED cmd_fd : %d\n", cmd_fd);
		return -1;
	}

	unsigned int i = 0;
	char *cmd = MK_QB_DATA_CMD;
	for(i = 0; i < strlen(MK_QB_DATA_CMD); i++) {
		sys_write(cmd_fd, cmd++, 1);
	}

	ret = sys_close(cmd_fd);
	if (ret != 0) {
		printk(KERN_INFO "QB_DATA cmd_fd Close ERROR!! %d\n", ret);
		return -1;
	}

	set_fs(old_fs_seg);

	printk(KERN_INFO "write qb_data cmd Complete.\n");

	return 0;
}

static int hibernate_mkdir(char* path) {
    static char * cmdpath = "/system/bin/mkdir";
    char *argv[] = { cmdpath, path, NULL};
    static char *envp[] = { "HOME=/",
                "TERM=linux",
                "PATH=/sbin:/usr/sbin:/bin:/usr/bin",
                NULL };

    return call_usermodehelper(cmdpath, argv, envp, UMH_WAIT_PROC);
}

static int hibernate_mount_cache(void){
	static char *cmdpath = "/system/bin/mount";
	char *argv[] = {cmdpath , "-t", "ext4", "-o", "rw,nosuid,nodev,noatime,errors=panic,data=ordered", "/dev/block/platform/bdm/by-name/cache","/cache",NULL};

	static char *envp[] = { "HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL };

	return call_usermodehelper(cmdpath, argv, envp, UMH_WAIT_PROC);
}

//-[TCCQB]

/*===========================================================================
 *																			*
 * 					 Create QuickBoot Image on Memory.						*
 *																			*
 * ========================================================================*/

/**
 * create_image - Create a hibernation image.
 * @platform_mode: Whether or not to use the platform driver.
 *
 * Execute device drivers' "late" and "noirq" freeze callbacks, create a
 * hibernation image and run the drivers' "noirq" and "early" thaw callbacks.
 *
 * Control reappears in this routine after the subsequent restore.
 */

static int create_image(int platform_mode)
{
	int error;

	printk(KERN_CRIT "============>> %s\n", __func__);

	error = dpm_suspend_end(PMSG_FREEZE);
	if (error) {
		printk(KERN_ERR "PM: Some devices failed to power down, "
			"aborting hibernation\n");
		return error;
	}

	error = platform_pre_snapshot(platform_mode);
	if (error || hibernation_test(TEST_PLATFORM))
		goto Platform_finish;

	if ( hibernation_test(TEST_CPUS))
        goto Platform_finish;
/*
	error = disable_nonboot_cpus();
	if (error || hibernation_test(TEST_CPUS))
		goto Enable_cpus;
*/
	local_irq_disable();

	error = syscore_suspend();
	if (error) {
		printk(KERN_ERR "PM: Some system devices failed to power down, "
			"aborting hibernation\n");
		goto Enable_irqs;
	}

#if defined(CONFIG_PLATFORM_AVN)
	if (hibernation_test(TEST_CORE) || pm_wakeup_pending() || sz_test_enabled)
#else
	if (hibernation_test(TEST_CORE) || pm_wakeup_pending())
#endif
		goto Power_up;

//	in_suspend = 1;
#ifdef CONFIG_SNAPSHOT_BOOT
    /*===========================================================================
	 *     Save context for snapshot boot
	 *=========================================================================*/
	printk(KERN_CRIT " %s : save register state start.\r", __func__);
    snapshot_state_store();
    printk(KERN_CRIT " %s : save register state done. \n", __func__);
	save_processor_state();
	trace_suspend_resume(TPS("machine_suspend"), PM_EVENT_HIBERNATE, true);
    printk(KERN_CRIT " %s : save processor state done. \n", __func__);
    save_cpu_reg_for_snapshot(0x0, reg, restore_snapshot_cpu_reg);
    printk(KERN_CRIT " %s : save cpu register done. \n", __func__);
#else
	error = swsusp_arch_suspend();
#endif
	trace_suspend_resume(TPS("machine_suspend"), PM_EVENT_HIBERNATE, false);

	if (error)
		printk(KERN_ERR "PM: Error %d creating hibernation image\n",
			error);
	/* Restore control flow magically appears here */
#ifdef CONFIG_SNAPSHOT_BOOT
	barrier();
	if (get_do_snapshot_boot() != 0) {
		/*	QuickBoot Booting.	*/
		do_hibernate_boot = 1;	// For tcc_ckc_restore_regs()

		restore_processor_state();
		
		/* Restore control flow magically appears here */
		time_total_snapshot = time_core_resume = cpu_clock(smp_processor_id());

		snapshot_state_restore();

//		in_suspend = 0;
		barrier();
	}
	else {
		/*	Making QuickBoot Image.	*/
		snapshot_state_restore();
		preempt_enable();
	#if defined(CONFIG_ARCH_TCC893X)
		restore_scu_state();
	#endif	/*	#if defined(CONFIG_ARCH_TCC893X)	*/
	}
#endif
	
    printk(KERN_CRIT " %s : restore cp15 state done.  [cpu%d] \n", __func__, smp_processor_id());
	if (!get_in_suspend()) {
		do_hibernate_boot = 1;	// For Hibernate Resume(krenel resume)
		events_check_enabled = false;
	platform_leave(platform_mode);
	}

 Power_up:
	syscore_resume();

 Enable_irqs:
	local_irq_enable();

//Enable_cpus:
//	enable_nonboot_cpus();

 Platform_finish:
	platform_finish(platform_mode);

	time_core_resume = cpu_clock(smp_processor_id()) - time_core_resume;
	time_dpm_resume = cpu_clock(smp_processor_id());

	dpm_resume_start(get_in_suspend() ?
		(error ? PMSG_RECOVER : PMSG_THAW) : PMSG_RESTORE);

	time_dpm_resume =  cpu_clock(smp_processor_id()) - time_dpm_resume;

	return error;
}

/**
 * hibernation_snapshot - Quiesce devices and create a hibernation image.
 * @platform_mode: If set, use platform driver to prepare for the transition.
 *
 * This routine must be called with pm_mutex held.
 */
int hibernation_snapshot(int platform_mode)
{
	pm_message_t msg;
	int error;

	unsigned long long time_tmp;

	error = platform_begin(platform_mode);
	if (error)
		goto Close;

	/* Preallocate image memory before shutting down devices. */
	error = hibernate_preallocate_memory();
	if (error)
		goto Close;

	error = freeze_kernel_threads();
	if (error)
		goto Cleanup;

	if (hibernation_test(TEST_FREEZER)) {

		/*
		 * Indicate to the caller that we are returning due to a
		 * successful freezer test.
		 */
		freezer_test_done = true;
		goto Thaw;
	}

	error = dpm_prepare(PMSG_FREEZE);
	if (error) {
		dpm_complete(PMSG_RECOVER);
		goto Thaw;
	}

	suspend_console();
	pm_restrict_gfp_mask();

	tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
	error = dpm_suspend(PMSG_FREEZE);

	tccwdt_qb_init();		// Re-init QB WatchDog ( suspendig tcc-dwc3(usb30_hclk) makes QB WatchDog doesn't work. )	
	tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
	if (error || hibernation_test(TEST_DEVICES))
		platform_recover(platform_mode);
	else
		error = create_image(platform_mode);

	time_tmp = cpu_clock(smp_processor_id());

	/*
	 * In the case that we call create_image() above, the control
	 * returns here (1) after the image has been created or the
	 * image creation has failed and (2) after a successful restore.
	 */

	/* We may need to release the preallocated image pages here. */
	if (error || !get_in_suspend())
		swsusp_free();

	msg = get_in_suspend() ? (error ? PMSG_RECOVER : PMSG_THAW) : PMSG_RESTORE;


	time_core_resume += cpu_clock(smp_processor_id()) - time_tmp;
	time_tmp = cpu_clock(smp_processor_id());

	tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
	dpm_resume(msg);
	tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog

	time_dpm_resume += cpu_clock(smp_processor_id()) - time_tmp;
	time_tmp = cpu_clock(smp_processor_id());
	
	if (error || !get_in_suspend())
		pm_restore_gfp_mask();


#ifndef CONFIG_PM_VERBOSE_DPM_SUSPEND
	resume_console();
#endif

	time_core_resume += cpu_clock(smp_processor_id()) - time_tmp;
	time_tmp = cpu_clock(smp_processor_id());

	dpm_complete(msg);

	time_dpm_resume += cpu_clock(smp_processor_id()) - time_tmp;
	time_task_resume = cpu_clock(smp_processor_id());

	thaw_kernel_threads(); 

	time_task_resume = cpu_clock(smp_processor_id()) - time_task_resume;

Close:
	time_tmp = cpu_clock(smp_processor_id());
	platform_end(platform_mode);
	time_core_resume += cpu_clock(smp_processor_id()) - time_tmp;
	return error;

 Thaw:
	thaw_kernel_threads();
 Cleanup:
	swsusp_free();
	goto Close;
}

/**
 * resume_target_kernel - Restore system state from a hibernation image.
 * @platform_mode: Whether or not to use the platform driver.
 *
 * Execute device drivers' "noirq" and "late" freeze callbacks, restore the
 * contents of highmem that have not been restored yet from the image and run
 * the low-level code that will restore the remaining contents of memory and
 * switch to the just restored target kernel.
 */
static int resume_target_kernel(bool platform_mode)
{
	int error;

	error = dpm_suspend_end(PMSG_QUIESCE);
	if (error) {
		printk(KERN_ERR "PM: Some devices failed to power down, "
			"aborting resume\n");
		return error;
	}

	error = platform_pre_restore(platform_mode);
	if (error)
		goto Cleanup;

	error = disable_nonboot_cpus();
	if (error)
		goto Enable_cpus;

	local_irq_disable();

	error = syscore_suspend();
	if (error)
		goto Enable_irqs;

	save_processor_state();
	error = restore_highmem();
	if (!error) {
		error = swsusp_arch_resume();
		/*
		 * The code below is only ever reached in case of a failure.
		 * Otherwise, execution continues at the place where
		 * swsusp_arch_suspend() was called.
		 */
		BUG_ON(!error);
		/*
		 * This call to restore_highmem() reverts the changes made by
		 * the previous one.
		 */
		restore_highmem();
	}
	/*
	 * The only reason why swsusp_arch_resume() can fail is memory being
	 * very tight, so we have to free it as soon as we can to avoid
	 * subsequent failures.
	 */
	swsusp_free();
	restore_processor_state();
	touch_softlockup_watchdog();

	syscore_resume();

 Enable_irqs:
	local_irq_enable();

 Enable_cpus:
	enable_nonboot_cpus();

 Cleanup:
	platform_restore_cleanup(platform_mode);

	dpm_resume_start(PMSG_RECOVER);

	return error;
}

/**
 * hibernation_restore - Quiesce devices and restore from a hibernation image.
 * @platform_mode: If set, use platform driver to prepare for the transition.
 *
 * This routine must be called with pm_mutex held.  If it is successful, control
 * reappears in the restored target kernel in hibernation_snapshot().
 */
int hibernation_restore(int platform_mode)
{
	int error;

	pm_prepare_console();
	suspend_console();
	pm_restrict_gfp_mask();
	error = dpm_suspend_start(PMSG_QUIESCE);
	if (!error) {
		error = resume_target_kernel(platform_mode);
		/*
		 * The above should either succeed and jump to the new kernel,
		 * or return with an error. Otherwise things are just
		 * undefined, so let's be paranoid.
		 */
		BUG_ON(!error);
	}
	dpm_resume_end(PMSG_RECOVER);
	pm_restore_gfp_mask();
	resume_console();
	pm_restore_console();
	return error;
}

/**
 * hibernation_platform_enter - Power off the system using the platform driver.
 */
int hibernation_platform_enter(void)
{
	int error;

	if (!hibernation_ops)
		return -ENOSYS;

	/*
	 * We have cancelled the power transition by running
	 * hibernation_ops->finish() before saving the image, so we should let
	 * the firmware know that we're going to enter the sleep state after all
	 */
	error = hibernation_ops->begin();
	if (error)
		goto Close;

	entering_platform_hibernation = true;
	suspend_console();
	error = dpm_suspend_start(PMSG_HIBERNATE);
	if (error) {
		if (hibernation_ops->recover)
			hibernation_ops->recover();
		goto Resume_devices;
	}

	error = dpm_suspend_end(PMSG_HIBERNATE);
	if (error)
		goto Resume_devices;

	error = hibernation_ops->prepare();
	if (error)
		goto Platform_finish;

	error = disable_nonboot_cpus();
	if (error)
		goto Platform_finish;

	local_irq_disable();
	syscore_suspend();
	if (pm_wakeup_pending()) {
		error = -EAGAIN;
		goto Power_up;
	}

	hibernation_ops->enter();
	/* We should never get here */
	while (1);

 Power_up:
	syscore_resume();
	local_irq_enable();
	enable_nonboot_cpus();

 Platform_finish:
	hibernation_ops->finish();

	dpm_resume_start(PMSG_RESTORE);

 Resume_devices:
	entering_platform_hibernation = false;
	dpm_resume_end(PMSG_RESTORE);
	resume_console();

 Close:
	hibernation_ops->end();

	return error;
}

/**
 * power_down - Shut the machine down for hibernation.
 *
 * Use the platform driver, if configured, to put the system into the sleep
 * state corresponding to hibernation, or try to power it off or reboot,
 * depending on the value of hibernation_mode.
 */
#if defined(CONFIG_QUICK_BOOT_PROGRESS_BAR)
extern int fb_quickboot_progress_bar(int percent);
#endif
static void power_down(void)
{
#ifdef CONFIG_SUSPEND
	int error;
#endif

#if defined(CONFIG_QUICK_BOOT_PROGRESS_BAR)
	fb_quickboot_progress_bar(1000);	// progress bar complete.
#endif
	msleep(1000);

	switch (hibernation_mode) {
	case HIBERNATION_REBOOT:
		kernel_restart(NULL);
		break;
	case HIBERNATION_PLATFORM:
		hibernation_platform_enter();
	case HIBERNATION_SHUTDOWN:
		if (pm_power_off)
			kernel_power_off();
		break;
#ifdef CONFIG_SUSPEND
	case HIBERNATION_SUSPEND:
		error = suspend_devices_and_enter(PM_SUSPEND_MEM);
		if (error) {
			if (hibernation_ops)
				hibernation_mode = HIBERNATION_PLATFORM;
			else
				hibernation_mode = HIBERNATION_SHUTDOWN;
			power_down();
		}
		/*
		 * Restore swap signature.
		 */
		error = swsusp_unmark();
		if (error)
			printk(KERN_ERR "PM: Swap will be unusable! "
			                "Try swapon -a.\n");
		return;
#endif
	}
	kernel_halt();
	/*
	 * Valid image is on the disk, if we continue we risk serious data
	 * corruption after resume.
	 */
	printk(KERN_CRIT "PM: Please power down manually\n");
	while (1)
		cpu_relax();
}


#if defined(CONFIG_ANDROID)
int set_hibernate_boot_property(void) {
	static char * path = "/system/bin/setprop";
	char *argv[] = { path, QBMANAGER_BOOT_WITH, "snapshot", NULL};
	static char *envp[] = { "HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL };

	return call_usermodehelper(path, argv, envp, UMH_WAIT_PROC);
}

int set_hibernate_swap_path_property(void) {
	static char * path = "/system/bin/setprop";

	char *argv[] = { path, QBMANAGER_SWAP_PATH, resume_file, NULL};

	static char *envp[] = { "HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL };

	return call_usermodehelper(path, argv, envp, UMH_WAIT_PROC);
}
#endif

int hibernate_mkswap(char* path) {
#if defined(CONFIG_ANDROID)
	static char * cmdpath = "/system/bin/mkswap";
#else
	static char * cmdpath = "/bin/mkswap";
#endif
	char *argv[] = { cmdpath, path, NULL};
	static char *envp[] = { "HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL };

	return call_usermodehelper(cmdpath, argv, envp, UMH_WAIT_PROC);
}

int hibernate_swapon(char* path) {
#if defined(CONFIG_ANDROID)
	static char * cmdpath = "/system/bin/swapon";
#else
	static char * cmdpath = "/bin/swapon";
#endif
	char *argv[] = { cmdpath, path, NULL};
	static char *envp[] = { "HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL };

	return call_usermodehelper(cmdpath, argv, envp, UMH_WAIT_PROC);
}

int hibernate_swapoff(char* path) {
#if defined(CONFIG_ANDROID)
	static char * cmdpath = "/system/bin/swapoff";
#else
	static char * cmdpath = "/bin/swapoff";
#endif
	char *argv[] = { cmdpath, path, NULL};
	static char *envp[] = { "HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL };

	return call_usermodehelper(cmdpath, argv, envp, UMH_WAIT_PROC);
}

#if defined(CONFIG_ANDROID)
static int slogi(char* message) {
	static char * cmdpath = "/system/bin/log";
	char *argv[] = { cmdpath, message, NULL};
	static char *envp[] = { "HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL };

	return call_usermodehelper(cmdpath, argv, envp, UMH_WAIT_PROC);
}

static int hibernate_e2fsck(char* path){
	static char *cmdpath = "/system/bin/e2fsck";
#if defined(CONFIG_DAUDIO_KK)
	char *argv[] = {cmdpath , "-p", path, NULL};
#else
	char *argv[] = {cmdpath , "-y", path, NULL};
#endif

	static char *envp[] = { "HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL };

	return call_usermodehelper(cmdpath, argv, envp, UMH_WAIT_PROC);
}

static int qb_runtime(char* sel_func, char* arg_1){
	static char *cmdpath = "/system/bin/sh";
	char *argv[] = {cmdpath , "/system/etc/qb_runtime.sh", sel_func, arg_1, NULL};

	static char *envp[] = { "HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL };

	return call_usermodehelper(cmdpath, argv, envp, UMH_WAIT_PROC);
}



/*		Check if system partition and snapshot is matched or not.		*/
#define QB_VER_CHECK_BEFORE	"/system/etc/qb_ver_before.dat"
#define QB_VER_CHECK_AFTER	"/system/etc/qb_ver_after.dat"
#define QB_VER_BUF_SIZE		512
char *read_qb_ver(char *buf, char *file)
{
    int sz;
    int fd;
	mm_segment_t old_fs_seg;

	old_fs_seg = get_fs();
	set_fs(KERNEL_DS);

    fd = sys_open(file, O_RDONLY, 0);
    if(fd < 0)  
		return NULL;

    sz = sys_lseek(fd, 0, SEEK_END);
    if(sz < 0) 
		goto oops;

	if(sz >= QB_VER_BUF_SIZE) 
		sz = QB_VER_BUF_SIZE -1;

    if(sys_lseek(fd, 0, SEEK_SET) != 0) 
		goto oops;

    if(sys_read(fd, buf, sz) != sz) 
		goto oops;

	buf[sz] = '\0';
   
	sys_close(fd);
	set_fs(old_fs_seg);
    return buf;

oops:
    sys_close(fd);
	set_fs(old_fs_seg);
	return NULL;
}
int get_before_qb_ver(char *be_buf)
{
	char *before_qb_ver = NULL;

	before_qb_ver = read_qb_ver(be_buf, QB_VER_CHECK_BEFORE);
	if (before_qb_ver == NULL) {
		printk(KERN_INFO "Failed to read Before QB VER file.\n");
		return -1;
	} else {
		printk(KERN_INFO "Before QB VER : %s\n", before_qb_ver);
	}
	return 0;
}
int check_qb_ver(char *before_qb_ver, char *af_buf)
{
	char *after_qb_ver = NULL;

	after_qb_ver = read_qb_ver(af_buf, QB_VER_CHECK_AFTER);
	if (after_qb_ver == NULL) {
		printk(KERN_INFO "Failed to read After QB VER file.\n");
		goto unmatch;
	} else if (*before_qb_ver == '\0') {
		/*	if data is not exist in before_qb_ver,		*/
		printk(KERN_INFO "Failed to get Before QB VER from Memory.\n");
		goto unmatch;
	} else {
		if (strlen(before_qb_ver) != strlen(after_qb_ver)) 
			goto unmatch;
		if (strncmp(before_qb_ver, after_qb_ver, strlen(after_qb_ver))) 
			goto unmatch;
		printk(KERN_INFO "After QB VER : %s\n", after_qb_ver);
		printk(KERN_INFO "QB: snapshot and system is matched.\n");
		return 0;
	}

unmatch:
	printk(KERN_INFO "Befor_QB_VER : %s\n", before_qb_ver);
	printk(KERN_INFO "After_QB_VER : %s\n", after_qb_ver);
	printk(KERN_INFO "snapshot & system is not matched.\n");
	printk(KERN_INFO "This system has potential problems.\n");
	return -2;
}


/*		Un-Mount Partitions for QuickBoot.		*/
#define UNMOUNT_TRY		10
int qb_umount(char *um_path)
{
	int umount_cnt;
	int error;
	struct mount *mount;
	struct path path;

	for(umount_cnt = 0; umount_cnt < UNMOUNT_TRY; umount_cnt++) {
		tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog

		if (umount_cnt != 0) {
			printk(" PM: Waiting to finish un-mount %s... [%d]\n", um_path, umount_cnt);
			msleep(1000);
			tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
		}

		printk(KERN_INFO "PM: un-mount %s ...\n", um_path);
		error = qb_runtime("unmount", um_path);
        if(error){
	        printk(" PM: Can't do umount %s... [%d] \n\n", um_path, error);
			continue;
        }
		return 0;
	}

	printk(" PM: Failed to unmount %s ...\n", um_path);

	return -1;
}
#else 
static int quickboot_rcS(void) {
    printk(" ___________ quickboot_rcS ___________ \n");
	static char * cmdpath = "/sbin/init-snapshot";
	char *argv[] = { cmdpath,NULL};
	static char *envp[] = { "HOME=/",
		"TERM=linux",
		"PATH=/sbin:/usr/sbin:/bin:/usr/bin",
		NULL };

	return call_usermodehelper(cmdpath, argv, envp, UMH_WAIT_EXEC);
}
#endif

/**
 * hibernate - Carry out system hibernation, including saving the image.
 */
extern int fb_hibernation_enter(void);

int hibernate(void)
{
	int error;
#if defined(CONFIG_ANDROID)
	unsigned long long start_e2fsck;
	unsigned long long stop_e2fsck;
	unsigned long long e2fsck_time;
#endif
	unsigned long long time_tmp;

	if (!hibernation_available()) {
		pr_debug("PM: Hibernation not available.\n");
		return -EPERM;
	}
	
#if defined(CONFIG_ANDROID)
	char before_buf[QB_VER_BUF_SIZE];
	char after_buf[QB_VER_BUF_SIZE];
	get_before_qb_ver(before_buf);	// get Before QB VER
#endif

	tccwdt_qb_init();	// Init QuickBoot WatchDog
	
	char message[512];

	lock_system_sleep();

	set_in_suspend(1);
#ifdef CONFIG_SNAPSHOT_BOOT
	set_do_snapshot_boot(0);
#endif

	/* The snapshot device should not be opened while we're running */
	if (!atomic_add_unless(&snapshot_device_available, -1, 0)) {
		error = -EBUSY;
		goto Unlock;
	}

	pm_prepare_console();

	error = pm_notifier_call_chain(PM_HIBERNATION_PREPARE);
	if (error)
		goto Enable_cpus;

	error = disable_nonboot_cpus();
	if (error)
		goto Enable_cpus;


#if defined(CONFIG_ANDROID)
	set_hibernate_swap_path_property();
#endif
	printk(KERN_INFO "PM: Syncing filesystems ... \n");
	sys_sync();
	sys_sync();
	sys_sync();

#if defined(CONFIG_ANDROID)
	/*		Un-Mount Partitions		*/
	int idx_list;
	for(idx_list = 0; idx_list < sizeof(qb_um_list)/sizeof(qb_um_list[0]); idx_list++) {
		if(qb_umount(qb_um_list[idx_list])) {
			qb_runtime("attached_logs", qb_um_list[idx_list]);	// Show the opened file list.
			goto Enable_cpus;
		}
	}
#endif

	tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
	printk("done.\n");

	lock_device_hotplug();
	/* Allocate memory management structures */
	error = create_basic_memory_bitmaps();
	if (error)
		goto Enable_cpus;

	printk(KERN_INFO "PM: Syncing filesystems ... ");
	sys_sync();
	printk("done.\n");

	error = freeze_processes();
	if (error)
		goto Free_bitmaps;


	#ifdef CONFIG_QUICK_BOOT_LOGO
		fb_hibernation_enter();
	#endif

	tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
	error = hibernation_snapshot(hibernation_mode == HIBERNATION_PLATFORM);
	tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
	if (error || freezer_test_done) {
		printk("failed hibernation_snapshot! error = %d\n", error);
		goto Thaw;
	}

#if defined(CONFIG_PLATFORM_AVN)
    /* Check swap size */
    if (sz_test_enabled)
    {
	    unsigned long pages;
		unsigned int flags = 0;
        int ret;

		usermodehelper_enable();	// Originally it was in thaw_processes();

		error = hibernate_mkswap(resume_file);
        printk(KERN_CRIT "hibernate_mkswap(%s) %d\n", resume_file, error);

		error = hibernate_swapon(resume_file);
		printk(KERN_CRIT "hibernate_swapon(%s) %d\n", resume_file, error);

		if (hibernation_mode == HIBERNATION_PLATFORM)
			flags |= SF_PLATFORM_MODE;
		if (nocompress)
			flags |= SF_NOCOMPRESS_MODE;
		else
			flags |= SF_CRC32_MODE;	// USE Telechips's CheckSum Algorithm.

	    pages = required_pages();

        ret = swsusp_swap_check();
        if (ret) {
            if (ret != -ENOSPC)
                printk(KERN_ERR "PM: Cannot find swap device, try " "swapon -a.\n");
        }
        if (!enough_swap(pages, flags)) {
            printk("PM: Not enough free swap\n");
        } else {
            printk("PM: Enough free swap\n");
        }

        goto Thaw;
    }
#endif
	if (get_in_suspend()) {
		unsigned int flags = 0;

		usermodehelper_enable();	// Originally it was in thaw_processes();
		enable_nonboot_cpus();		// Use multi thread for compressing snapshot.

		tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
		thaw_flush_kernel_threads();	// thaw 'flush' tasks.
		tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog

        printk(KERN_CRIT "swap partition path[%s]\n", resume_file);

		error = hibernate_mkswap(resume_file);
        printk(KERN_CRIT "hibernate_mkswap(%s) error: %d\n", resume_file, error);

		error = hibernate_swapon(resume_file);
		printk(KERN_CRIT "hibernate_swapon(%s) error: %d\n", resume_file, error);

		if (hibernation_mode == HIBERNATION_PLATFORM)
			flags |= SF_PLATFORM_MODE;
		if (nocompress)
			flags |= SF_NOCOMPRESS_MODE;
		else
		        flags |= SF_CRC32_MODE;	// In QuickBoot, USE Telechips's CheckSum Algorithm than CRC32.

		pr_debug("PM: writing image.\n");
		error = swsusp_write(flags);
		swsusp_free();
		
		sys_sync();
		sys_sync();
		sys_sync();
		tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog

//+[TCCQB] Making QB_DATA
		error = hibernate_mount_cache();
		if(error)
			printk(KERN_INFO "QB : hibernate_mount_cache error=0x%x\n", error);
			
		hibernate_mkdir(RECOVERY_CMD_PATH);	// mkdir recovery command file path
		error = qb_data_cmd();	// Making QB_DATA command
		if(error)
			printk(KERN_INFO "QB : /qb_data_cmd end error = 0x%x\n", error);
		tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
		sys_sync();

//+[TCCQB] QB_kmsg
		tccwdt_qb_exit();       // Exit QuickBoot WatchDog
		thaw_processes();

		printk(KERN_INFO "QB : qb_kmsg log finished and start to write log file.\n"); // don't edit this log becuase qb_kmsg check the string

		qb_runtime("qb_kmsg", NULL);
//-[TCCQB] QB_kmsg

		if (!error)
			//power_down();
		{
			handle_sysrq('u'); //remount ro
			kernel_restart("recovery"); //reboot into recovery
		}
//-[TCCQB]
		pm_restore_gfp_mask();
	} else {
		pr_debug("PM: Image restored successfully.\n");
		
		usermodehelper_enable();
		enable_nonboot_cpus();		// Use multi thread for compressing snapshot.

		time_tmp = cpu_clock(smp_processor_id());
		thaw_flush_kernel_threads();	// thaw 'flush' tasks.
		time_task_resume = cpu_clock(smp_processor_id()) - time_tmp;

		do_hibernate_boot = 0;

		time_checkdisk = cpu_clock(smp_processor_id());

		tccwdt_qb_reset_time(40);	// Change WatchDog Reset Time for CheckDisk.
		tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
#ifdef CONFIG_ANDROID
		start_e2fsck = cpu_clock(smp_processor_id());

		hibernate_e2fsck("/dev/block/platform/bdm/by-name/cache");
		stop_e2fsck = cpu_clock(smp_processor_id());
		e2fsck_time = (stop_e2fsck - start_e2fsck);
		do_div(e2fsck_time, 1000000);

		printk("Cache Partition CheckDisk Time : %lldms \n", e2fsck_time);
//		error = do_mount("/dev/block/platform/bdm/by-num/p5", "/cache", "ext4", MS_NOATIME | MS_NOSUID | MS_NODEV, data);

		
		tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
//		start_e2fsck = cpu_clock(smp_processor_id());
//		hibernate_e2fsck("/dev/block/platform/bdm/by-name/data");
//		stop_e2fsck = cpu_clock(smp_processor_id());
//		e2fsck_time = (stop_e2fsck - start_e2fsck);
//	do_div(e2fsck_time, 1000000);

		printk("UserData Partition CheckDisk Time : %lldms \n", e2fsck_time);
//		error = do_mount("/dev/block/platform/bdm/by-num/p3", "/data", "ext4", MS_NOATIME | MS_NOSUID | MS_NODEV, data);
#else
//		qb_runtime("e2fsck_log", NULL);	// e2fsck with log.
#endif

		tccwdt_qb_reset_time(0);	// Change WatchDog Reset Time to Default.
		tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
		time_checkdisk = cpu_clock(smp_processor_id()) - time_checkdisk;

		do_hibernation = 0;
	}

 Thaw:
	unlock_device_hotplug();
	usermodehelper_disable(); // It will enabled in thaw_processes();

	time_tmp = cpu_clock(smp_processor_id());
	thaw_processes();
	time_task_resume = cpu_clock(smp_processor_id()) - time_tmp;


	/* Don't bother checking whether freezer_test_done is true */
	freezer_test_done = false;

 Free_bitmaps:
	free_basic_memory_bitmaps();
Enable_cpus:
	enable_nonboot_cpus();
 Exit:
	pm_notifier_call_chain(PM_POST_HIBERNATION);
	pm_restore_console();
	atomic_inc(&snapshot_device_available);
 Unlock:
	tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
	unlock_system_sleep();

#if defined(CONFIG_ANDROID)
	check_qb_ver(before_buf, after_buf);	// Check if snapshot & system is matched.
#endif

	tccwdt_qb_kickdog();    // Kick QuickBoot WatchDog
	if( get_in_suspend() == 0 ) {
		time_total_snapshot = cpu_clock(smp_processor_id()) - time_total_snapshot;
		frameworks_boot_time = cpu_clock(0);	// Kernel Boot End Time. ( CORE 0 base )

#if defined(CONFIG_ANDROID)
        error = set_hibernate_boot_property();	// Set "snapshot" property.
#endif

		do_div(time_total_snapshot, 1000000);
		do_div(time_core_resume, 1000000);
		do_div(time_dpm_resume, 1000000);
		do_div(time_task_resume, 1000000);
		do_div(time_checkdisk, 1000000);
		time_etc_resume = time_total_snapshot - time_core_resume - time_dpm_resume - time_task_resume - time_checkdisk;

		sprintf(message, "QuickBoot Kernel Total[%lldms] = Core resume[%lldms] + Device resume[%lldms] + Task resume[%lldms] + CheckDisk[%lldms] + Etc[%lldms]\n", time_total_snapshot, time_core_resume, time_dpm_resume, time_task_resume, time_checkdisk, time_etc_resume);

#if defined(CONFIG_ANDROID)
		slogi(message);
#else
        quickboot_rcS();
#endif
#if defined(CONFIG_PLATFORM_AVN)
        if (!sz_test_enabled)
#endif
		printk(KERN_ERR "\x1b[1;32m%s\x1b[0m", message);
	} else {
#if defined(CONFIG_PLATFORM_AVN)
        if (!sz_test_enabled)
#endif
        {
		printk("\x1b[1;31mPM: Failed to Making QuickBoot Image!\x1b[0m\n");
		printk("\x1b[1;31mReboot with Normal boot.\x1b[0m\n");
		power_down();
        }
#if defined(CONFIG_PLATFORM_AVN)
        else {
		printk("\x1b[1;31mTest Completed, Running with Normal boot.\x1b[0m\n");
        }
#endif
	}

	/*      Get LK total time & Send LK, Kernel Total Time to Frameworks    */
	u32 usts = 0;
	void __iomem *pmu_reg = (void __iomem *)io_p2v(TCC_PA_PMU);

	usts = read_usstatus();             // get LK's Time.
	lk_boot_time = usts & 0x00ffffff;   // LK Boot Time.
	kernel_boot_time = (int)time_total_snapshot;	// Kernel Boot Time.

	usts &= 0xff000000;
	writel(usts, pmu_reg+PMU_USSTATUS);  //  Set PMU_USSTATUS to 0.

	tccwdt_qb_exit();	// Exit QuickBoot WatchDog
#ifdef CONFIG_LK_DEBUG_LOG_BUF
	//resume time, do not print lk log
	//lk_log_print_show();
#endif // CONFIG_LK_DEBUG_LOG_BUF
	return error;
}


/**
 * software_resume - Resume from a saved hibernation image.
 *
 * This routine is called as a late initcall, when all devices have been
 * discovered and initialized already.
 *
 * The image reading code is called to see if there is a hibernation image
 * available for reading.  If that is the case, devices are quiesced and the
 * contents of memory is restored from the saved image.
 */
void *read_file(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    int fd;

    data = 0;
    fd = sys_open(fn, O_RDONLY, 0);
    if(fd < 0) return 0;

    sz = sys_lseek(fd, 0, SEEK_END);
    if(sz < 0) goto oops;

    if(sys_lseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) vmalloc(sz + 2);
    //data = (char*) vmalloc(sz + 2);
    if(data == 0) goto oops;

    if(sys_read(fd, data, sz) != sz) goto oops;
    sys_close(fd);
    data[sz] = '\n';
    data[sz+1] = 0;
    if(_sz) *_sz = sz;
    return data;

oops:
    sys_close(fd);
    if(data != 0) vfree(data);
    return 0;
}

static int insmod(const char *filename)
{
    void *module;
    void __user *umodule;
    void __user *uoptions;
    char options[2];
    unsigned size;
    int ret;
	unsigned long ret_copy_to_user;

    options[0] = '\0';
    module = read_file(filename, &size);
    if (!module)
        return -1;

    umodule =  kmalloc(size + 2, GFP_USER);
    
	ret_copy_to_user = copy_to_user(umodule, module, size + 2);
    vfree(module);

    uoptions =  kmalloc(2, GFP_USER);
    ret_copy_to_user = copy_to_user(uoptions, options, 2);
	
    ret =sys_init_module(umodule, size, uoptions);

    kfree(umodule);
    kfree(uoptions);

    return ret;
}

#include <linux/module.h>
int rmmod(const char *module_name)
{
    char module[MODULE_NAME_LEN];
	void __user *umodule;
	void __user *uoptions;
	char options[2];
	unsigned int size;
	int ret;
	unsigned long ret_copy_to_user;

	options[0] = '\0';

	memset(module, 0, MODULE_NAME_LEN);
	strcpy(module, module_name);
	size = (unsigned int)strlen(module);

	umodule =  kmalloc(size + 2, GFP_USER);
	ret_copy_to_user = copy_to_user(umodule, module, size + 2);

	/* pass it to the kernel */
	ret = sys_delete_module(umodule, O_RDONLY);//O_TRUNC);

	kfree(umodule);

	return ret;
}

/*
 * If this is successful, control reappears in the restored target kernel in
 * hibernation_snaphot() which returns to hibernate().  Otherwise, the routine
 * attempts to recover gracefully and make the kernel return to the normal mode
 * of operation.
 */
static int software_resume(void)
{
	int error;
	unsigned int flags;

	pr_info("PM: %s \n", __func__);
#ifdef CONFIG_SNAPSHOT_BOOT
	return -EPERM;

	/*
	 * If the user said "noresume".. bail out early.
	 */
	if (noresume || !hibernation_available())
		return 0;
#endif

	printk("\x1b[1;35mPM: %s! resume_file[%s]\x1b[0m\n", __func__, resume_file);
/*
	if( !strncmp(resume_file, "/dev/block/mtd", 14)  || !strncmp(resume_file, "/dev/block/ndd", 14)) 
	{
		if(error = insmod("/lib/modules/tcc_nand_core.ko"))
			pr_info("load_nand_core_module result = %d\n", error);

		if(error = insmod("/lib/modules/tcc_nand.ko"))
			pr_info("load_nand_module result = %d\n", error);
		
//		error = insmod("/lib/modules/tcc_mtd.ko");
//		pr_info("load_mtd_module result = %d\n", error);
	}
*/

	/*
	 * name_to_dev_t() below takes a sysfs buffer mutex when sysfs
	 * is configured into the kernel. Since the regular hibernate
	 * trigger path is via sysfs which takes a buffer mutex before
	 * calling hibernate functions (which take pm_mutex) this can
	 * cause lockdep to complain about a possible ABBA deadlock
	 * which cannot happen since we're in the boot code here and
	 * sysfs can't be invoked yet. Therefore, we use a subclass
	 * here to avoid lockdep complaining.
	 */
	mutex_lock_nested(&pm_mutex, SINGLE_DEPTH_NESTING);

	if (swsusp_resume_device)
		goto Check_image;

	if (!strlen(resume_file)) {
		error = -ENOENT;
		goto Unlock;
	}

/*
 * In default, resume device path is using symbolic link.
 * However, symbolic link is not useful here.
 * So, you should check SNAPSHOT PARTITON DEVICE PATH and HARDCODING IT.
 * Ex) /dev/block/mmcblk0p9  <- It is a SNAPSHOT partition dev file.
 * So, define SNAPSHOT_PATH.
 */
#if !defined(CONFIG_DAUDIO_KK)
#define SNAPSHOT_PATH	"/dev/block/mmcblk0p9"
#else
#define SNAPSHOT_PATH	"/dev/block/mmcblk0p10"
#endif
	strcpy(resume_file, SNAPSHOT_PATH);		// Add for QB TEST
	resume_wait = 1;
	printk("PM: Checking hibernation image partition %s\n", resume_file);
	//pr_debug("PM: Checking hibernation image partition %s\n", resume_file);

	if (resume_delay) {
		printk(KERN_INFO "Waiting %dsec before reading resume device...\n",
			resume_delay);
		ssleep(resume_delay);
	}

	/* Check if the device is there */
	swsusp_resume_device = name_to_dev_t(resume_file);

	/*
	 * name_to_dev_t is ineffective to verify parition if resume_file is in
	 * integer format. (e.g. major:minor)
	 */
	if (isdigit(resume_file[0]) && resume_wait) {
		int partno;
		while (!get_gendisk(swsusp_resume_device, &partno))
			msleep(10);
	}

	if (!swsusp_resume_device) {
		/*
		 * Some device discovery might still be in progress; we need
		 * to wait for this to finish.
		 */
		wait_for_device_probe();

		if (resume_wait) {
			while ((swsusp_resume_device = name_to_dev_t(resume_file)) == 0)
				msleep(10);
			async_synchronize_full();
		}

		swsusp_resume_device = name_to_dev_t(resume_file);
		if (!swsusp_resume_device) {
			error = -ENODEV;
			goto Unlock;
		}
	}

 Check_image:
	pr_debug("PM: Hibernation image partition %d:%d present\n",
		MAJOR(swsusp_resume_device), MINOR(swsusp_resume_device));

	pr_debug("PM: Looking for hibernation image.\n");
	error = swsusp_check();
	if (error) {
		printk("\x1b[1;35mPM: hibernation image is invalid or empty.\x1b[0m\n");
		goto Unlock;
	}

	/* The snapshot device should not be opened while we're running */
	if (!atomic_add_unless(&snapshot_device_available, -1, 0)) {
		error = -EBUSY;
		swsusp_close(FMODE_READ);
		goto Unlock;
	}

	pm_prepare_console();
	error = pm_notifier_call_chain(PM_RESTORE_PREPARE);
	if (error)
		goto Close_Finish;

	error = create_basic_memory_bitmaps();
	if (error)
		goto Close_Finish;

	pr_debug("PM: Preparing processes for restore.\n");
	error = freeze_processes();
	if (error) {
		swsusp_close(FMODE_READ);
		goto Close_Finish;
	}

	pr_debug("PM: Loading hibernation image.\n");

	lock_device_hotplug();
	error = create_basic_memory_bitmaps();
	if (error)
		goto Thaw;

	error = swsusp_read(&flags);
	swsusp_close(FMODE_READ);

	/*	Remove nand modules 	*/
/*
	if( !strncmp(resume_file, "/dev/block/mtd", 14)  || !strncmp(resume_file, "/dev/block/ndd", 14)) 
	{
//		if(error = rmmod("tcc_nand"))
//			pr_info("Remove nand_module result = %d\n", error);

//		if(error = rmmod("tcc_nand_core"))
//			pr_info("Remove nand_core_module result = %d\n", error);
		
//		if(error = insmod("/lib/modules/tcc_nand.ko"))
//			pr_info("load_nand_module result = %d\n", error);
	}
*/
	if (!error)
		hibernation_restore(flags & SF_PLATFORM_MODE);

	printk(KERN_ERR "PM: Failed to load hibernation image, recovering.\n");
	swsusp_free();
	free_basic_memory_bitmaps();
 Thaw:
	unlock_device_hotplug();
	thaw_processes();
 Finish:
	pm_notifier_call_chain(PM_POST_RESTORE);
	pm_restore_console();
	atomic_inc(&snapshot_device_available);
	/* For success case, the suspend path will release the lock */
 Unlock:
	mutex_unlock(&pm_mutex);
	pr_debug("PM: Hibernation image not present or could not be loaded.\n");
	return error;
 Close_Finish:
	swsusp_close(FMODE_READ);
	goto Finish;
}

late_initcall_sync(software_resume);


static const char * const hibernation_modes[] = {
	[HIBERNATION_PLATFORM]	= "platform",
	[HIBERNATION_SHUTDOWN]	= "shutdown",
	[HIBERNATION_REBOOT]	= "reboot",
#ifdef CONFIG_SUSPEND
	[HIBERNATION_SUSPEND]	= "suspend",
#endif
};

/*
 * /sys/power/disk - Control hibernation mode.
 *
 * Hibernation can be handled in several ways.  There are a few different ways
 * to put the system into the sleep state: using the platform driver (e.g. ACPI
 * or other hibernation_ops), powering it off or rebooting it (for testing
 * mostly).
 *
 * The sysfs file /sys/power/disk provides an interface for selecting the
 * hibernation mode to use.  Reading from this file causes the available modes
 * to be printed.  There are 3 modes that can be supported:
 *
 *	'platform'
 *	'shutdown'
 *	'reboot'
 *
 * If a platform hibernation driver is in use, 'platform' will be supported
 * and will be used by default.  Otherwise, 'shutdown' will be used by default.
 * The selected option (i.e. the one corresponding to the current value of
 * hibernation_mode) is enclosed by a square bracket.
 *
 * To select a given hibernation mode it is necessary to write the mode's
 * string representation (as returned by reading from /sys/power/disk) back
 * into /sys/power/disk.
 */

static ssize_t disk_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	int i;
	char *start = buf;

	if (!hibernation_available())
		return sprintf(buf, "[disabled]\n");

	for (i = HIBERNATION_FIRST; i <= HIBERNATION_MAX; i++) {
		if (!hibernation_modes[i])
			continue;
		switch (i) {
		case HIBERNATION_SHUTDOWN:
		case HIBERNATION_REBOOT:
#ifdef CONFIG_SUSPEND
		case HIBERNATION_SUSPEND:
#endif
			break;
		case HIBERNATION_PLATFORM:
			if (hibernation_ops)
				break;
			/* not a valid mode, continue with loop */
			continue;
		}
		if (i == hibernation_mode)
			buf += sprintf(buf, "[%s] ", hibernation_modes[i]);
		else
			buf += sprintf(buf, "%s ", hibernation_modes[i]);
	}
	buf += sprintf(buf, "\n");
	return buf-start;
}

static ssize_t disk_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t n)
{
	int error = 0;
	int i;
	int len;
	char *p;
	int mode = HIBERNATION_INVALID;

	if (!hibernation_available())
		return -EPERM;

	p = memchr(buf, '\n', n);
	len = p ? p - buf : n;

	lock_system_sleep();
	for (i = HIBERNATION_FIRST; i <= HIBERNATION_MAX; i++) {
		if (len == strlen(hibernation_modes[i])
		    && !strncmp(buf, hibernation_modes[i], len)) {
			mode = i;
			break;
		}
	}
	if (mode != HIBERNATION_INVALID) {
		switch (mode) {
		case HIBERNATION_SHUTDOWN:
		case HIBERNATION_REBOOT:
#ifdef CONFIG_SUSPEND
		case HIBERNATION_SUSPEND:
#endif
			hibernation_mode = mode;
			break;
		case HIBERNATION_PLATFORM:
			if (hibernation_ops)
				hibernation_mode = mode;
			else
				error = -EINVAL;
		}
	} else
		error = -EINVAL;

	if (!error)
		pr_debug("PM: Hibernation mode set to '%s'\n",
			 hibernation_modes[mode]);
	unlock_system_sleep();
	return error ? error : n;
}

power_attr(disk);

static ssize_t resume_show(struct kobject *kobj, struct kobj_attribute *attr,
			   char *buf)
{
	return sprintf(buf,"%d:%d\n", MAJOR(swsusp_resume_device),
		       MINOR(swsusp_resume_device));
}

static ssize_t resume_store(struct kobject *kobj, struct kobj_attribute *attr,
			    const char *buf, size_t n)
{
	dev_t res;
	int len = n;
	char *name;

	if (len && buf[len-1] == '\n')
		len--;
	name = kstrndup(buf, len, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	res = name_to_dev_t(name);
	kfree(name);
	if (!res)
		return -EINVAL;

	lock_system_sleep();
	swsusp_resume_device = res;
	unlock_system_sleep();
	printk(KERN_INFO "PM: Starting manual resume from disk\n");
	noresume = 0;
	software_resume();
	return n;
}

power_attr(resume);

static ssize_t image_size_show(struct kobject *kobj, struct kobj_attribute *attr,
			       char *buf)
{
	return sprintf(buf, "%lu\n", image_size);
}

static ssize_t image_size_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned long size;

	if (sscanf(buf, "%lu", &size) == 1) {
		image_size = size;
		return n;
	}

	return -EINVAL;
}

power_attr(image_size);

static ssize_t reserved_size_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", reserved_size);
}

static ssize_t reserved_size_store(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t n)
{
	unsigned long size;

	if (sscanf(buf, "%lu", &size) == 1) {
		reserved_size = size;
		return n;
	}

	return -EINVAL;
}

power_attr(reserved_size);

#if defined(CONFIG_PLATFORM_AVN)
static ssize_t snapshot_size_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", snapshot_size);
}

static ssize_t snapshot_size_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	return -EINVAL;
}

power_attr(snapshot_size);

static ssize_t free_swap_size_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", free_swap_size);
}

static ssize_t free_swap_size_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	return -EINVAL;
}

power_attr(free_swap_size);

static ssize_t required_swap_size_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", required_swap_size);
}

static ssize_t required_swap_size_store(struct kobject *kobj,
		struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	return -EINVAL;
}

power_attr(required_swap_size);
#endif

static struct attribute * g[] = {
	&disk_attr.attr,
	&resume_attr.attr,
	&image_size_attr.attr,
	&reserved_size_attr.attr,
#if defined(CONFIG_PLATFORM_AVN)
    &snapshot_size_attr.attr,
	&free_swap_size_attr.attr,
	&required_swap_size_attr.attr,
#endif
	NULL,
};


static struct attribute_group attr_group = {
	.attrs = g,
};


static int __init pm_disk_init(void)
{
	return sysfs_create_group(power_kobj, &attr_group);
}

core_initcall(pm_disk_init);


static int __init resume_setup(char *str)
{
	if (noresume)
		return 1;

	strncpy( resume_file, str, 255 );
	return 1;
}

static int __init resume_offset_setup(char *str)
{
	unsigned long long offset;

	if (noresume)
		return 1;

	if (sscanf(str, "%llu", &offset) == 1)
		swsusp_resume_block = offset;

	return 1;
}

static int __init hibernate_setup(char *str)
{
	if (!strncmp(str, "noresume", 8))
		noresume = 1;
	else if (!strncmp(str, "nocompress", 10))
		nocompress = 1;
	else if (!strncmp(str, "no", 2)) {
		noresume = 1;
		nohibernate = 1;
	}
	return 1;
}

static int __init noresume_setup(char *str)
{
	noresume = 1;
	return 1;
}

static int __init resumewait_setup(char *str)
{
	resume_wait = 1;
	return 1;
}

static int __init resumedelay_setup(char *str)
{
	int rc = kstrtouint(str, 0, &resume_delay);

	if (rc)
		return rc;
	return 1;
}

static int __init nohibernate_setup(char *str)
{
	noresume = 1;
	nohibernate = 1;
	return 1;
}

static int __init kaslr_nohibernate_setup(char *str)
{
	return nohibernate_setup(str);
}

__setup("noresume", noresume_setup);
__setup("resume_offset=", resume_offset_setup);
__setup("resume=", resume_setup);
__setup("hibernate=", hibernate_setup);
__setup("resumewait", resumewait_setup);
__setup("resumedelay=", resumedelay_setup);
__setup("nohibernate", nohibernate_setup);
__setup("kaslr", kaslr_nohibernate_setup);
