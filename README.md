# Bthread - A basic user-level thread library for Nanvix

The Bthread library was created for the Interdisciplinary Project V subject
of the computer science course at PUC Minas with educational purposes in mind,
such as, helping students understand how concurrent programs work
as well as providing ways for the developing of concurrent programming practices.

## About Bthread

### Features
The Bthread library implements a preemptive aproach with round-robin 
and provides the following features:
Thread creation and termination
* Thread joining and detaching
* Thread cancellation
* Thread yield
* Mutexes

### API
The Bthread library API is composed of the following functions:

```c
int bthread_create(bthread_t *thread, void *(*start_routine)(), void *arg);
int bthread_join(bthread_t thread, void **thread_return);
int bthread_detach(bthread_t thread);
void bthread_yield(void);
bthread_t bthread_self(void);
int bthread_cancel(bthread_t thread);
struct bt_mutex *bthread_mutex_init(void);
int bthread_mutex_lock(struct bt_mutex *mutex);
int bthread_mutex_unlock(struct bt_mutex *mutex);
int bthread_mutex_destroy(struct bt_mutex *mutex);
```

For more information about the Bthread library API, please refer to the [bthread.h](include/bthread.h) header file.

### Usage
To use the Bthread library in a Nanvix program, the following steps must be taken:

1. Setup the Nanvix development environment as described at [setup.md](doc/setup.md).
2. Create a new user program in the `src/ubin` directory.
3. Include the `bthread.h` header file in the user program.
4. Build Nanvix as described at [build.md](doc/build.md).
5. Run Nanvix using your choosen system simulator (QEMU is recommended).

### Example
A simple example of the Bthread library usage is available in the user program `bthd` in Nanvix,
its source code is available at [src/ubin/bthd/bthd.c](src/ubin/bthd/bthd.c).

## About Nanvix

### What Is Nanvix?

[Nanvix](https://github.com/nanvix/nanvix) is a Unix-like operating system written by [Pedro Henrique
Penna](https://github.com/ppenna) for educational purposes. It is designed to be
simple and small, but also modern and fully featured.

### What Hardware Is Required to Run Nanvix?

Nanvix targets 32-bit x86-based PCs and only requires a floppy or
CD-ROM drive and 16 MB of RAM. You can run it either in a real PC
or in a virtual machine, using a system image.

## License and Maintainers

Nanvix is a free software that is under the GPL V3 license and is
maintained by Pedro Henrique Penna. Any questions or suggestions send him an
email: <pedrohenriquepenna@gmail.com>

### BThread

BThread is also under the GPL V3 license and was made by:

- [Arthur Ruiz](https://github.com/ArthurSRuiz)
- [Edmar Melandes](https://github.com/Lexizz7)
- [Marco Aurélio Noronha](https://github.com/marconoronha)
- [Pedro Pampolini](https://github.com/PedroPampolini)
- [Vinicius Gabriel Santos](https://github.com/ravixr)
