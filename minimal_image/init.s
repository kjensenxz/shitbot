; init.s - simple init for x86_64 linux under QEMU
; Copyright (C) 2021 Kenneth B. Jensen. All rights reserved.
; This program is licensed under the GNU GPLv3 or later.
; Credit to Jeff Marrison of 2 Ton Digital for the sleep macro.

; This program mounts the necessary filesystems and forks.
; The child exec()s getty for /bin/sh.
; The parent waits for the child and pokes /dev/watchdog every second.
; Theoretically, when the child exits, /dev/watchdog will not be updated
; and QEMU will exit.

BITS    64
CPU     x64

%define EXIT_FAILURE 0x01

; see /usr/include/bits/waitflags.h
%define WNOHANG 0x01

; see /usr/include/asm-generic/fcntl.h
%define O_RDWR 0x02

; see linux/arch/x86/entry/syscalls/syscall_64.tbl
%define sys_write       0x01
%define sys_open        0x02
%define sys_nanosleep   0x23
%define sys_fork        0x39
%define sys_execve      0x3B
%define sys_exit        0x3C
%define sys_wait4       0x3D
%define sys_mkdir       0x53
%define sys_mount       0xA5

; usage: mkmount path, fs_type, mode
%macro mkmount 3
	mov     rdi, %1         ; path
	mov     esi, %3         ; mode
	mov     eax, sys_mkdir
	syscall

	xor     rdi, rdi        ; device name
	mov     rsi, %1         ; mount point
	mov     rdx, %2         ; fs type
	xor     r10, r10        ; flags
	xor     r8, r8          ; data
	mov     eax, sys_mount
	syscall
%endmacro

; Copyright (C) 2015-2018 2 Ton Digital
; This macro stolen/ported from HeavyThing x86_64 assembly language
; library/showcase program (see sleeps.inc)
; Homepage: https://2ton.com.au/
; Author: Jeff Marrison <jeff@2ton.com.au>
; Licensed under the GNU GPLv3 or later.
; (see <http://www.gnu.org/licenses/>
;
; @param: time in seconds to sleep
%macro sleep 1
	sub     rsp, 16           ; give us a stack
	mov     qword [rsp], %1   ; tv_sec
	mov     qword [rsp+8], 0  ; tv_nsec
	mov     rdi, rsp          ; req
	xor     esi, esi          ; rem
	mov     eax, sys_nanosleep
	syscall
	add     rsp, 16           ; give it back
%endmacro

; =================================

section         .text
global          _start

_start:
_init_begin:
_init_mount:
	mkmount proc_path, proc_fstype, 755o
	mkmount sys_path,  sys_fstype,  755o
	mkmount tmp_path,  tmp_fstype,  777o

_init_fork:
	mov     eax, sys_fork
	syscall

	; bad fork!
	cmp     eax, -1
	je      _panic_

	; good fork; child process
	cmp     eax, 0
	je      _init_child

	; keep child pid in parent...
	mov     r13d, eax

_init_parent:
	mov    rdi, watchdog_path       ; path
	mov    esi, O_RDWR              ; flags
	xor    edx, edx                 ; mode
	mov    eax, sys_open            
	syscall
	
	; bad open!
	cmp    eax, -1
	je     _panic_

	mov    r12d, eax     ; file descriptor from open

_check_childpid:
	mov    edi, r13d        ; child pid
	xor    esi, esi         ; wstatus
	mov    edx, WNOHANG     ; options
	xor    r10d, r10d       ; rusage
	mov    eax, sys_wait4
	syscall

	; bad wait4!
	cmp    eax, -1
	je     _panic_
	
	; no change
	cmp    eax, 0
	je     _poke_watchdog

	; child has changed state (probably exited...)
	;
	; the correct way to do this would be compare the
	; wait4 return with our pid in r13d, but this works.
	jmp    _panic_

_poke_watchdog:
	mov    edi, r12d        ; file descriptor
	xor    rsi, null_string ; should be '\0'
	mov    rdx, 1           ; count
	mov    eax, sys_write
	syscall
	
	; bad write!
	cmp    eax, -1
	je     _panic_

	sleep   10

	; bad nanosleep!
	cmp     eax, -1
	je      _panic_

	jmp     _check_childpid

_panic_:
	mov     edi, EXIT_FAILURE
	mov     eax, sys_exit

_init_child:
_run_getty:
; exec /bin/getty -n -l /bin/sh 9600 ttyS0
; launching /bin/sh as init will give an error:
;      "/bin/sh: can't access tty; job control turned off"
; To the best of my understanding, /dev/tty is a virtual terminal
; identical to the current terminal.
; It is not set up if you're on a raw TTY; a serial connection in this
; case. /dev/tty must be set up by getty... I think. this fixes it.
	
	mov     rdi, getty_path         ; filename
	mov     rsi, getty_argv         ; argv
	xor     edx, edx                ; envp, not using it.
	mov     eax, sys_execve
	syscall

	; getty exited...
	jmp     _panic_

; =================================
section         .rodata

proc_path:
	db      "/proc", 0
proc_fstype:
	db      "proc", 0

sys_path:
	db      "/sys", 0
sys_fstype:
	db      "sys", 0

tmp_path:
	db      "/tmp", 0
tmp_fstype:
	db      "ramfs", 0

watchdog_path:
	db      "/dev/watchdog", 0
null_string:
	db      0

getty_path:
	db      "/bin/getty", 0
getty_arg1:
	db      "-n", 0
getty_arg2:
	db      "-l", 0
getty_arg3:
	db      "/bin/sh", 0
getty_arg4:
	db      "115200", 0
getty_arg5:
	db      "ttyS0", 0
getty_argv:
	dq      getty_path, getty_arg1, getty_arg2, \
	        getty_arg3, getty_arg4, getty_arg5, 0
