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
#define register_size 8

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

int regA, regB, regC, regD, carry;
int rezultat;
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

		
        len = scnprintf(buff,BUFF_SIZE , "0x%x ", rezultat);
	ret = copy_to_user(buffer, buff, len);
	if(ret)
		return -EFAULT;
	printk(KERN_INFO "Succesfully read from file\n");

	return len;
}

ssize_t alu_write(struct file *pfile, const char *buffer, size_t length, loff_t *offset) 
{
	int ret;
	char buff[BUFF_SIZE];
	char* ptr;
	char oznaka_registar;
	char sabirak1,sabirak2,operacija;
	int vrednost; 
	int prvi_op=0,drugi_op=0;
	
	ret=copy_from_user(buff, buffer, length);


	
	buff[length-1]='\0';


	if((!strncmp(buff,"regA=",5) || (!strncmp(buff,"regB=",5)) || (!strncmp(buff,"regC=",5)) || (!strncmp(buff,"regD=",5))))                                          
  	{
  		ret=sscanf(buff, "reg%c=%x", &oznaka_registar, &vrednost);

  	
  		if(oznaka_registar=='A')
  		{
  			regA=vrednost;
  			printk(KERN_INFO "Upisana je vrednost %#04x u reg%c", regA, oznaka_registar);
  		}			

  		if(oznaka_registar=='B')
  		{
  			regB=vrednost;
  			printk(KERN_INFO "Upisana je vrednost %#04x u reg%c", regB, oznaka_registar);
  		}
  		
  		if(oznaka_registar=='C')
  		{
  			regC=vrednost;
  			printk(KERN_INFO "Upisana je vrednost %#04x u reg%c", regC, oznaka_registar);
  		}
  	    	
  	    	if(oznaka_registar=='D')
  		{
  			regD=vrednost;
  			printk(KERN_INFO "Upisana je vrednost %#04x u reg%c", regD, oznaka_registar);
  		}
  	}
  	
  	else 
  	{
  		ret = sscanf(buff, "reg%c %c reg%c", &sabirak1, &operacija, &sabirak2);
		if(ret == 3)
		{
			switch(sabirak1)
			{
				case 'A': prvi_op=regA;
					  break;
			        case 'B': prvi_op=regB;
					  break;
				case 'C': prvi_op=regC;
					  break;
				case 'D': prvi_op=regD;
					  break;
		                default : printk(KERN_WARNING "Reg je A,B,C,D");
			};
			switch(sabirak2)
			{
				case 'A': drugi_op=regA;
					  break;
			        case 'B': drugi_op=regB;
					  break;
				case 'C': drugi_op=regC;
					  break;
				case 'D': drugi_op=regD;
					  break;
		                default : printk(KERN_WARNING "Reg je A,B,C,D");
			};
			switch(operacija)
			{
				case '+': rezultat=prvi_op+drugi_op;
					  break;
			        case '-': rezultat=prvi_op-drugi_op;
					  break;
				case '*': rezultat=prvi_op*drugi_op;
					  break;
				case '/': 
					  if(drugi_op==0)
						  printk(KERN_WARNING "Deljenje nulom");
					  else 
						  rezultat=prvi_op/drugi_op;
					  break;
		                default : printk(KERN_WARNING "Operacija je +,-,*,/");
			};
	
		}


	}
	
	return length;
}

static int __init alu_init(void)
{
   int ret = 0;
   regA=0;
   regB=0;
   regC=0;
   regD=0;
   rezultat=0;

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
