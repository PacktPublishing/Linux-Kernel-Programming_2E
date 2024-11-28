# Linux Kernel Programming, Second Edition
This is the code repository for [Linux Kernel Programming, Second Edition](https://www.packtpub.com/product/linux-kernel-programming-second-edition/9781803232225), published by Packt.

**A comprehensive and practical guide to kernel internals, writing modules, and kernel synchronization**

The author of this book is - [Kaiwan N. Billimoria](https://in.linkedin.com/in/kaiwanbillimoria) 
## About the book

The 2nd Edition of Linux Kernel Programming is an updated, comprehensive guide for new programmers to the Linux kernel. This book uses the recent 6.1 Long-Term Support (LTS) Linux kernel series, which will be maintained until Dec 2026, and also delves into its many new features. Further, the Civil Infrastructure Project has pledged to maintain and support this 6.1 Super LTS (SLTS) kernel right until August 2033, keeping this book valid for years to come!

You’ll begin this exciting journey by learning how to build the kernel from source. In a step by step manner, you will then learn how to write your first kernel module by leveraging the kernel’s powerful Loadable Kernel Module (LKM) framework. With this foundation, you will delve into key kernel internals topics including Linux kernel architecture, memory management, and CPU (task) scheduling. You’ll finish with understanding the deep issues of concurrency, and gain insight into how they can be addressed with various synchronization/locking technologies (e.g., mutexes, spinlocks, atomic/refcount operators, rw-spinlocks and even lock-free technologies such as per-CPU and RCU).

By the end of this book, you’ll have a much better understanding of the fundamentals of writing the Linux kernel and kernel module code that can straight away be used in real-world projects and products.

## Key Takeaways
- Configure and build the 6.1 LTS kernel from source
- Write high-quality modular kernel code (LKM framework) for 6.x kernels
- Explore modern Linux kernel architecture
- Get to grips with key internals details regarding memory management within the kernel
- Understand and work with various dynamic kernel memory alloc/dealloc APIs
- Discover key internals aspects regarding CPU scheduling within the kernel, including cgroups v2
- Gain a deeper understanding of kernel concurrency issues
- Learn how to work with key kernel synchronization primitives

## Outline and Chapter Summary

This book, LKP 2E, is an ideal guide to Linux kernel development for programmers new to it, as well as for experienced developers. It incorporates a judicious mix of theory and practice, with more focus on practical aspects. It deliberately moves away from bland and generic 'OS theory' topics and instead covers them where required from the clear and distilled viewpoint of implementation details in a real, live OS - Linux.
The book's chapters are as follows:

1. Linux Kernel Programming – A Quick Introduction
    - [Online chapter (PDF)](http://www.packtpub.com/sites/default/files/downloads/9781803232225_Online_Chapter.pdf)
2. Building the 6.x Linux Kernel from Source – Part 1
3. Building the 6.x Linux Kernel from Source – Part 2
4. Writing Your First Kernel Module – Part 1
5. Writing Your First Kernel Module – Part 2
6. Kernel Internals Essentials – Processes and Threads
7. Memory Management Internals – Essentials
8. Kernel Memory Allocation for Module Authors – Part 1
9. Kernel Memory Allocation for Module Authors – Part 2
10. The CPU Scheduler – Part 1
11. The CPU Scheduler – Part 2
12. Kernel Synchronization – Part 1
13. Kernel Synchronization – Part 2


> If you feel this book is for you, get your [copy](https://www.amazon.com/Linux-Kernel-Programming-practical-synchronization/dp/1803232226) today! <img alt="Coding" height="15" width="35"  src="https://media.tenor.com/ex_HDD_k5P8AAAAi/habbo-habbohotel.gif">


With the following software and hardware list you can run all code files present in the book.

## Software and hardware list

These are the Linux platforms upon which the book's codebase has been developed and tested:

- x86_64 Ubuntu 22.04 LTS native and guest OS
- x86_64 Ubuntu 23.04 LTS native and guest OS
- x86_64 Fedora 38 (and 39) on a native and guest OS 
    - All guest OS's have been run upon the host Oracle VirtualBox 7.0 hypervisor
- ARM Raspberry Pi 4 Model B (64-bit, running both its “distro” kernel as well as our custom 6.1 kernel); lightly tested

## Known Errata

- page 9:
    - The bullet point "4.0 to 4.19 (20 minor releases)" should be "4.0 to 4.20 (21 minor releases)"; it turns out it's an exception to the rule of '20 minor releases per major rel' (hey, every good rule has one!). (Similarly for *Figure 2.1* as well.)
    - The phrase 'the 6.1 kernel is a Long-Term Stable (LTS) version', should be
    'the 6.1 kernel is a Long-Term *Support* (LTS) version'.

 - page 34:
    - replace "$ cat Kconfig" with "$ cat Kbuild"

 - page 85, 86:
    - In the section *Locating the kernel modules within the kernel source*, it's mentioned that modules always have a .ko suffix. While usually true, a config setting can force modules to compressed (typically via ZSTD), thus making the name /lib/modules/<...>/foo.ko**.zst** and not foo.ko. This ZSTD module compression has been added [from 5.13](https://www.phoronix.com/news/Linux-5.13-Zstd-Modules), and some distros might enable it to be the default. If you prefer uncompressed modules (as the book often assumes), you can set module compression to 'none' via make menuconfig > Enable loadable module support > Module compression mode (None). (Thanks to @redbilledpanda @GitHub for pointing this out!)

 - page 197:
    - In the command-box under this line: "1.
Attempt the build (for a second time) with the make command appropriate for cross-compilation:", the command to cross-compile is mistakenly shown for the 32-bit build (aka 1st Ed!); so, replace the command <br>
    `$ make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-` <br>
with<br>
    `$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-`


 - page 264:
    - Added the ch6/countem2.sh Bash script to the repo; it complements the ch6/countem.sh script, adding on a few details

- Ch 13
    - *Sincere apologies!* I've made several (small-ish) mistakes here! (please read on...)
    - *Figure 13.14* - the circled numbers mentioned in the book are missing. The reason: I eventually removed the circled numbers from *Figure 13.14* as it was already a bit too crowded, but inadvertently missed updating the text; apologies, my mistake! So, **please ignore** the following:
        - page 716:
            - The sentences " (See *Figure 13.14* and the (red) circle with 2 in it.)", "(See *Figure 13.14* and the (red) circle with 3 in it.)" and " (See *Figure 13.14* and the (red) circle with 3 in it.)".
        - page 718:
            - The sentence " (See *Figure 13.14*, with the (red) circles with 4 and 5 in them, respectively.".
        - page 720:
            - The phrases "(see *Figure 13.14* and the (red) circle with 1 in it)".
        - page 721:
            - The phrase " (see *Figure 13.14* and the (red) circle with 2 in it)" and "(see *Figure 13.14* and the (red) circle with 3 in it)".
        - page 722:
            - The phrase "(see *Figure 13.14* and the (red) circles with 4 and 5 in them, respectively)".

    - page 716:
        - "*Figure 13.12: A writer thread comes along and creates a copy of the data object (notice that it’s begun to modify its content); the vertical line – once a writer enters – separates the old
RCU readers (1, 2, and 3) from the new one (4)*" is incorrect. It should read:   
"*Figure 13.12: A writer thread comes along and creates a copy of the data object (notice that
it’s begun to modify its content)*"
        - The phrases "and any subsequent “new” reader (RCU readers 4)", "and any subsequent “new” reader (like RCU readers 4 and 5 in *Figure 13.13*)" should be:  
         "and any subsequent “new” reader (RCU readers 5)", "and any subsequent “new” reader (like RCU reader 5 in *Figure 13.13*)"
    - page 717:
        - "*Figure 13.13: The pre-existing RCU readers (1 to 3) refer to the old reality; the writer and new readers (4 and 5) see the new reality*" should be:  
"*Figure 13.13: The pre-existing RCU readers (1 to 4) refer to the old reality; the writer and
new reader (5) see the new reality*"
    - page 718:
        - The sentence "(In this particular example, even the new RCU reader 4 has finished and context-switched; it can happen. Note though that it’s reading the new object.)" is just wrong. RCU reader 4 is an 'old' reader
    - page 719:
        - "The point by which all the older (pre-existing) RCU readers (here, 1 to 3) finish and context-switch off their core is shown via the thick vertical line. (In this particular example, even the new RCU reader 4 has finished and context-switched; it can happen. Note though that it’s reading the new object.)" should read: 
"At some point all the older (pre-existing) RCU readers (here, 1 to 4) finish and context-switch off their cores."
    - page 726:
        - whoops, fix the potential deadlock bug!
            - `if (!gd_new) { *spin_unlock(&gdata_lock);* return -ENOMEM; }`
        - whoops again! `fix:writer makes copy for RCU & should free the old object only`
            - see commit # `70dd634`
            - as well, in *Figure 13.16* right side bottom: replace the `kfree(gd_new);` with `kfree(gd);`
<br>

## Know more on the Discord server <img alt="Coding" height="25" width="32"  src="https://cliply.co/wp-content/uploads/2021/08/372108630_DISCORD_LOGO_400.gif">
You can get more engaged on the discord server for more latest updates and discussions in the community at [Discord](https://packt.link/SecNet)

## Download a free PDF <img alt="Coding" height="25" width="40" src="https://emergency.com.au/wp-content/uploads/2021/03/free.gif">

_If you have already purchased a print or Kindle version of this book, you can get a DRM-free PDF version at no cost. Simply click on the link to claim your free PDF._
[Free-Ebook](https://packt.link/free-ebook/9781803232225) <img alt="Coding" height="15" width="35"  src="https://media.tenor.com/ex_HDD_k5P8AAAAi/habbo-habbohotel.gif">

We also provide a PDF file that has color images of the screenshots/diagrams used in this book at [GraphicBundle]( https://packt.link/gbp/9781803232225) <img alt="Coding" height="15" width="35"  src="https://media.tenor.com/ex_HDD_k5P8AAAAi/habbo-habbohotel.gif">


## Get to know the Author
_Kaiwan N. Billimoria_ taught himself BASIC programming on his dad's IBM PC back in 1983. He was programming in C and Assembly on DOS until he discovered the joys of Unix, and by around 1997, Linux!
Kaiwan has worked on many aspects of the Linux system programming stack, including Bash scripting, system programming in C, kernel internals, device drivers, and embedded Linux work. He has actively worked on several commercial/FOSS projects. His contributions include drivers to the mainline Linux OS and many smaller projects hosted on GitHub. His Linux passion feeds well into his passion for teaching these topics to engineers, which he has done for well over two decades now. He's also the author of Hands-On System Programming with Linux, Linux Kernel Programming (and its Part 2 book) and Linux Kernel Debugging. It doesn't hurt that he is a recreational ultrarunner too.

## Other Related Books
- [The Software Developer’s Guide to Linux](https://www.packtpub.com/product/the-software-developers-guide-to-linux/9781804616925)
- [Mastering Linux Security and Hardening - Third Edition](https://www.packtpub.com/product/mastering-linux-security-and-hardening-third-edition/9781837630516)

