# tc-p

tc-p is a simple C++ TCP protocol implementation, to serve as an example to anyone who wants to learn it.

**Useful links:**
1) https://www.rfc-editor.org/rfc/rfc793: This document describes the DoD Standard Transmission Control Protocol (TCP)
2) https://www.youtube.com/watch?v=bzja9fQWzdA: A learning experience in implementing TCP in Rust
3) https://gist.github.com/david-hoze/0c7021434796997a4ca42d7731a7073a: Checksum calculation for IP and TCP header
4) https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html: GCC sanitizer options

**Dependencies:**
1) https://github.com/LaKabane/libtuntap: TunTap C++ wrapper 

**Run:**

To run the program:
```sh
$ ./run.sh
```

To connect the tc-p server:
```sh
$ netcat 192.168.0.2 9000
```
