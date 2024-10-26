#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/highmem.h>
#include <linux/uh.h>

#define UH_LOG_START	(0xD0100000)
#define UH_LOG_SIZE		(0x40000)

unsigned long uh_log_paddr;
unsigned long uh_log_size;

ssize_t	uh_log_read(struct file *filep, char __user *buf, size_t size, loff_t *offset)
{
	static size_t log_buf_size;
	unsigned long *log_addr = 0;

	if (!strcmp(filep->f_path.dentry->d_iname, "uh_log"))
		log_addr = (unsigned long *)phys_to_virt(UH_LOG_START);
	else
		return -EINVAL;

	if (!*offset) {
		log_buf_size = 0;
		while (log_buf_size < UH_LOG_SIZE && ((char *)log_addr)[log_buf_size] != 0)
			log_buf_size++;
	}

	if (*offset >= log_buf_size)
		return 0;

	if (*offset + size > log_buf_size)
		size = log_buf_size - *offset;

	if (copy_to_user(buf, (const char *)log_addr + (*offset), size))
		return -EFAULT;

	*offset += size;
	return size;
}

static const struct proc_ops uh_proc_ops = {
	.proc_read		= uh_log_read,
};

static int __init uh_log_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("uh_log", 0644, NULL, &uh_proc_ops);
	if (!entry) {
		pr_err("uh_log: Error creating proc entry\n");
		return -ENOMEM;
	}

	pr_info("uh_log : create /proc/uh_log\n");
	uh_call(UH_PLATFORM, 0x5, (u64)&uh_log_paddr, (u64)&uh_log_size, 0, 0);
	return 0;
}

static void __exit uh_log_exit(void)
{
	remove_proc_entry("uh_log", NULL);
}

module_init(uh_log_init);
module_exit(uh_log_exit);
