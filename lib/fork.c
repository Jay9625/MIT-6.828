// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if ((!(err & FEC_WR)) || (!(uvpd[PDX(addr)] & PTE_P)) || (!(uvpt[PGNUM(addr)] & PTE_P)) || (!(uvpt[PGNUM(addr)] & PTE_COW)))
		panic("panic at pgfault for faulting access\n");
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	envid_t env = sys_getenvid();
	if ((r = sys_page_alloc(env, PFTEMP, PTE_U | PTE_P | PTE_W)))
		panic("panic at pgfault for %e\n", r);
	memcpy((void *) PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);
	if ((r = sys_page_map(env, (void *) PFTEMP, env, ROUNDDOWN(addr, PGSIZE), PTE_U|PTE_W|PTE_P)))
		panic("panic at pgfault for %e\n", r);
	if ((r = sys_page_unmap(env, (void *) PFTEMP)))
		panic("panic at pgfault for %e\n", r);
	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	// panic("duppage not implemented");
	envid_t envsrc = thisenv->env_id;
	pte_t pte = uvpt[pn];
	void *va = (void *) (pn * PGSIZE);
	if (!((pte & PTE_W) || (pte & PTE_COW))) {
		r = sys_page_map(envsrc, va, envid, va, pte & 0xfff);
		return r;
	}

	if (uvpt[pn] & PTE_SHARE) {
		if((r = sys_page_map(envsrc, va, envid, va, pte & PTE_SYSCALL)) <0 ) 
			return r;
	}

	// set both env's pte
	if ((r = sys_page_map(envsrc, va, envid, va, PTE_U|PTE_P|PTE_COW))) 
		return r;
	if ((r = sys_page_map(envsrc, va, envsrc, va, PTE_U|PTE_P|PTE_COW)))
		return r;
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// panic("fork not implemented");
	int r;
	set_pgfault_handler(pgfault);
	envid_t envid = sys_exofork();
	if (envid < 0) {
		// incorrect id
		panic("panic at fork: %e\n", envid);
	} else if (envid == 0) {
		// child env
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	// parent env
	// copy pages below USTACKTOP to child
	r = 0;
	for (size_t pn = PGNUM(UTEXT); pn < PGNUM(USTACKTOP) && !r; pn++) {
		if ((uvpd[PDX(pn * PGSIZE)] & PTE_P) && (uvpt[pn] & PTE_P))
				r = duppage(envid, pn);
	}
	if (r)
		return r;
		
	// allocate page for child exception stack
	if ((r = sys_page_alloc(envid, (void *) (UXSTACKTOP - PGSIZE), PTE_W|PTE_U|PTE_P)))
		return r;

	// set upcall for child
	extern void _pgfault_upcall();
	if ((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)))
		return r;

	// set RUNNABLE status for child
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)))
		return r;

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
