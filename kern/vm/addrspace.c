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
    // set head as null and malloc the page table
    as->region_head = NULL;
    as->pagetable = kmalloc(sizeof(paddr_t *) * PT_FIRST_SIZE);
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

	//create pagetable
	newas->pagetable = kmalloc(PT_FIRST_SIZE*sizeof(paddr_t *));
    if(newas->pagetable == NULL){
        as_destroy(newas);
        return ENOMEM;
    }

    for(int i = 0; i < PT_FIRST_SIZE; i++){
        if(old->pagetable[i] == NULL){
            newas->pagetable[i] = NULL;
            continue;
        }

        //if there is a value in the old page table assign a new one
        //for the new one to point to a new 2nd level page table
        newas->pagetable[i] = kmalloc(sizeof(paddr_t *) * PT_SECOND_SIZE);
        for(int j = 0;j < PT_SECOND_SIZE;j++){
            if(old->pagetable[i][j] == 0){
                newas->pagetable[i][j] = 0;
            }
            else{
                // Allocate physical frame 
                vaddr_t frame = alloc_kpages(1);
                if(frame == 0){
                    as_destroy(newas);
                    return ENOMEM;
                }
                //set frame
                paddr_t add_frame = KVADDR_TO_PADDR(frame);
                newas->pagetable[i][j] = add_frame;
                // copy mem
                memcpy((void *) frame, (const void*)PADDR_TO_KVADDR(old->pagetable[i][j]), PAGE_SIZE);
            }
        }

    }

	if(old->region_head == NULL){
		newas->region_head = NULL;
	}else{
		//copy region linked list
		newas->region_head = kmalloc(sizeof(struct region));
        if(newas->pagetable == NULL){
            as_destroy(newas);
            return ENOMEM;
        }
        //copy old region_head to new region_head
		newas->region_head->base= old->region_head->base;
		newas->region_head->permission = old->region_head->permission;
		newas->region_head->size = old->region_head->size;

		struct region *curOldNode = old->region_head->next;
		struct region *curNode = newas->region_head;

        //copy data from each old region node
        //into a new region node, stop when old list ends
		while(curOldNode!= NULL){
			curNode->next = kmalloc(sizeof(struct region));
            //return memory error if no page table
            if(newas->pagetable == NULL){
                as_destroy(newas);
                return ENOMEM;
            }
            //copy data
			curNode->next->base= curOldNode->base;
			curNode->next->permission = curOldNode->permission;
			curNode->next->size = curOldNode->size;

            //incrament nodes to next node
			curNode = curNode->next;
			curOldNode = curOldNode->next;

		}

		curNode->next = NULL;
	}

	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
    //free all nodes in region linked list
	struct region *curNode = as->region_head;
	struct region *temp = NULL;


	while(curNode != NULL){
		temp = curNode->next ;
		kfree(curNode);
		curNode = temp;
	}

    //free all pagetable entries and pagetable itself
    if(as->pagetable != NULL){
        for(int i = 0; i < PT_FIRST_SIZE; i++){
            if (as->pagetable[i] != NULL){
                for(int j = 0; j < PT_SECOND_SIZE; j++){
                    if(as->pagetable[i][j] != 0){
                        free_kpages(PADDR_TO_KVADDR(as->pagetable[i][j]));
                    }
                }
                kfree(as->pagetable[i]);
            }
            
        }
        kfree(as->pagetable);
    }
    // free address space
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
     // same as as_activate
	int spl = splhigh();

	for (int i=0; i<NUM_TLB; i++) {
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
     // Check not NULL
    if(as == NULL){
        return EFAULT;
    }
    // Check it is valid region
    if(vaddr + memsize > 0x80000000){
        return EFAULT;
    }
    // make new region
    struct region *new = kmalloc(sizeof(struct region));
    if(new == NULL){
        return ENOMEM;
    }
    // Set base, size, the persmissions
    new->permission = 0;
    if(readable){
        new->permission |= READ;
    }
    if(writeable){
        new->permission |= WRITE;
    }
    if(executable){
        new->permission |= EXECUTE;
    }
    new->base = vaddr;
    new->size = memsize;
    new->next = as->region_head;
    as->region_head = new;
    return 0;
}

int
as_prepare_load(struct addrspace *as)
{
     // Check for err
    if(as == NULL){
        return EFAULT;
    }
    struct region *curNode = as->region_head;

    while(curNode != NULL){
        // Check curNode is READ_ONLY or ReadExecute, not sure if we need to also
        // include execute only
        if(curNode->permission == READ || curNode->permission == (READ & EXECUTE)){
            // set to writable
            curNode->permission = curNode->permission | WRITE;
            // set the 4th bith to 1 so that we know this a one that was changed 
            curNode->permission = curNode->permission | 0x8;
        }
        curNode = curNode->next;
    }
    return 0;
}

int
as_complete_load(struct addrspace *as)
{
    //check for err
	if(as == NULL){
        return EFAULT;
    } 
    struct region *curNode = as->region_head ;
    while(curNode != NULL){
        // check if the 4th bit is set
        if(curNode->permission & 0x8){
            // remove the write and 4th bit so its the old permission
            // shift 1 to the second bit flip the bits and do an and function
            curNode->permission &= ~(1 << 1);
            // shift 1 to the fouth bit flip the bits and do an and function
            curNode->permission &= ~(1 << 3);
            // this should result into the origninal read or execute bits
            // by setting write to 0 and 4th bit to 0
        }
        curNode = curNode->next;
    }
    // flush tlb
	int spl = splhigh();

	for (int i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
    return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
    // get size of the stack
    size_t size = NUM_STACK * PAGE_SIZE;
    // adress of the stack base
    vaddr_t stack = USERSTACK - size;
    // define the stack and make it so it is read and write
    int ret = as_define_region(as, stack, size, 1, 1, 0);
    if(ret){
        return ret;
    }
    /* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	return 0;
}

