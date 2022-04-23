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

    if(faulttype = VM_FAULT_READONLY){
        return EFAULT;
    }
    if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	struct addrspace *as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}
    faultaddress &= PAGE_FRAME;

    paddr = lookup(faultaddress)
    if(paddr == 0){
        addrspace *as = proc_getas()
        region *curNode = as->region_head
        int valid = 0 
        while(curNode != NULL){
            if(faultaddress >= curNode->base && 
                faultaddress <= (curNode->base+curNode->size)){
                    valid = 1;
                    break;
            }
            curNode = curNode->next;
        }
        if(valid == 1){
            //needs fixing
            vm_ptable_insert(1)
        }else{
            return EFAULT;
        }
    }else{
        tlb_random(paddr, faultaddress|TLBLO_DIRTY |TLBLO_VALID)
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

vaddr_t vm_ptable_insert(unsigned int npages){
    vaddr = alloc_kpages(npages);
    paddr = KVADDR_TO_PADDR(vaddr);
    int firstIndex = vaddr>>21;
    int secondIndex = (vaddr>>21) & 0x1FF;
    if(as->page[firstIndex] == 0){
        as->page[firstIndex] = kmalloc(512*sizeof(paddr_t));
        for(j = 0; j < 512; j++){
            as->pagetable[firstIndex][j] = 0;
        }
    }
    as->page[firstIndex][secondIndex] = paddr<<12;

    return vaddr;

}

void vm_ptable_update(vaddr_t vaddr){
    
}

paddr_t vm_ptable_lookup(vaddr_t vaddr){
    addrspace *as = proc_getas();
    int firstIndex = vaddr>>21;
    int secondIndex = (vaddr>>21) & 0x1FF;
    if(as->page[firstIndex] == 0){
        return 0;
    }else if (as->page[firstIndex][secondIndex] == 0){
        return 0;
    }
    return as->page[firstIndex][secondIndex];
}
