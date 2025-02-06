#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/tty.h>
#include <asm/termios.h>
#include <linux/proc_fs.h>
#include "ioctl_duptty_function.h"
#include <linux/delay.h>

#define MODULE_NAME "dupttyd"

#define dtd_dbg(fmt, args...) \
	pr_debug("%s: "fmt, MODULE_NAME, ##args)
#define dtd_info(fmt, args...) \
	pr_info("%s: "fmt, MODULE_NAME, ##args)
#define dtd_err(fmt, args...) \
	pr_err("%s: "fmt, MODULE_NAME, ##args)

#define DEVICE_PATH "/dev/ttyTCC3"

#define PROCFS_NBUFFERS (12)
#define PROCFS_FILE_NAME "options"
#define PROCFS_OPTIONS_STRING "115200N81" /* the default control options */

#define DTD_INIT_C_CC \
	"\003\034\177\025\004\1\1\0\021\023\032\0\022\017\027\026\0"

static struct ktermios dtd_init_ktermios = {
	.c_iflag = IGNBRK | IGNPAR,
	.c_oflag = 0,
	.c_cflag = CREAD | CLOCAL, /* the remaning default control options */
	.c_lflag = 0,
	.c_cc = DTD_INIT_C_CC,
	.c_ispeed = 115200,
    .c_ospeed = 115200
};

typedef struct dup_serial_config ds_conf_t;

static const ds_conf_t ds_config = {
        .major = 207,
        .minor = 0,
        .nr = 3,
		.dev_name = "ttyGPS"
};

typedef enum {
	GET = 0x00,
	SET,
} ktermios_mode_t;

/* have refered to mxcuart_set_termios() in driver/serial/mxc_uart.c */
struct baud_rates {
		unsigned int baud;
		unsigned int rate;
};

static const struct baud_rates baud_table[] = {
	{4000000, B4000000}, {3500000, B3500000}, {3000000, B3000000},
	{2500000, B2500000}, {2000000, B2000000}, {1500000, B1500000},
	{1152000, B1152000}, {1000000, B1000000}, {921600, B921600},
	{576000, B576000}, {500000, B500000}, {460800, B460800},
	{230400, B230400}, {115200, B115200}, {57600, B57600},
	{38400, B38400}, {19200, B19200}, {9600, B9600}, {0, B38400}
};

#define SET_CFLAG(tc,c) (tc=(((tc)&(~(CBAUD|PARENB|PARODD|CSTOPB|CSIZE)))|(c)))

/**
  *     uart_parse_options decodes a string containing the serial console
  *     options.  The format of the string is <baud><parity><bits><stop>,
  *     eg: 115200n81
  */

typedef struct contorl_options_elememt_struct {
	int baud;
	int parity;
	int bits;
	int stop;
} ctl_opts_elem_t;

typedef struct dtd_control_options_struct {
	char options[PROCFS_NBUFFERS];
	int options_size;
	tcflag_t cflag;
	bool eof;
	atomic_t dirty;
	struct task_struct *sleep;
} dtd_ctl_opts_t;

typedef struct dtd_proc_data_struct {
		char *dir_name;
		char *file_name;
		mode_t mode;
		struct proc_dir_entry *root;
		struct file_operations *fops;
		dtd_ctl_opts_t *ctl_opts;
} dtd_proc_data_t;

typedef struct dtd_proc_struct {
	    dtd_proc_data_t *(*alloc_data) (void);
		void (*free_data) (dtd_proc_data_t *data);
		void (*set_data) (dtd_proc_data_t *data);
		int (*create) (dtd_proc_data_t *data);
		void (*remove) (dtd_proc_data_t *data);
		dtd_proc_data_t *data;
} dtd_proc_t;

typedef struct dtd_dup_struct {
	int ldisc;
	ds_conf_t config;
} dtd_dup_t;

typedef struct dtd_data_struct {
	char *path;
	int flags;
	struct file *file;
	struct ktermios old_ktermios;
	struct ktermios ktermios;
	dtd_dup_t *dup;
	dtd_proc_t *proc;
} dtd_data_t;

typedef struct dtd_thread_struct {
	struct file *(*open) (char *path, int flags);
	void (*close) (struct file *file);
	int (*duplicate) (struct file *file, int ldisc, ds_conf_t *config);
	int (*ktermios) (struct file *file, struct ktermios *ktios,
				     ktermios_mode_t mode);

	dtd_data_t *(*alloc_data) (void);
	void (*free_data) (dtd_data_t *data);
	void (*set_data) (dtd_data_t *data);

	struct task_struct *task;
	dtd_data_t *data;
} dtd_thread_t;

static dtd_thread_t dtd_thread;

static void dump(struct ktermios *ktios)
{
	cc_t *c = ktios->c_cc;

	dtd_dbg("iflag:0x%08x, lflag:0x%08x, oflag:0x%08x, cflag:0x%08x\n",
	       ktios->c_iflag, ktios->c_lflag, ktios->c_oflag, ktios->c_cflag);
	dtd_dbg("c_cc:0x:%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,"
		"%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
		c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7],
		c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15], c[16]);
}

static ctl_opts_elem_t parse_control_options_string(char *options)
{
	ctl_opts_elem_t elem = {0,};
	char *s = options;

	elem.baud = simple_strtoul(s, NULL, 10);
	while (*s >= '0' && *s <= '9')
		s++;
	if (*s)
		elem.parity = *s++;
	if (*s)
	    elem.bits = *s++ - '0';
	if (*s)
	    elem.stop = *s - '0';

	dtd_dbg("baud=%d, parity=%c, bits=%d, stop=%d\n",
		elem.baud, elem.parity, elem.bits, elem.stop);

	return elem;
}

static unsigned int glean_control_options_element(ctl_opts_elem_t *elem)
{
	unsigned int i, cflag = 0;

	/* set the baud rate */
	for (i=0; baud_table[i].baud; i++)
		if (baud_table[i].baud == elem->baud)
			cflag |= baud_table[i].rate;

	/* set the parity bits */
	switch (elem->parity) {
	case 'e': case 'E': cflag |= PARENB; break;
	case 'o': case 'O': cflag |= PARENB | PARODD; break;
	case 'n': case 'N': break;
	default: cflag = 0; goto out;
	/* case 'n': case 'N': break; */
	}

	/* set the data bits */
	switch (elem->bits) {
	case 7: cflag |= CS7; break;
	case 8: cflag |= CS8; break;
	default : cflag = 0; goto out;
  	}

	/* set the stop bits */
	switch (elem->stop) {
	case 1: break;
	case 2: cflag |= CSTOPB; break;
	default: cflag = 0; goto out;
 	}

out:
	dtd_dbg("cflag : 0x%08x\n", cflag);
	return cflag;
}

static unsigned int parse_options(char *options)
{
	unsigned int cflag;
	ctl_opts_elem_t elem;

	elem = parse_control_options_string(options);
	cflag = glean_control_options_element(&elem);

	return cflag;
}

/* proc file system */
static ssize_t dtd_proc_read(struct file *file, char *buffer,
			     size_t length, loff_t * offset)
{
	int result;
	dtd_ctl_opts_t *data = file->private_data;

	if (data->eof) {
		data->eof = false;
		result = 0;
		goto out;
	}

    if (copy_to_user(buffer, data->options, data->options_size)) {
		result = -EFAULT;
		goto out;
    }

	data->eof = true;
	result = data->options_size;;
out:
    return result;
}

static ssize_t dtd_proc_write(struct file *file, const char *buffer,
			      size_t length, loff_t * offset)
{
	int result;
	dtd_ctl_opts_t *data = file->private_data;
	char tmp[PROCFS_NBUFFERS];
	unsigned int cflag;

    if (length > data->options_size) {
		result = -EINVAL;
		goto out;
	}

	if (strncmp(data->options, buffer, length) == 0) {
		result = length;
		goto out;
	}

	if (copy_from_user(tmp, buffer, length)) {
		result = -EFAULT;
		goto out;
	}

	cflag = parse_options(tmp);
	if (cflag == 0) {
		result = -EFAULT;
		goto out;
	}

	memset(data->options, 0, data->options_size);
	memcpy(data->options, tmp, length);
	data->cflag = cflag;

	result = length;

	atomic_set(&data->dirty, 1);
	wake_up_process(data->sleep);
out:
	return result;
}

static int dtd_proc_open(struct inode *inode, struct file *file)
{
	dtd_thread_t *thread = &dtd_thread;

	file->private_data = (struct dtd_ctl_opts_t *)
		thread->data->proc->data->ctl_opts;

	return 0;
}

static int dtd_proc_close(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations dtd_proc_fops = {
	.owner   = THIS_MODULE,
	.read    = dtd_proc_read,
	.write   = dtd_proc_write,
	.open    = dtd_proc_open,
	.release = dtd_proc_close,
};

/* dtd_proc_data_struct */
dtd_proc_data_t *alloc_dtd_proc_data_struct(void)
{
	dtd_proc_data_t *data;

	data = (dtd_proc_data_t *)kzalloc(sizeof(dtd_proc_data_t), GFP_KERNEL);
	if (IS_ERR_OR_NULL(data)) {
		dtd_err("error: kzalloc() data=%ld\n",PTR_ERR(data));
		goto out;
	}

	data->ctl_opts = (dtd_ctl_opts_t *)kzalloc(sizeof(dtd_ctl_opts_t), GFP_KERNEL);
    if (IS_ERR_OR_NULL(data->ctl_opts)) {
	    dtd_err("error: kzalloc() options=%ld\n",
		PTR_ERR(data->ctl_opts));
	    goto out1;
	}

	return data;
out1:
	kfree(data);
out:
	data = ERR_PTR(-ENOMEM);
	return data;
}

void free_dtd_proc_data_struct(dtd_proc_data_t *data)
{
	kfree(data->ctl_opts);
	kfree(data);
}

void init_dtd_proc_data_struct(dtd_proc_data_t *data)
{
	data->dir_name = MODULE_NAME;
	data->file_name = PROCFS_FILE_NAME;
	data->mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
	data->fops = &dtd_proc_fops;
	sprintf(data->ctl_opts->options, "%s\n", PROCFS_OPTIONS_STRING);
	data->ctl_opts->options_size = PROCFS_NBUFFERS;
	data->ctl_opts->eof = false;
}

int create_dtd_proc_data_struct(dtd_proc_data_t *data)
{
	struct proc_dir_entry *p;

	data->root = proc_mkdir(data->dir_name, NULL);
	if (IS_ERR_OR_NULL(data->root)) {
		dtd_err("error: proc_mkdir()\n");
		goto out;
	}

	p = proc_create(data->file_name, data->mode, data->root, data->fops);
	if (IS_ERR_OR_NULL(p)) {
		dtd_err("error: proc_create()\n");
		goto out1;
	}

	return 0;

out1:
	remove_proc_entry(data->dir_name, NULL);
out:
	return -ENOMEM;
}

void remove_dtd_proc_data_struct(dtd_proc_data_t *data)
{
	remove_proc_entry(data->file_name, data->root);
	remove_proc_entry(data->dir_name, NULL);
}

static void init_proc_callback(dtd_proc_t *proc)
{
	proc->alloc_data = alloc_dtd_proc_data_struct;
	proc->free_data = free_dtd_proc_data_struct;
	proc->set_data = init_dtd_proc_data_struct;
	proc->create = create_dtd_proc_data_struct;
	proc->remove = remove_dtd_proc_data_struct;
}

static int __tty_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	int result;
	mm_segment_t fs = get_fs();

	if (!file->f_op->unlocked_ioctl) {
		result = -ENOMEM;
		goto out;
	}

	set_fs(get_ds());

	result = file->f_op->unlocked_ioctl(file, cmd, arg);

	set_fs(fs);
out:
    return result;
}

static int __tty_mode_ioctl(struct tty_struct *tty, struct file *file,
					      unsigned int cmd, unsigned long arg)
{
	int result;
	mm_segment_t fs = get_fs();

	set_fs(get_ds());

	result = tty_mode_ioctl(tty, file, cmd, arg);

	set_fs(fs);

	return result;
}

static int get_ktermios(struct file *file, struct ktermios *ktios)
{
	int result;
	struct tty_struct *tty = ((struct tty_file_private *)file->private_data)->tty;

	result = __tty_mode_ioctl(tty, file, TCGETS, (unsigned long)ktios);
	if ( result < 0) {
		dtd_err("error: __tty_mode_ioctl() %d\n", result);
	}

	return result;
}

static int set_ktermios(struct file *file, struct ktermios *ktios)
{
	int result;
	struct tty_struct *tty = ((struct tty_file_private *)file->private_data)->tty;

	result = __tty_mode_ioctl(tty, file, TCSETS, (unsigned long)ktios);
	if ( result < 0) {
		dtd_err("error: __tty_mode_ioctl() %d\n", result);
	}

	return result;
}

static int set_ktermios_dup_ttyd(struct file *file, struct ktermios *ktios,
				 ktermios_mode_t mode)
{
	int result;

	switch (mode) {
	case GET:
		dtd_info("GET KTERMIOS:\n");
		result = get_ktermios(file, ktios);
		if (result < 0)
			break;
		dump(ktios);
		break;

	case SET:
		dtd_info("SET KTERMIOS:\n");
		result = set_ktermios(file, ktios);
		if (result < 0)
			break;
		dump(ktios);
		break;
	}

	result = 0;
	return result;
}

static int duplicate_dup_ttyd(struct file *file, int ldisc, ds_conf_t *config)
{
	int result;

	result = __tty_ioctl(file, TIOCSETD, (unsigned long)&ldisc);
	if ( result < 0) {
		dtd_err("error: __tty_ioctl() %d\n", result);
		goto out;
	}

	result = __tty_ioctl(file, IOC_UART_DUPLICATE, (unsigned long)config);
	if ( result < 0) {
		dtd_err("error: __tty_ioctl() %d\n", result);
		goto out;
	}

	dtd_info("duplicated name:%s, the number:%d\n",
		config->dev_name, config->nr);

out:
	return result;
}

#if 0
static int get_modem_status(struct file *file, int *status)
{
	int result;

	result = __tty_ioctl(file, TIOCMGET, (unsigned long)status);
	if ( result < 0) {
		dtd_err("error: __tty_ioctl() %d\n", result);
	}

	return result;
}

static int set_modem_status(struct file *file, int *status)
{
	int result;

	result = __tty_ioctl(file, TIOCMSET, (unsigned long)status);
	if ( result < 0) {
		dtd_err("error: __tty_ioctl() %d\n", result);
	}

	return result;
}
#endif
/*
#define DUP_TTYD_READ_DUMMY
#define DUP_TTYD_READ_BUF_MAX	1
*/
static struct file *open_dup_ttyd(char *path, int flags)
{
	struct file *file;
	mm_segment_t fs;

#if defined(DUP_TTYD_READ_DUMMY)
	int bytes_read = 0;
	int retry = 10;
	char buf[DUP_TTYD_READ_BUF_MAX];
	loff_t pos = 0;
#endif

	dtd_dbg("path:%s, flags=0x%08X\n",path, flags);

	fs = get_fs();
	set_fs(get_ds());
	file = filp_open(path, flags, 0);

#if defined(DUP_TTYD_READ_DUMMY)
	do {
		bytes_read = vfs_read(file, buf, DUP_TTYD_READ_BUF_MAX, &pos);
	}
	while (bytes_read <= 0 && retry--);

	if (!retry) {
		dtd_err("%s: no data from %s\n", __func__, path);
	}
#endif

	set_fs(fs);
	if (IS_ERR(file)) {
		dtd_err("error: filp_open() %ld, %s\n",PTR_ERR(file) , path);
	}

	msleep(500);

	return file;
}

static void close_dup_ttyd(struct file *file)
{
	filp_close(file, 0);
}

static dtd_data_t *alloc_dtd_data_struct(void)
{
	dtd_data_t *data;

	data = (dtd_data_t *)kzalloc(sizeof(dtd_data_t), GFP_KERNEL);
	if (IS_ERR_OR_NULL(data)) {
		dtd_err("error: kzalloc() data=%ld\n",PTR_ERR(data));
		goto out;
	}

	data->dup = (dtd_dup_t *)kzalloc(sizeof(dtd_dup_t), GFP_KERNEL);
	if (IS_ERR_OR_NULL(data->dup)) {
		dtd_err("error: kzalloc() data->dup=%ld\n",PTR_ERR(data->dup));
		goto out1;
	}

	data->proc = (dtd_proc_t *)kzalloc(sizeof(dtd_proc_t), GFP_KERNEL);
	if (IS_ERR_OR_NULL(data->proc)) {
		dtd_err("error: kzalloc() proc=%ld\n",PTR_ERR(data->proc));
		goto out2;
	}

	return data;

out2:
	kfree(data->dup);
out1:
	kfree(data);
out:
	data = ERR_PTR(-ENOMEM);
	return data;
}

static void free_dtd_data_struct(dtd_data_t *data)
{
	kfree(data->proc);
	kfree(data->dup);
	kfree(data);
}

static void init_dtd_data_struct(dtd_data_t *data)
{
	data->path = DEVICE_PATH;
	data->flags = O_RDWR;
	data->ktermios = dtd_init_ktermios;
	data->dup->ldisc = N_DUP;
	data->dup->config = ds_config;
}

static void init_thread_callback(dtd_thread_t *thread)
{
	thread->open = open_dup_ttyd;
	thread->close = close_dup_ttyd;
	thread->duplicate =  duplicate_dup_ttyd;
	thread->ktermios = set_ktermios_dup_ttyd;

	thread->alloc_data = alloc_dtd_data_struct;
	thread->free_data = free_dtd_data_struct;
	thread->set_data = init_dtd_data_struct;
}

/* dupttyd kernel thread */
static int dup_ttyd_kthread(void *arg)
{
	int result;
	dtd_thread_t *thread = arg;
	dtd_data_t *data = thread->data;
	dtd_proc_t *proc = data->proc;
	dtd_dup_t *dup = data->dup;
	dtd_ctl_opts_t *ctl_opts;

	dtd_dbg("+ %s start\n", __func__);
	result = 0;

	init_proc_callback(proc);
	proc->data = proc->alloc_data();
	if (IS_ERR_OR_NULL(proc->data)) {
		goto out;
	}
	proc->set_data(proc->data);

	result = proc->create(proc->data);
	if (result < 0)
		goto out1;

	data->file = thread->open(data->path, data->flags);
	if (IS_ERR(data->file))
		goto out2;

	result = thread->ktermios(data->file, &data->old_ktermios, GET);
	if (result < 0)
		goto out3;

	ctl_opts = proc->data->ctl_opts;
	SET_CFLAG(data->ktermios.c_cflag, parse_options(ctl_opts->options));
	result = thread->ktermios(data->file, &data->ktermios, SET);
	if (result < 0)
		goto out4;
#if 0
	{
		int status;
		get_modem_status(data->file, &status);
	    status |= = (TIOCM_DTR | TIOCM_RTS);
		set_modem_status(data->file, &status);
		dtd_info("modem status : 0x%08x\n", status);
	}
#endif
	result = thread->duplicate(data->file, dup->ldisc, &dup->config);
	if (result < 0)
		goto out4;

	ctl_opts->sleep = current;

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();

		if (kthread_should_stop())
			break;

		if (atomic_read(&ctl_opts->dirty)) {
			SET_CFLAG(data->ktermios.c_cflag, ctl_opts->cflag);
			thread->ktermios(data->file, &data->ktermios, SET);
			atomic_set(&ctl_opts->dirty, 0);
		}
	}
out4:
	thread->ktermios(data->file, &data->old_ktermios, SET);
out3:
	thread->close(data->file);
out2:
	proc->remove(proc->data);
out1:
	proc->free_data(proc->data);
out:
	thread->free_data(data);

	dtd_dbg("- %s exit\n", __func__);

	return result;
}

static int __init dup_ttyd_init(void)
{
	dtd_thread_t *thread = &dtd_thread;

	init_thread_callback(thread);
	thread->data = thread->alloc_data();
	if (IS_ERR_OR_NULL(thread->data)) {
		goto out;
	}

	thread->set_data(thread->data);

	thread->task = kthread_run(dup_ttyd_kthread,
				   (dtd_thread_t *)thread,
				   "k%s", MODULE_NAME);
	if (IS_ERR(thread->task)) {
		dtd_err("error: kthread_run() %ld\n",PTR_ERR(thread->task));
		goto out1;
	}

	return 0;
out1:
	thread->free_data(thread->data);
out:
	return -ENOMEM;
}
module_init(dup_ttyd_init);

#if 0
static __exit void dup_ttyd_exit(void)
{
	dtd_thread_t *thread = &dtd_thread;

	kthread_stop(thread->task);
}
module_exit(dup_ttyd_exit);
#endif

MODULE_LICENSE("GPL");

