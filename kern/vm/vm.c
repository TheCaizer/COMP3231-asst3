#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>
#include <current.h>
#include <proc.h>
#include <spl.h>

/* Place your page table functions here */
int insert_pt(vaddr_t page, paddr_t frame, struct addrspace *as);
int check_valid_region(vaddr_t page, struct addrspace *as);
paddr_t lookup_pt(uint32_t page, struct addrspace *as);

void vm_bootstrap(void)
{
    /* Initialise any global components of your VM sub-system here.  
     *  
     * You may or may not need to add anything here depending what's
     * provided or required by the assignment spec.
     */
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    // Check read_only
    if(faulttype == VM_FAULT_READONLY){
        return EFAULT;
    }
    // Check invalid address
    if(faultaddress == 0x0){
        return EFAULT;
    }
    if(curproc == NULL){
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
        return EFAULT;
    }

	struct addrspace *as = proc_getas();
	if (as == NULL || as->region_head == NULL || as->pagetable == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

    //FROM dumbvm
    faultaddress &= PAGE_FRAME;
    // Look up the page table
    paddr_t f_addr = lookup_pt(faultaddress, as);
    // Zero meaning NULL first level
    if(f_addr == 0){
        // check if the region is valid
        int ret = check_valid_region(faultaddress, as);
        if(ret){
            return ret;
        }
        vaddr_t vBase = alloc_kpages(1);
        // alloc_kpages failed
        if(vBase == 0){
            return ENOMEM;
        }
        // convert the vBAse and zero the mems then insert into the pagetable
        f_addr = KVADDR_TO_PADDR(vBase);
        bzero((void *) vBase, PAGE_SIZE);
        ret = insert_pt(faultaddress, f_addr, as);
        if(ret){
            return ret;
        }
    }
    // get hi and lo
    uint32_t hi = faultaddress & TLBHI_VPAGE;
    uint32_t lo = f_addr | TLBLO_DIRTY | TLBLO_VALID;
	/* Disable interrupts on this CPU while frobbing the TLB. */
    int spl = splhigh();
    tlb_random(hi, lo);
    splx(spl);
    return 0;
}

/*
 * SMP-specific functions.  Unused in our UNSW configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("vm tried to do tlb shootdown?!\n");
}

// Function to check if the region is valid
int check_valid_region(vaddr_t page, struct addrspace *as){
    struct region *curNode =  as->region_head;
    while(curNode != NULL){
        vaddr_t base = curNode->base;
        vaddr_t top = curNode->base + (curNode->size * PAGE_SIZE);
        if(page >= base && page < top){
            return 0;
        }
        curNode = curNode->next;
    }
    return EFAULT;
}
// Function to lookup the pagetable
paddr_t lookup_pt(uint32_t page, struct addrspace *as){
    uint32_t firstIndex = page >> 21;
    if(as->pagetable[firstIndex] == NULL){
        return 0;
    }
    uint32_t secondIndex = (page << 11) >> 23;
    return as->pagetable[firstIndex][secondIndex];
}
// function to insert the pagetable
int insert_pt(vaddr_t page, paddr_t frame, struct addrspace *as) {
    uint32_t firstIndex = page >> 21;
    if (as->pagetable[firstIndex] == NULL){
        as->pagetable[firstIndex] = kmalloc(PT_SECOND_SIZE * sizeof(paddr_t *));
        if(as->pagetable[firstIndex] == NULL){
            return ENOMEM;
        }
        for (int i = 0; i < PT_SECOND_SIZE; i++){
            as->pagetable[firstIndex][i] = 0;
        }
    }
    uint32_t secondIndex = (page << 11) >> 23;
    as->pagetable[firstIndex][secondIndex] = frame;
    return 0;
}