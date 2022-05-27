/*
 * 实现一个 用户信息管理系统 的功能，完成用户的添加、删除、修改和查询。 
 * 采用字符设备的方式把数据存储到内核， 实现该字符设备的 read/write/ioctl 接口。
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kdev_t.h>

#define DEVICE_NAME "my_chrdev"
#define DEV_SIZE 0x1000
#define CDEV_MAJOR 255

#define MEM_MAGIC 'U' //定义一个幻数，而长度正好和ASC码长度一样为8位，0x00-0xff
#define USER_ADD _IO(MEM_MAGIC,0)
#define USER_DEL _IO(MEM_MAGIC,1) 
#define USER_MOD _IO(MEM_MAGIC,2)
#define USER_INQ _IO(MEM_MAGIC,3)

static int cdev_major = CDEV_MAJOR;

struct chrdev_dev {  //private->data
	struct cdev cdev;
	unsigned char mem[DEV_SIZE]; 
};

struct userdata {
	int index;  //编号
	char name[10][20];
}Udata;


dev_t devno;
struct chrdev_dev *pdev;
static struct class *my_class;

static ssize_t my_chrdev_read(struct file *filp, char __user *buf, size_t size, loff_t *offp)
{
	pr_err("%s,%d", __func__, __LINE__);
	unsigned long p = *offp;
	unsigned int count = size;
	int ret = 0;
	struct chrdev_dev *dev = filp->private_data;

	if (p >= DEV_SIZE)
		return 0;
	if (count > DEV_SIZE - p)
		count = DEV_SIZE - p;

	if (copy_to_user(buf, dev->mem, count)) {
		ret = EFAULT;
	}else {
		*offp += count;
		ret = count;

		printk(KERN_INFO "read %u bytes(s)", count);
	}

	return ret;
}


static ssize_t my_chrdev_write(struct file *filp, const char __user *buf, size_t size, loff_t *offp)
{
	pr_err("%s,%d", __func__, __LINE__);
	unsigned long p = *offp;
	unsigned int count = size;
	int ret = 0;
	struct chrdev_dev *dev = filp->private_data; 

	if (p >= DEV_SIZE)
		return 0;
	if (count > DEV_SIZE - p)
		count = DEV_SIZE - p;
	
	if (copy_from_user(dev->mem + p, buf, count)) {
		ret = EFAULT;
	}else {
		*offp += count;
		ret = count;

		printk(KERN_INFO "write %u bytes(s) from %lu\n", count, p);
	}

	return ret;
}


static long my_chrdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	//pr_err("%s,%d", __func__, __LINE__);
	int i=0;
	struct userdata val;
	switch (cmd) {
		case USER_ADD:
			printk("cmd: USER_ADD");
			copy_from_user(&val, (struct userdata *)arg, sizeof(struct userdata));
			if(strlen(Udata.name[val.index])) 
				printk("name[%d] has existed",val.index);
			else {	
				memcpy(Udata.name[val.index],val.name[val.index],20);
				printk("add user name[%d]=%s\n", val.index, val.name[val.index]);
			}
			break;
		case USER_INQ:
			printk("cmd: USER_INQ");
			if(copy_to_user((struct userdata *)arg, &Udata, sizeof(struct userdata))) 
				printk("copy_to_user error!\n");
			while(i<10) {
				if(strlen(Udata.name[i]))
					printk("name[%d]=%s\n", i, Udata.name[i]);
				i++;
			}
			break;
		case USER_DEL:
			printk("cmd: USER_DEL");
			copy_from_user(&val, (struct userdata *)arg, sizeof(struct userdata));
			if(strlen(val.name[val.index])) {
				printk("del user[%d] name is %s", val.index, val.name[val.index]);
				memset(Udata.name[val.index], 0, 20);
			}
			else
				printk("del user[%d] no such person!", val.index);
			break;
		case USER_MOD:
			printk("cmd: USER_MOD");
			copy_from_user(&val, (struct userdata *)arg, sizeof(struct userdata));
			if(strlen(val.name[val.index])) {
				printk("modify user[%d] name to %s", val.index, val.name[val.index]);
				memcpy(Udata.name[val.index],val.name[val.index],20);
			}
			else
				printk("del user[%d] no such person!", val.index);
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static int my_chrdev_open(struct inode *inode, struct file *filp)
{
	pr_err("%s,%d", __func__, __LINE__);
	/*获取次设备号*/
	int num = MINOR(inode->i_rdev);
	if (num == 0) {
		printk("minor = %d\n", num);
		filp->private_data = pdev; // 私有数据一般指向设备结构体
		printk(KERN_INFO "Hey! User information management opened!\n");
	} else
		return -ENODEV;  //无效的次设备号
	
	return 0;
}

static struct file_operations chrdev_fops = {
	.owner = THIS_MODULE,
	.read = my_chrdev_read,
	.write = my_chrdev_write,
	.open = my_chrdev_open,
	.unlocked_ioctl = my_chrdev_ioctl,
};

/* 初始化并添加cdev */
static void setup_cdev(struct chrdev_dev *dev, int index, char *classname)
{
	pr_err("%s,%d", __func__, __LINE__);
	int err, devno = MKDEV(cdev_major, index);
	/*struect cdev结构体和file_operations结构体绑定*/
	cdev_init(&dev->cdev, &chrdev_fops); 
	dev->cdev.owner = THIS_MODULE;
	/* 注册字符设备 */
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding globalmem %d", err, index);

	// 为了省去手动创建设备节点，创建class,并将class注册到内核中
	my_class = class_create(THIS_MODULE, classname);
	// 用户空间的udev会响应device_create函数，去/sys/class下寻找对应的类，在/dev下自动创建节点
	device_create(my_class, NULL, devno, NULL, classname);
}

/*设备驱动模块加载函数*/
static int __init my_chrdev_init(void)
{
	pr_err("%s,%d", __func__, __LINE__);
	int ret;
	devno = MKDEV(cdev_major, 0);

	/* 申请设备号，当xxx_major不为0时，表示静态指定；当为0时，表示动态申请*/
	if (devno)
		ret = register_chrdev_region(devno, 1, DEVICE_NAME);
	else {  /* 动态分配设备号 */
		ret = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
		cdev_major = MAJOR(devno);
	}
	if (ret < 0)
		return ret;

	/* 为设备描述结构分配内存 */
	pdev = kmalloc(sizeof(struct chrdev_dev), GFP_KERNEL);
	if (!pdev) {  /* 申请失败 */
		ret = - ENOMEM;
		goto fail_malloc;
	}
	memset (pdev, 0, sizeof(struct chrdev_dev));

	setup_cdev(pdev, 0, "chardev0");
	return 0;

fail_malloc:
	unregister_chrdev_region(devno, 1);
	return ret;
}

/*模块卸载函数*/
static void __exit my_chrdev_exit(void)
{
	pr_err("%s,%d", __func__, __LINE__);
	// 首先取消掉对应的节点
	device_destroy(my_class, MKDEV(cdev_major, 0));
	// 再删除对应的类
	class_destroy(my_class);
	// 取消注册驱动
	cdev_del(&pdev->cdev);
	// 释放设备号
	unregister_chrdev_region(devno, 1);
	// 释放设备结构体内存
	kfree(pdev);
    return;
}

module_init(my_chrdev_init);
module_exit(my_chrdev_exit);

MODULE_DESCRIPTION("User information management");
MODULE_AUTHOR("lzs");
MODULE_LICENSE("GPL");