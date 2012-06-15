
#define _GNU_SOURCE 1 /* for feenableexcept */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <fenv.h>

void print_si(siginfo_t *si)
{
	printf("signo = %d\n", si->si_signo);
	printf("errno = %d\n", si->si_errno);
	printf("code = %d\n", si->si_code);
	
	switch (si->si_signo) {
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	case SIGTRAP:
		if (si->si_code != SI_USER) {
			printf("addr = %p\n", si->si_addr);
		}
		break;
	case SIGCHLD:
		printf("pid = %lu\n", (unsigned long)si->si_pid);
		printf("uid = %lu\n", (unsigned long)si->si_uid);
		printf("status = %lu\n", (unsigned long)si->si_status);
		printf("utime = %lu\n", (unsigned long)si->si_utime);
		printf("stime = %lu\n", (unsigned long)si->si_stime);
		break;
	case SIGIO: /* == SIGPOLL */
		printf("fd = %d\n", si->si_fd);
		break;
	default:
		if (si->si_code == SI_QUEUE || si->si_code == SI_USER) {
			printf("pid = %lu\n", (unsigned long)si->si_pid);
			printf("uid = %lu\n", (unsigned long)si->si_uid);
			if (si->si_code == SI_QUEUE) {
				printf("int = %d\n", si->si_int);
				printf("ptr = %p\n", si->si_ptr);
			}
		}
		break;
	}
}

void handler(int sig, siginfo_t *si, void *unused)
{
	print_si(si);
	exit(0);
}

void setup(int sig)
{
	struct sigaction sa;
	sa.sa_sigaction = handler;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(sig, &sa, NULL);
}

void cause_failed()
{
	printf("Failed to cause signal\n");
	exit(1);
}


void test_sigsegv()
{
	printf("Testing SIGSEGV\n");
	setup(SIGSEGV);
	int *p = (void*) 0x42;
	*p = 123;
	cause_failed();
}

void test_sigfpe_intdiv()
{
	printf("Testing SIGFPE/FPE_INTDIV\n");
	setup(SIGFPE);
	printf("%d", 3 / 0);
	cause_failed();
}

void test_sigfpe_fltdiv()
{
	printf("Testing SIGFPE/FPE_FLTDIV\n");
	setup(SIGFPE);
	feenableexcept(FE_ALL_EXCEPT);
	printf("%f", 3.0f / 0.0f);
	cause_failed();
}

void test_sigill()
{
	printf("Testing SIGILL\n");
	setup(SIGILL);
	const static unsigned char code[4] = { 0xff, 0xff, 0xff, 0xff };
	void (*func)() = (void (*)()) code;
	func();
	cause_failed();
}

void test_kill_sigterm()
{
	printf("Testing kill(self, SIGTERM)\n");
	setup(SIGTERM);
	kill(getpid(), SIGTERM);
	cause_failed();
}

void test_kill_sigusr1()
{
	printf("Testing kill(self, SIGUSR1)\n");
	setup(SIGUSR1);
	kill(getpid(), SIGUSR1);
	cause_failed();
}

void test_kill_sigfpe()
{
	printf("Testing kill(self, SIGFPE)\n");
	setup(SIGFPE);
	kill(getpid(), SIGFPE);
	cause_failed();
}

void test_sigio()
{
	int fds[2];
	pid_t pid;
	
	printf("Testing SIGIO\n");
	
	if (pipe(fds) == -1) {
		printf("pipe() failed: %s\n", strerror(errno));
		exit(1);
	}
	
	setup(SIGIO);
	
	if (fcntl(fds[0], F_SETFL, (long)O_ASYNC) != 0) {
		printf("fcntl() F_SETFL failed: %s\n", strerror(errno));
		exit(1);
	}
	
	if (fcntl(fds[0], F_SETOWN, (long)getpid()) != 0) {
		printf("fcntl() F_SETOWN failed: %s\n", strerror(errno));
		exit(1);
	}
	
	if (fcntl(fds[0], F_SETSIG, (long)SIGIO) != 0) {
		printf("fcntl() F_SETSIG failed: %s\n", strerror(errno));
		exit(1);
	}
	
	pid = fork();
	if (pid == 0) {
		write(fds[1], "hello", 6);
		close(fds[1]);
		_exit(0);
	} else if (pid < 0) {
		printf("fork() failed: %s\n", strerror(errno));
		exit(1);
	}
	
	close(fds[1]);
	
	sleep(2);
	
	cause_failed();
}

typedef void (*test_func_t)();

test_func_t tests[] = {
	test_sigsegv,
	test_sigfpe_intdiv,
	test_sigfpe_fltdiv,
	test_sigill,
	test_kill_sigterm,
	test_kill_sigusr1,
	test_kill_sigfpe,
	test_sigio,
	NULL
};

int test_count()
{
	int c = 0;
	int i;
	for (i = 0; tests[i] != NULL; ++i) {
		c += 1;
	}
	return c;
}

int main()
{
	int i;
	pid_t pid;
	int status;
	
	for (i = 0; tests[i] != NULL; ++i) {
		pid = fork();
		if (pid == 0) {
			tests[i]();
			_exit(0);
		} else {
			if (pid < 0) {
				printf("Failed to fork: %s\n", strerror(errno));
				exit(1);
			}
			if (waitpid(pid, &status, 0) != pid) {
				printf("Failed to wait for child process: %s", strerror(errno));
				exit(1);
			}
			printf("\n");
		}
	}
	return 0;
}
