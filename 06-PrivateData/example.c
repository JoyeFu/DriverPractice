#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/errno.h>

MODULE_LICENSE("Dual BSD/GPL");

static char data[] = "0123456789\r\n";

static int example_open(struct inode *inode, struct file *filp) {
	printk(KERN_DEBUG "EXAMPLE: open\n");

	/* Map the data location to the file data pointer. */
	filp->private_data = data;

	return 0;
}

static int example_close(struct inode *inode, struct file *filp) {
	printk(KERN_DEBUG "EXAMPLE: close\n");
	
	/* Release the mapping of file data address. */
	if(filp->private_data) {
		filp->private_data = NULL;
	}

	return 0;
}

static ssize_t example_read(struct file *filp, char __user *buf, size_t size, loff_t *f_pos) {
	size_t count;
	uint8_t byte;
	uint8_t *data_p;
	
	printk(KERN_DEBUG "EXAMPLE: read (size=%zu)\n", size);

	data_p = filp->private_data;
	for(count = 0; (count < size) && (*f_pos) < strlen(data); ++(*f_pos), ++count) {
		byte = data_p[*f_pos];
		if(copy_to_user(buf + count, &byte, 1) != 0) {
			break;
		}
		printk(KERN_DEBUG "EXAMPLE: read (buf[%zu]=%02x)\n", count, (unsigned)byte);
	}

	return count;
}

static ssize_t example_write(struct file *filp, const char __user *buf, size_t size, loff_t *f_pos) {
	size_t count;
	ssize_t ret;
	uint8_t byte;
	uint8_t *data_p;

	printk(KERN_DEBUG "EXAMPLE: write (size=%zu)\n", size);

	data_p = filp->private_data;
	for(count = 0; (count < size) && (*f_pos) < strlen(data); ++(*f_pos), ++count) {
		if(copy_from_user(&byte, buf + count, 1) != 0) {
			break;
		}
		data_p[*f_pos] = byte;
		printk(KERN_DEBUG "EXAMPLE: write (buf[%zu]=%02x)\n", count, (unsigned)byte);
	}

	if((count == 0) && ((*f_pos) >= strlen(data))) {
		ret = -ENOBUFS;
	}
	else {
		ret = count;
	}

	return ret;
}

static struct file_operations example_fops = {
	.open = example_open,
	.release = example_close,
	.read = example_read,
	.write = example_write,
};

#define EXAMPLE_NAME	"example"

static unsigned int example_major;
static unsigned int example_devs = 2;
static struct cdev example_cdev;

static int example_init(void) {
	dev_t dev;
	int alloc_ret, cdev_err;

	printk(KERN_DEBUG "EXAMPLE: init\n");

	/* Allocate a character device. */
	alloc_ret = alloc_chrdev_region(&dev, 0, example_devs, EXAMPLE_NAME);
	if(alloc_ret) {
		printk(KERN_DEBUG "EXAMPLE: Failed to allocate a character device\n");
		return -1;
	}
	/* Initial the character device ddriver. */
	example_major = MAJOR(dev);
	cdev_init(&example_cdev, &example_fops);
	example_cdev.owner = THIS_MODULE;
	/* Add the character device driver into system. */
	dev = MKDEV(example_major, 0);
	cdev_err = cdev_add(&example_cdev, dev, example_devs);
	if(cdev_err) {
		printk(KERN_DEBUG "EXAMPLE: Failed to register a character device\n");
		/* Release the allocated character device. */
		if(alloc_ret == 0) {
			unregister_chrdev_region(dev, example_devs);
		}
		return -1;
	}

	printk(KERN_DEBUG "EXAMPLE: %s driver(major %d) installed.\n", EXAMPLE_NAME, example_major);

	return 0;
}

static void example_exit(void) {
	dev_t dev = MKDEV(example_major, 0);

	printk(KERN_DEBUG "EXAMPLE: exit\n");
	/* Delete the character device driver from system. */
	cdev_del(&example_cdev);
	/* Unregister the allocated character device. */
	unregister_chrdev_region(dev, example_devs);
	printk(KERN_DEBUG "EXAMPLE: %s driver removed.\n", EXAMPLE_NAME); 
}

module_init(example_init);
module_exit(example_exit);
