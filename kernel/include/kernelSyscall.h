#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include "kernelServer.h"
#include "cpuKernel.h"

void* hilo_syscalls_cpu(void* arg);
void procesar_syscall_io(t_package_io* dispositivo_io, t_log* logger);
const char* syscall_recibida(int modulo);
void procesar_syscall_init_proc(t_log* logger, char* archivo, uint32_t tamanio );
void procesar_syscall_exit(t_log* logger, t_package_syscall_exit* syscallExit);
void procesar_syscall_dump_memory(t_log* logger, t_package_syscall_exit* syscallDump);

#endif