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

#include <asm-generic/errno-base.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h> // kmalloc
#include "aesd-circular-buffer.h"
#include "aesdchar.h"
#include "aesd_ioctl.h"

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
    ssize_t bytes_read = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
    * TODO: handle read
    */
    size_t read_offs = (size_t) filp->f_pos;
    size_t ret_offs;
    PDEBUG("read offset %lld bytes",read_offs);
    PDEBUG("we have buffer at %p", aesd_device.dev_buff);
    struct aesd_buffer_entry* entry = aesd_circular_buffer_find_entry_offset_for_fpos(aesd_device.dev_buff, read_offs, &ret_offs);
    PDEBUG("ret offset %lld bytes",ret_offs);
    if ( entry == 0 ){
        PDEBUG("got empty entry");
        return 0;
    }
    PDEBUG("got entry %s with size %d",entry->buffptr, entry->size);
    if ( entry->size > count ){
        return -EFAULT;
    }
    bytes_read = min(count, entry->size - ret_offs);
    copy_to_user(buf, entry->buffptr + ret_offs, bytes_read);
    *f_pos = read_offs - ret_offs + entry->size; 

    return bytes_read;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */

    mutex_lock(&aesd_device.write_mutex);
    retval = copy_from_user(&aesd_device.write_buff[aesd_device.write_len], buf, count);
    if (retval != 0){
        mutex_unlock(&aesd_device.write_mutex);
        return -EFAULT;
    }
    aesd_device.write_len = aesd_device.write_len + count;
    PDEBUG("write buffer cointains: %s", aesd_device.write_buff);

    if ( strcmp(&aesd_device.write_buff[aesd_device.write_len -1], "\n") == 0){
        PDEBUG("newline detected");

        struct aesd_buffer_entry *new_entry = (struct aesd_buffer_entry*) kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
        new_entry->size = aesd_device.write_len;
        new_entry->buffptr = (char *) kmalloc(sizeof(char) * aesd_device.write_len, GFP_KERNEL);
        memcpy(new_entry->buffptr, aesd_device.write_buff, sizeof(char) * aesd_device.write_len);
        PDEBUG("write buffer cointains: %s", aesd_device.write_buff);

        struct aesd_buffer_entry* old_entry = aesd_circular_buffer_add_entry(aesd_device.dev_buff, new_entry);
        if ( old_entry != NULL ){
            PDEBUG("free entry memory containing %s size %d", old_entry->buffptr, old_entry->size);
            kfree(old_entry->buffptr);
            kfree(old_entry);
        }

        memset(aesd_device.write_buff, 0, sizeof(aesd_device.write_buff));
        aesd_device.write_len = 0;
    }
    mutex_unlock(&aesd_device.write_mutex);
    retval = count;
    return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence){
    loff_t newpos;
    switch (whence) {
        case 0: /* SEEK_SET*/
            newpos = off;
            break;
        case 1: /* SEEK_CUR*/
            newpos = filp->f_pos + off;
            break;
        case 2: /* SEEK_END*/
            newpos = aesd_device.dev_buff->size + off;
            break;
        default: /* can't happen */
            return -EINVAL;
    }
    if(newpos < 0){
        return -EINVAL;
    }
    if(newpos > aesd_device.dev_buff->size){
        return -EINVAL;
    }
    filp->f_pos = newpos;
    return newpos;
}

long aesd_ioctl(struct file *filp,unsigned int cmd, unsigned long arg){
    struct aesd_seekto* pargs;
    long newpos;
    int ret = copy_from_user(pargs,(struct aesd_seekto*)arg, sizeof(pargs));
    switch (cmd) {
        case AESDCHAR_IOCSEEKTO:
            newpos = aesd_circular_buffer_offset_adjust(aesd_device.dev_buff, pargs->write_cmd, pargs->write_cmd_offset);
            filp->f_pos = newpos;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = aesd_ioctl,
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
    result = alloc_chrdev_region(&dev, aesd_minor, 1,"aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));
    aesd_device.dev_buff = (struct aesd_circular_buffer *) kmalloc(sizeof(struct aesd_circular_buffer),GFP_KERNEL);
    aesd_circular_buffer_init(aesd_device.dev_buff);
    mutex_init(&aesd_device.write_mutex);
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
    // int index = 0;
    // struct aesd_buffer_entry *entry;
    // AESD_CIRCULAR_BUFFER_FOREACH(entry, aesd_device.dev_buff, index) {
    //    kfree(entry->buffptr);
    //    kfree(entry);
    // }
    mutex_destroy(&aesd_device.write_mutex);
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
