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
#include <linux/wait.h>

#define BUFF_SIZE 20
#define register_size 8

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

DECLARE_WAIT_QUEUE_HEAD(readQ);
DECLARE_WAIT_QUEUE_HEAD(writeQ);

int regA, regB, regC, regD, carry;
int rezultat,format=1;
int pos = 0;
int endRead = 0;
int rezultat_binarni[8]={0,0,0,0,0,0,0,0};
int flag;
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

	if (endRead)
	{
		flag=0;
		wake_up_interruptible(&writeQ);

		endRead=0;
		printk(KERN_INFO "Succesfully read from file\n");
		return 0;
	}	
	else
	{
		if(format==1)
		{
			len = scnprintf(buff,BUFF_SIZE , "0x%x , %d", rezultat,carry);
			ret = copy_to_user(buffer, buff, len);
			endRead=1;
	        }
		else if (format==2)
		{
			len = scnprintf(buff,BUFF_SIZE , "%d , %d", rezultat,carry);
			ret = copy_to_user(buffer, buff, len);
			endRead=1;
	        }
		else if(format==3)
		{	
			if(pos<8)
			{
				len = scnprintf(buff,BUFF_SIZE , "%d", rezultat_binarni[7-pos]);
				ret = copy_to_user(buffer, buff, len);
				pos++;
			}
			else 
			{	
				len = scnprintf(buff,BUFF_SIZE , " , %d ", carry);
				ret = copy_to_user(buffer, buff, len);

				endRead=1;
				pos=0;
			}
	        }

	}
	if(ret)
		return -EFAULT;

	return len;
}

ssize_t alu_write(struct file *pfile, const char *buffer, size_t length, loff_t *offset) 
{
	int ret;
	char buff[BUFF_SIZE];
	char* ptr;
	char oznaka_registar,slovo1,slovo2,slovo3;
	char sabirak1,sabirak2,operacija;
	int vrednost; 
	int prvi_op=0,drugi_op=0;
	int promenljiva;
	int i;
	ret=copy_from_user(buff, buffer, length);


	
	buff[length-1]='\0';


	if((!strncmp(buff,"regA=",5) || (!strncmp(buff,"regB=",5)) || (!strncmp(buff,"regC=",5)) || (!strncmp(buff,"regD=",5))))                                          
  	{
  		ret=sscanf(buff, "reg%c=%x", &oznaka_registar, &vrednost);
		
  		if(ret==2)
		{	
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
			printk(KERN_WARNING "Pogresan format unosa");

  	}
	else if(!strncmp(buff,"format=",7)) 
	{
		ret=sscanf(buff, "format=%c%c%c", &slovo1, &slovo2, &slovo3);
		if(ret==3)
		{	
			if(slovo1=='h' && slovo2=='e' && slovo3=='x')
				format=1; //hex
		
			else if(slovo1=='d' && slovo2=='e' && slovo3=='c')
				format=2; //dec
		
	        	else if(slovo1=='b' && slovo2=='i' && slovo3=='n')
				format=3; //bin
		}
		else 
			printk(KERN_WARNING "Pogresan format unosa");

			
	}
  	else 
  	{
		if(wait_event_interruptible(writeQ,(flag==0)))
		{	
			//printk(KERN_WARNING "Proces je blokiran");
	
			return -ERESTARTSYS;
		}
			
  		ret = sscanf(buff, "reg%c %c reg%c", &sabirak1, &operacija, &sabirak2);
		if(ret == 3)
		{	
			flag=1;
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

			if (rezultat>255)
			{	
				rezultat=rezultat-255;
				carry=1;
			}
			else if (rezultat<0)
			{
				rezultat=rezultat+255;
				carry=1;
			}
			else 
				carry=0;

		
			promenljiva=rezultat;
	
			for(i=0;i<8;i++)
			{
				rezultat_binarni[i]=promenljiva%2;
				promenljiva=promenljiva/2;
			}
		}
		else 
			printk(KERN_WARNING "Pogresan format unosa");
		
	//	wake_up_interruptible(&writeQ);

	}
	//	wake_up_interruptible(&writeQ);

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
   carry=0;
   format=1;
   flag=0;

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
