hInjector
=========

Lead architect: Aleksandar Milenkoski (aleksandar.milenkoski@uni-wuerzburg.de)

Development: Christoph Sendner (christoph.sendner@stud-mail.uni-wuerzburg.de)


Tested in the following environment: Xen-4.4.1 (Intel 64 bit), a fully paravirtualized virtual machine (Intel 64 bit). 

The virtual machine from where hypercalls are injected has to be a Linux system.

Installation
=========

1) Download the source code of Xen and extract it. In the following, [path] is the absolute path to the location where the source code of Xen is extracted.

2) Patch Xen using the file hInjector.patch.

Example:

$ cd [path]/xen-4.4.1

$ patch -p1 < ../hInjector.patch

3) Recompile and install Xen on your system. Reboot. 

4) Create a virtual machine, log in, and copy the hInjector folder on the hard disk of the virtual machine.  

5) Access the hInjector folder and type "make".

6) Make sure that the structure "arch_shared_info" used by the kernel of the virtual machine is identical to the structure "arch_shared_info" used by the Xen kernel. The best way to do this is to open the files where the structures are declared, compare the declarations of "arch_shared_info", and make changes if necessary. 

Example:

Location of "arch_shared_info" in the Linux kernel: /usr/src/linux-header-$(uname -r)/arch/x86/include/asm/xen/interface.h

Location of "arch_shared_info" in the Xen kernel: [path]/xen-4.4.1/xen/include/public/arch-x86/xen.h 


