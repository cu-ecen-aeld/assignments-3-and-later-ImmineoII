/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h> // kmalloc
#include "aesd-circular-buffer.h"
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Eric Boccati"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */

    struct aesd_dev* dev_data = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev_data;

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
    * TODO: handle read
    */
    size_t read_offs = (size_t) filp->f_pos;
    size_t * ret_offs;

    struct aesd_buffer_entry* entry = aesd_circular_buffer_find_entry_offset_for_fpos(aesd_device.dev_buff, read_offs, ret_offs);
    if ( entry->size == 0 ){
        PDEBUG("got empty entry");
        retval = 0;
        return retval;
    }
    PDEBUG("got entry %s with size %d",entry->buffptr, entry->size);
    if ( entry->size > count ){
        retval = -EFAULT;
        return retval;
    }
    
    retval = entry->size;
    copy_to_user(buf, entry->buffptr, count);
    *f_pos = read_offs - *ret_offs + entry->size; 

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    retval = copy_from_user(&aesd_device.write_buff[aesd_device.write_len], buf, count);
    if (retval != 0){
        return -EFAULT;
    }
    aesd_device.write_len = aesd_device.write_len + count;
    PDEBUG("write buffer cointains: %s", aesd_device.write_buff);

    if ( strcmp(&aesd_device.write_buff[aesd_device.write_len -1], "\n") == 0){
        PDEBUG("newline detected");

        struct aesd_buffer_entry *new_entry = (struct aesd_buffer_entry*) kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
        new_entry->size = aesd_device.write_len;
        new_entry->buffptr = (char *) kmalloc(sizeof(char) * aesd_device.write_len, GFP_KERNEL);
        memcpy(aesd_device.write_buff, new_entry->buffptr, sizeof(char) * aesd_device.write_len);

        struct aesd_buffer_entry* old_entry = aesd_circular_buffer_add_entry(aesd_device.dev_buff, new_entry);
        if ( old_entry != NULL ){
            kfree(old_entry->buffptr);
            kfree(old_entry);
        }

        memset(aesd_device.write_buff, 0, sizeof(aesd_device.write_buff));
        aesd_device.write_len = 0;
    }
    retval = count;
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));
    aesd_device.dev_buff = (struct aesd_circular_buffer *) kmalloc(sizeof(struct aesd_circular_buffer),GFP_KERNEL);
    aesd_circular_buffer_init(aesd_device.dev_buff);
    /**
     * TODO: initialize the AESD specific portion of the device
     */

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
