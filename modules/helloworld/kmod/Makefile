MODULENAME=helloworld


obj-m += $(MODULENAME).o

module:
	make -C $(KSRC) M=$(PWD) modules

clean:
	make -C $(KSRC) M=$(PWD) clean


