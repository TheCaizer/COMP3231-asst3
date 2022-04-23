/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 *
 * UNSW: If you use ASST3 config as required, then this file forms
 * part of the VM subsystem.
 *
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	*/
    // set head as null and malloc the page table
    as->head = NULL;
    as->pagetable = kmalloc(sizeof(paddr_t **) * PT_FIRST_SIZE);
    // Did not set pagetable therefore no mem or error so free and return
    if(as->pagetable == NULL){
        kfree(as->pagetable);
        kfree(as);
        return NULL;
    }
    // Initialize first level
    for(int i = 0; i < PT_FIRST_SIZE; i++){
        as->pagetable[i] = NULL;
    }
     
	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	//copy pagetable
	newas->pagetable = kmalloc(2048*sizeof(*paddr_t));
	for(i=0; i < 2048; i++){
		if old->pagetable[i] != 0:
			newas->pagetable[i] =  kmalloc(512*sizeof(paddr_t));
			for(j = 0; j < 512; j++){
				newas->pagetable[i][j] = old->pagetable[i][j]
			}
	}

	if(old->region_head == NULL){
		newas->region_head = NULL;
	}else{
		//copy region linked list
		newas->region_head = kmalloc(sizeof(region));
		newas->region_head->base= old->region_head->base
		newas->region_head->permission = old->region_head->permission
		newas->region_head->size = old->region_head->size
		region *curOldNode = old->region_head->next
		region *curNode = newas->region_head
		while(curOldNode!= NULL){
			curNode->next = kmalloc(sizeof(region));
			curNode->next->base= curOldNode->base
			curNode->next->permission = curOldNode->permission
			curNode->next->size = curOldNode->size

			curNode = curNode->next
			curOldNode = curOldNode->next

		}

		curNode->next = NULL
	}
	//(void)old;

	addrspace **ret = &newas;
	return ret;
}

void
as_destroy(struct addrspace *as)
{
	region *curNode = as->region_head
	region *temp = NULL
	while(curNode != NULL){
		temp = curNode->next 
		kfree(curNode)
		curNode = temp
	}
	for(i = 0; i < PT_FIRST_SIZE){
		if (as->pagetable[i] != 0){
			for(j = 0; j < PT_SECOND_SIZE2; j++){
				kfree(as->pagetable[i][j])
			}
			kfree(as->pagetable[i])
		}
		
	}

	kfree(as);
}

void
as_activate(void)
{
    // FROM dumbvm
	int i, spl;
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable)
{
	/*
	 * Write this.
	 */
    if(as == NULL){
        return EFAULT;
    }

}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	(void)as;

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	return 0;
}

