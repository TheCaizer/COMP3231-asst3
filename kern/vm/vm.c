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
// Need to add functions to insert (Izaac), lookup (Jackie) and update (Izaac) page table entries 
// Need to do vm_fault (Izaac)
int insert_pt(vaddr_t page, paddr_t frame, struct addrspace *as);
int check_valid_region(struct addrspace *as, vaddr_t page);
paddr_t lookup_pt(uint32_t page, struct addrspace *as);
void vm_bootstrap(void)
{
    /* Initialise any global components of your VM sub-system here.  
     *  
     * You may or may not need to add anything here depending what's
     * provided or required by the assignment spec.
     */
}
int check_valid_region(struct addrspace *as, vaddr_t page){
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
paddr_t lookup_pt(uint32_t page, struct addrspace *as){
    uint32_t firstIndex = page >> 21;
    if(as->pagetable[firstIndex] == NULL){
        return 0;
    }
    uint32_t secondIndex = (page << 11) >> 23;
    return as->pagetable[firstIndex][secondIndex];
}
int insert_pt(vaddr_t page, paddr_t frame, struct addrspace *as) {
    uint32_t firstIndex = page >> 21;
    if (as->pagetable[firstIndex] == NULL){
        as->pagetable[firstIndex] = kmalloc(PT_SECOND_SIZE * sizeof(paddr_t *));
        for (int i = 0; i < PT_SECOND_SIZE; i++){
            as->pagetable[firstIndex][i] = 0;
        }
    }
    uint32_t secondIndex = (page << 11) >> 23;
    as->pagetable[firstIndex][secondIndex] = frame;
    return 0;
}
int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    if(faulttype == VM_FAULT_READONLY){
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
    faultaddress &= PAGE_FRAME;
    paddr_t f_addr = lookup_pt(faultaddress, as);
    if(f_addr == 0){
        int ret = check_valid_region(as, faultaddress);
        if(ret){
            return ret;
        }
        vaddr_t vBase = alloc_kpages(1);
        if(vBase == 0){
            return ENOMEM;
        }
        f_addr = KVADDR_TO_PADDR(vBase);
        bzero((void *) vBase, PAGE_SIZE);
        ret = insert_pt(faultaddress, f_addr, as);
        if(ret){
            return EFAULT;
        }
    }
    uint32_t hi = faultaddress & TLBHI_VPAGE;
    uint32_t lo = f_addr | TLBLO_DIRTY | TLBLO_VALID;
    int spl = splhigh();
    tlb_random(hi, lo);
    splx(spl);
    return 0;

    /* Jackie attempt 1
    struct region *curNode = as->region_head;
    while(curNode != NULL){
        if(faultaddress >= curNode->base){
            if(faultaddress < curNode->size + curNode->base){
                break;
            }
        }
        curNode = curNode->next;
    }
    if(curNode == NULL){
        int size = NUM_STACK * PAGE_SIZE;
        if (faultaddress >= USERSTACK && faultaddress <= (USERSTACK - size)) {
            return EFAULT;
        }
    }
    paddr_t base = KVADDR_TO_PADDR(faultaddress);
    uint32_t firstIndex = base >> 21;
    uint32_t secondIndex = (base << 11) >> 23;
    int allocated = 0;
    if(as->pagetable[firstIndex] == NULL){
        as->pagetable[firstIndex] = kmalloc(sizeof(paddr_t) * PT_SECOND_SIZE);
        if(as->pagetable[firstIndex] == NULL){
            return ENOMEM;
        }
        allocated = 1;
    }
    uint32_t dirtyBit = 0;
    if(as->pagetable[firstIndex][secondIndex == 0]){
        struct region *head = as->region_head;
        while(head != NULL){
            if(faultaddress >= (head->base + head->size) * PAGE_SIZE && faultaddress < head->base){
                continue;
            }
            if((head->permission & WRITE) != WRITE){
                dirtyBit = 0;
            }
            else{
                dirtyBit = TLBLO_DIRTY;
                break;
            }
            head = head->next;
        }
        if(head == NULL){
            if(allocated){
                kfree(as->pagetable[firstIndex]);
                return EFAULT;
            }
        }
        vaddr_t virtualBase = alloc_kpages(1);
        if(virtualBase == 0){
            if(allocated){
                kfree(as->pagetable[firstIndex]);
            }
            return ENOMEM;
        }
        bzero((void *)virtualBase, PAGE_SIZE);
        paddr_t pageBase = KVADDR_TO_PADDR(virtualBase);
        as->pagetable[firstIndex][secondIndex] = (pageBase & PAGE_FRAME) | dirtyBit | TLBLO_VALID;
    }
    uint32_t hi = faultaddress & TLBHI_VPAGE;
    uint32_t lo = as->pagetable[firstIndex][secondIndex];
    int spl = splhigh();
    tlb_random(hi, lo);
    splx(spl);
    return 0;
    */
    /* Izzac attempt
    paddr = lookup(faultaddress);
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
    */
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
/*
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
*/