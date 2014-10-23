hInjector
=========

hInjector is a customizable framework for injecting hypercall attacks during regular operation of a partially or fully paravirtualized guest virtual machine in a Xen-based environment. The attacks that can be injected using hInjector conform to the following attack models:

(i) execution of a single hypercall with:

◦ regular parameter value(s) (i.e., regular hypercall), or

◦ parameter value(s) specifically crafted for triggering a given vulnerability, which includes values inside and
outside valid value domains, or

(ii) execution of a series of regular hypercalls in a given order, including:

◦ repetitive execution of a single hypercall, or 

◦ repetitive execution of multiple hypercalls.

We constructed the above attack models based on analyzing publicly disclosed vulnerabilities of Xen’s hypercall handlers. 


-- Lead architect: Aleksandar Milenkoski (aleksandar.milenkoski@uni-wuerzburg.de)

-- Development: Christoph Sendner (christoph.sendner@stud-mail.uni-wuerzburg.de)

-- hInjector was tested in the following environment: Xen-4.4.1 (Intel 64 bit), a fully paravirtualized virtual machine running on Linux Debian (Intel 64 bit). 

Installation
=========

1) Download the source code of Xen and extract it. In the following, [path] is the absolute path to the location where the source code of Xen is extracted.

2) Copy the file hInjector.patch to [path].

3) Patch Xen using the file hInjector.patch:

$ cd [path]/xen-4.4.1

$ patch -p1 < ../hInjector.patch

4) Recompile and install Xen on your system. Reboot. 

5) Create a virtual machine, log in, and copy the hInjector folder on the hard disk of the virtual machine.  

6) Access the hInjector folder and type "make".

7) Make sure that the structure "arch_shared_info" used by the kernel of the virtual machine is identical to the structure "arch_shared_info" used by the Xen kernel. The best way to do this is to open the files where the structures are declared, compare the declarations of "arch_shared_info", and make changes if necessary. 

Example:

Location of "arch_shared_info" in the Linux kernel: /usr/src/linux-header-$(uname -r)/arch/x86/include/asm/xen/interface.h

Location of "arch_shared_info" in the Xen kernel: [path]/xen-4.4.1/xen/include/public/arch-x86/xen.h 

Usage
=========

Run hInjector from the virtual machine from where hypercall attacks should be injected:

$ python hInjector.py <configuration file>

Example:

$ python hInjector.py example.xml 

The used configuration file should be located in the folder "config". This folder already contains a few example configuration files (i.e., default.xml, example.xml, and example2.xml) and configuration files that enable the injection of attacks that trigger the vulnerabilities CVE-2012-3495, CVE-2012-5510, and CVE-2012-5513 (i.e., example_cve_2012_3495.xml, example_cve_2012_5510.xml, and example_cve_2012_5513.xml). If no configuration file is specified, the file "default.xml", stored in the "config" folder, is used. 
