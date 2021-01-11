#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#define BUFF_SIZE 20

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

unsigned int regA[BUFF_SIZE];
unsigned int regB[BUFF_SIZE];
unsigned int regC[BUFF_SIZE];
unsigned int regD[BUFF_SIZE];
unsigned int rezultat;
int pos = 0;
int endRead = 0;

int alu_open(struct inode *pinode, struct file *pfile);
int alu_close(struct inode *pinode, struct file *pfile);
ssize_t alu_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t alu_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = alu_open,
	.read = alu_read,
	.write = alu_write,
	.release = alu_close,
};


int alu_open(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully opened file\n");
		return 0;
}

int alu_close(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully closed file\n");
		return 0;
}

ssize_t alu_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
	int ret;
	char buff[BUFF_SIZE];
	long int len;
	if (endRead){
		endRead = 0;
		pos = 0;
		printk(KERN_INFO "Succesfully read from file\n");
		return 0;
	}
	//len = scnprintf(buff,BUFF_SIZE , "%d ", storage[0]);
	ret = copy_to_user(buffer, buff, len);
	if(ret)
		return -EFAULT;
	pos ++;
	if (pos == 10) {
		endRead = 1;
	}
	return len;
}

ssize_t alu_write(struct file *pfile, const char *buffer, size_t length, loff_t *offset) 
{
	int ret;
	char buff[BUFF_SIZE];
	char* ptr;
	char oznaka_registar;
	char sabirak1,sabirak2;
	unsigned int vrednost; 
	
	unsigned int prvi_op,drugi_op;
	
	ret=copy_from_user(buff, buffer, length);


	
	buff[length-1]='\0';


	if((!strncmp(buff,"regA=",5) || (!strncmp(buff,"regB=",5)) || (!strncmp(buff,"regC=",5)) || (!strncmp(buff,"regD=",5))))                                          
  	{
  	ret=sscanf(buff, "reg%c=%x", &oznaka_registar, &vrednost);

  	
  		if(oznaka_registar=='A')
  		{
  			regA[0]=vrednost;
  			printk(KERN_INFO "Upisana je vrednost %#04x u reg%c", regA[0], oznaka_registar);
  		}			

  		if(oznaka_registar=='B')
  		{
  			regB[0]=vrednost;
  			printk(KERN_INFO "Upisana je vrednost %#04x u reg%c", regB[0], oznaka_registar);
  		}
  		
  		if(oznaka_registar=='C')
  		{
  			regC[0]=vrednost;
  			printk(KERN_INFO "Upisana je vrednost %#04x u reg%c", regC[0], oznaka_registar);
  		}
  	    	
  	    	if(oznaka_registar=='D')
  		{
  			regA[0]=vrednost;
  			printk(KERN_INFO "Upisana je vrednost %#04x u reg%c", regD[0], oznaka_registar);
  		}
  	}
  	
  	else 
  	{
  		ptr=strchr(buff,'+');
  		if(ptr!=NULL)
  		{
  		ret=sscanf(buff, "reg%c+reg%c", &sabirak1, &sabirak2);
  		printk(KERN_INFO "Sabiranje reg%c i reg%c", sabirak1, sabirak2);
  		}
  		
  		
  		if(sabirak1=='A')
  		{
  		prvi_op=regA[0];
  		}
  		
  		if(sabirak1=='B')
  		{
  		prvi_op=regB[0];
  		}
  		
  		
  		if(sabirak1=='C')
  		{
  		prvi_op=regC[0];
  		}
  		
  		if(sabirak1=='D')
  		{
  		prvi_op=regD[0];
  		}
  		
  			
  		if(sabirak2=='A')
  		{
  		drugi_op=regA[0];
  		}
  		
  		if(sabirak2=='B')
  		{
  		drugi_op=regB[0];
  		}
  		
  		
  		if(sabirak2=='C')
  		{
  		drugi_op=regC[0];
  		}
  		
  		if(sabirak2=='D')
  		{
  		drugi_op=regD[0];
  		}
  		
  		rezultat=prvi_op+drugi_op;
  		printk(KERN_INFO "rezultat sabiranja je %#04x", rezultat);

	}
	
	return length;
}

static int __init alu_init(void)
{
   int ret = 0;
	int i=0;

	//Initialize array
	//for (i=0; i<10; i++)
		//storage[i] = 0;

   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "alu");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "alu_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
   my_device = device_create(my_class, NULL, my_dev_id, NULL, "alu");
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

	my_cdev = cdev_alloc();	
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, 1);
	if (ret)
	{
      printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "Hello world\n");

   return 0;

   fail_2:
      device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;
}

static void __exit alu_exit(void)
{
   cdev_del(my_cdev);
   device_destroy(my_class, my_dev_id);
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(alu_init);
module_exit(alu_exit);
