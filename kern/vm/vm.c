#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */
// Need to add functions to insert (Izaac), lookup (Jackie) and update (Izaac) page table entries 
// Need to do vm_fault (Izaac)

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
    (void) faultaddress;

    if(faulttype = VM_FAULT_READONLY){
        return EFAULT;
    }else{
        if(lookup(faultaddress) == -1){
            addrspace *as = proc_getas()
            region *curNode = as->region_head
            while(curNode != NULL){
                if(faultaddress >= curNode->base && 
                    faultaddress >= (curNode->base+curNode->size)
            }
        }

    }

    panic("vm_fault hasn't been written yet\n");

    return EFAULT;
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

vm_ptable_insert(unsigned int npages){
    vaddr = alloc_kpages(npages)

}
