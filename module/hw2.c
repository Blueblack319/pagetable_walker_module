#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/sched.h>

#define PROC_NAME "hw2"

MODULE_AUTHOR("Yang, Jaehoon");
MODULE_LICENSE("GPL");

// Lock
static DEFINE_SPINLOCK(hw2_lock);

// Data structure for a trace
struct hw2_trace_struct{
    unsigned long uptime;
    char *comm;
    pid_t pid;
    unsigned long start_time;
    pgd_t *pgd;

    // Code area
    unsigned long code_start_virt_by_mm;
    unsigned long code_start_virt_by_vm;
    pgd_t *code_start_pgd;
    unsigned long code_start_pgd_index;
    p4d_t *code_start_p4d;
    unsigned long code_start_p4d_base;
    unsigned long code_start_p4d_index;
    pud_t *code_start_pud;
    unsigned long code_start_pud_base;
    unsigned long code_start_pud_index;
    pmd_t *code_start_pmd;
    unsigned long code_start_pmd_base;
    unsigned long code_start_pmd_index;
    pte_t *code_start_pte;
    unsigned long code_start_pte_base;
    unsigned long code_start_pte_index;
    unsigned long code_start_pgd_val;
    unsigned long code_start_p4d_val;
    unsigned long code_start_pud_val;
    unsigned long code_start_pmd_val;
    unsigned long code_start_pte_val;
    unsigned long code_start_pfn;
    unsigned long code_start_phys;
    
    unsigned long code_end_virt_by_mm;
    unsigned long code_end_virt_by_vm;
    pgd_t *code_end_pgd;
    unsigned long code_end_pgd_index;
    p4d_t *code_end_p4d;
    unsigned long code_end_p4d_base;
    unsigned long code_end_p4d_index;
    pud_t *code_end_pud;
    unsigned long code_end_pud_base;
    unsigned long code_end_pud_index;
    pmd_t *code_end_pmd;
    unsigned long code_end_pmd_base;
    unsigned long code_end_pmd_index;
    pte_t *code_end_pte;
    unsigned long code_end_pte_base;
    unsigned long code_end_pte_index;
    unsigned long code_end_pgd_val;
    unsigned long code_end_p4d_val;
    unsigned long code_end_pud_val;
    unsigned long code_end_pmd_val;
    unsigned long code_end_pte_val;
    unsigned long code_end_pfn;
    unsigned long code_end_phys;

    // unsigned long code_end_debugging;

    // Data area
    unsigned long data_start_virt_by_mm;
    unsigned long data_start_virt_by_vm;
    unsigned long data_start_phys;
    unsigned long data_end_virt_by_mm;
    unsigned long data_end_virt_by_vm;
    unsigned long data_end_phys;

    // Heap area
    unsigned long heap_start_virt_by_mm;
    unsigned long heap_start_virt_by_vm;
    unsigned long heap_start_phys;
    unsigned long heap_end_virt_by_mm;
    unsigned long heap_end_virt_by_vm;
    unsigned long heap_end_phys;

    // Stack area
    unsigned long stack_start_virt_by_mm;
    unsigned long stack_start_virt_by_vm;
    unsigned long stack_start_phys;
    unsigned long stack_end_virt_by_mm;
    unsigned long stack_end_virt_by_vm;
    unsigned long stack_end_phys;
};
// Data structure for a tasklet
struct hw2_tasklet_data_struct{
    struct tasklet_struct tasklet;
    struct hw2_trace_struct hw2_trace[5];
};

// Declaring tasklet and timer
static struct timer_list hw2_timer;
static struct hw2_tasklet_data_struct hw2_tasklet_data;

static unsigned int hw2_trace_len = 0;

// Convert a virtual address to physical address
static unsigned long convert_to_phys(struct mm_struct *mm, unsigned long va){
    // Walk through pgd, p4d, pud, pmd, page table
    
    pgd_t *pgd; 
    p4d_t *p4d; 
    pud_t *pud; 
    pmd_t *pmd; 
    pte_t *pte; 
    spinlock_t *ptl;

    pgd = pgd_offset(mm, va);
    if(pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
        return -1;
    p4d = p4d_offset(pgd, va);
    if(p4d_none(*p4d) || unlikely(p4d_bad(*p4d)))
        return -1;
    pud = pud_offset(p4d, va);
    if(pud_none(*pud) || unlikely(pud_bad(*pud)))
        return -1;
    pmd = pmd_offset(pud, va);
    if(pmd_none(*pmd) || unlikely(pmd_bad(*pmd)))
        return -1;
    pte = pte_offset_map_lock(mm, pmd, va, &ptl);

    // Get the page frame number corresponding to the page table entry
    resource_size_t pfn = pte_pfn(*pte);

    // Make physical address by using pfn and virtual address
    unsigned long phys_address = (pfn << PAGE_SHIFT) | (va & ~PAGE_MASK);

    // Release all temporal cache for page table walks
    pte_unmap_unlock(pte, ptl);
    return phys_address;
}

// Extract the information from task
static void extract_info_from_task(struct hw2_trace_struct *trace, struct task_struct *task){
    // Get the system uptime
    trace->uptime = (unsigned long)ktime_to_ms(ktime_get_boottime()) / 1000; // 1000 milliseconds = 1 seconds
    // Extract the task information
    trace->pid = task->pid;
    trace->start_time = task->start_time / 1000000000;
    trace->comm = task->comm;

    // Create the vma iterator to traverse all memory regions 
    struct mm_struct *mm = task->mm;
    // Read lock
    mmap_read_lock(mm);
    VMA_ITERATOR(vmi, mm, 0);

    struct vm_area_struct *vma;
    unsigned long pfn;
    
    // Reference point for checking each memory region
    trace->pgd = mm->pgd;
    trace->code_start_virt_by_mm = mm->start_code;
    trace->code_end_virt_by_mm = mm->end_code;
    trace->data_start_virt_by_mm = mm->start_data; 
    trace->data_end_virt_by_mm = mm->end_data; 
    trace->heap_start_virt_by_mm = mm->start_brk; 
    trace->heap_end_virt_by_mm = mm->brk; 
    trace->stack_start_virt_by_mm = mm->start_stack; 
    trace->stack_end_virt_by_mm = trace->stack_start_virt_by_mm + mm->stack_vm; 

    // Traverse all memory areas to get information
    for_each_vma(vmi, vma){
        if(trace->code_start_virt_by_mm <= vma->vm_start && vma->vm_start < trace->code_end_virt_by_mm){
            spinlock_t *ptl;
            // VMA of code area 
            // In the case of code area, I need to get all the intermediate results of walking page tables
            trace->code_start_virt_by_vm = vma->vm_start; 
            trace->code_end_virt_by_vm = vma->vm_end; 
            // Debugging
            // trace->code_start_pgd_index = pgd_index(trace->code_start_virt_by_vm);
            trace->code_start_pgd = pgd_offset(mm, trace->code_start_virt_by_vm);
            trace->code_start_pgd_val = (*trace->code_start_pgd).pgd;
            // Debugging
            // trace->code_start_p4d_base = pgd_page_vaddr((*trace->code_start_pgd));
            // trace->code_start_p4d_index = p4d_index(trace->code_start_virt_by_vm) << 3;
            trace->code_start_p4d = p4d_offset(trace->code_start_pgd, trace->code_start_virt_by_vm);
            trace->code_start_p4d_val = (*trace->code_start_p4d).p4d;
            // Debugging
            // trace->code_start_pud_base = p4d_pgtable((*trace->code_start_p4d));
            // trace->code_start_pud_index = pud_index(trace->code_start_virt_by_vm) << 3;
            trace->code_start_pud = pud_offset(trace->code_start_p4d, trace->code_start_virt_by_vm);
            trace->code_start_pud_val = (*trace->code_start_pud).pud;
            // Debugging
            // trace->code_start_pmd_base = pud_pgtable((*trace->code_start_pud));
            // trace->code_start_pmd_index = pmd_index(trace->code_start_virt_by_vm) << 3;
            trace->code_start_pmd = pmd_offset(trace->code_start_pud, trace->code_start_virt_by_vm);
            trace->code_start_pmd_val = (*trace->code_start_pmd).pmd;
            // Debugging
            // trace->code_start_pte_base = pmd_page_vaddr((*trace->code_start_pmd));
            // trace->code_start_pte_index = pte_index(trace->code_start_virt_by_vm) << 3;
            trace->code_start_pte = pte_offset_map_lock(mm, trace->code_start_pmd, trace->code_start_virt_by_vm, &ptl);
            trace->code_start_pte_val = (*trace->code_start_pte).pte;

            pfn = pte_pfn(*trace->code_start_pte);
            trace->code_start_pfn = pfn;
            pte_unmap_unlock(trace->code_start_pte, ptl);
            trace->code_start_phys = (pfn << PAGE_SHIFT) | (trace->code_start_virt_by_vm & ~PAGE_MASK);

            // Debugging
            // trace->code_end_pgd_index = pgd_index(trace->code_end_virt_by_vm);
            trace->code_end_pgd = pgd_offset(mm, trace->code_end_virt_by_vm);
            trace->code_end_pgd_val = (*trace->code_end_pgd).pgd;
            // Debuggingend
            // trace->code_end_p4d_base = pgd_page_vaddr((*trace->code_end_pgd));
            // trace->code_end_p4d_index = p4d_index(trace->code_end_virt_by_vm) << 3;
            trace->code_end_p4d = p4d_offset(trace->code_end_pgd, trace->code_end_virt_by_vm);
            trace->code_end_p4d_val = (*trace->code_end_p4d).p4d;
            // Debuggingend
            // trace->code_end_pud_base = p4d_pgtable((*trace->code_end_p4d));
            // trace->code_end_pud_index = pud_index(trace->code_end_virt_by_vm) << 3;
            trace->code_end_pud = pud_offset(trace->code_end_p4d, trace->code_end_virt_by_vm);
            trace->code_end_pud_val = (*trace->code_end_pud).pud;
            // Debuggingend
            // trace->code_end_pmd_base = pud_pgtable((*trace->code_end_pud));
            // trace->code_end_pmd_index = pmd_index(trace->code_end_virt_by_vm) << 3;
            trace->code_end_pmd = pmd_offset(trace->code_end_pud, trace->code_end_virt_by_vm);
            trace->code_end_pmd_val = (*trace->code_end_pmd).pmd;
            // Debuggingend
            // trace->code_end_pte_base = pmd_page_vaddr((*trace->code_end_pmd));
            // trace->code_end_pte_index = pte_index(trace->code_end_virt_by_vm) << 3;
            trace->code_end_pte = pte_offset_map_lock(mm, trace->code_end_pmd, trace->code_end_virt_by_vm, &ptl);
            trace->code_end_pte_val = (*trace->code_end_pte).pte;

            pfn = pte_pfn(*trace->code_end_pte);
            trace->code_end_pfn = pfn;
            pte_unmap_unlock(trace->code_end_pte, ptl);
            trace->code_end_phys = (pfn << PAGE_SHIFT) | (trace->code_end_virt_by_vm & ~PAGE_MASK);
        }else if(trace->data_start_virt_by_mm <= vma->vm_start && vma->vm_start < trace->data_end_virt_by_mm){
            // VMA for data area
            trace->data_start_virt_by_vm = vma->vm_start; 
            trace->data_start_phys = convert_to_phys(mm, trace->data_start_virt_by_vm);
            trace->data_end_virt_by_vm = vma->vm_end; 
            trace->data_end_phys = convert_to_phys(mm, trace->data_end_virt_by_vm); 
        }else if(trace->heap_start_virt_by_mm <= vma->vm_start && vma->vm_start < trace->heap_end_virt_by_mm){
            // VMA for heap area
            trace->heap_start_virt_by_vm = vma->vm_start; 
            trace->heap_start_phys = convert_to_phys(mm, trace->heap_start_virt_by_vm); 
            trace->heap_end_virt_by_vm = vma->vm_end; 
            trace->heap_end_phys = convert_to_phys(mm, trace->heap_end_virt_by_vm);
        }else if(vma->vm_start <= trace->stack_start_virt_by_mm && trace->stack_end_virt_by_mm <= vma->vm_end){
            // VMA for stack area
            trace->stack_start_virt_by_vm = vma->vm_start; 
            trace->stack_start_phys = convert_to_phys(mm, trace->stack_start_virt_by_vm);
            trace->stack_end_virt_by_vm = vma->vm_end; 
            trace->stack_end_phys = convert_to_phys(mm, trace->stack_end_virt_by_vm);
        } 
    }
    // Unlock 
    mmap_read_unlock(mm);

}

// Tasklet function
static void hw2_tasklet_function(unsigned long data){
    // Declare and initiate the tasklet data
    struct hw2_tasklet_data_struct *tasklet_data = (struct hw2_tasklet_data_struct *)data;
    unsigned long hw2_flags;
    spin_lock_irqsave(&hw2_lock, hw2_flags);

    if (hw2_trace_len == 5){
        // Make a room for inserting the last element
        for(int i = 1; i < 5; i++){
            hw2_tasklet_data.hw2_trace[i-1].uptime = hw2_tasklet_data.hw2_trace[i].uptime;
            hw2_tasklet_data.hw2_trace[i-1].start_time = hw2_tasklet_data.hw2_trace[i].start_time;
            hw2_tasklet_data.hw2_trace[i-1].comm = hw2_tasklet_data.hw2_trace[i].comm ;
            hw2_tasklet_data.hw2_trace[i-1].pid = hw2_tasklet_data.hw2_trace[i].pid ;
            hw2_tasklet_data.hw2_trace[i-1].pgd = hw2_tasklet_data.hw2_trace[i].pgd ;

            hw2_tasklet_data.hw2_trace[i-1].code_start_virt_by_mm = hw2_tasklet_data.hw2_trace[i].code_start_virt_by_mm ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_virt_by_mm = hw2_tasklet_data.hw2_trace[i].code_end_virt_by_mm ;
            hw2_tasklet_data.hw2_trace[i-1].data_start_virt_by_mm = hw2_tasklet_data.hw2_trace[i].data_start_virt_by_mm ;
            hw2_tasklet_data.hw2_trace[i-1].data_end_virt_by_mm= hw2_tasklet_data.hw2_trace[i].data_end_virt_by_mm;
            hw2_tasklet_data.hw2_trace[i-1].heap_start_virt_by_mm = hw2_tasklet_data.hw2_trace[i].heap_start_virt_by_mm ;
            hw2_tasklet_data.hw2_trace[i-1].heap_end_virt_by_mm = hw2_tasklet_data.hw2_trace[i].heap_end_virt_by_mm ;
            hw2_tasklet_data.hw2_trace[i-1].stack_start_virt_by_mm = hw2_tasklet_data.hw2_trace[i].stack_start_virt_by_mm ;
            hw2_tasklet_data.hw2_trace[i-1].stack_end_virt_by_mm= hw2_tasklet_data.hw2_trace[i].stack_end_virt_by_mm;

            hw2_tasklet_data.hw2_trace[i-1].code_start_virt_by_vm = hw2_tasklet_data.hw2_trace[i].code_start_virt_by_vm ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_virt_by_vm = hw2_tasklet_data.hw2_trace[i].code_end_virt_by_vm ;
            hw2_tasklet_data.hw2_trace[i-1].data_start_virt_by_vm = hw2_tasklet_data.hw2_trace[i].data_start_virt_by_vm ;
            hw2_tasklet_data.hw2_trace[i-1].data_end_virt_by_vm= hw2_tasklet_data.hw2_trace[i].data_end_virt_by_vm;
            hw2_tasklet_data.hw2_trace[i-1].heap_start_virt_by_vm = hw2_tasklet_data.hw2_trace[i].heap_start_virt_by_vm ;
            hw2_tasklet_data.hw2_trace[i-1].heap_end_virt_by_vm = hw2_tasklet_data.hw2_trace[i].heap_end_virt_by_vm ;
            hw2_tasklet_data.hw2_trace[i-1].stack_start_virt_by_vm = hw2_tasklet_data.hw2_trace[i].stack_start_virt_by_vm ;
            hw2_tasklet_data.hw2_trace[i-1].stack_end_virt_by_vm= hw2_tasklet_data.hw2_trace[i].stack_end_virt_by_vm;

            hw2_tasklet_data.hw2_trace[i-1].code_start_pgd = hw2_tasklet_data.hw2_trace[i].code_start_pgd ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pud = hw2_tasklet_data.hw2_trace[i].code_start_pud ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pmd = hw2_tasklet_data.hw2_trace[i].code_start_pmd ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pte = hw2_tasklet_data.hw2_trace[i].code_start_pte ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pgd_val = hw2_tasklet_data.hw2_trace[i].code_start_pgd_val ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pud_val = hw2_tasklet_data.hw2_trace[i].code_start_pud_val ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pmd_val = hw2_tasklet_data.hw2_trace[i].code_start_pmd_val ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pte_val = hw2_tasklet_data.hw2_trace[i].code_start_pte_val ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pfn = hw2_tasklet_data.hw2_trace[i].code_start_pfn ;

            hw2_tasklet_data.hw2_trace[i-1].code_end_pgd = hw2_tasklet_data.hw2_trace[i].code_end_pgd ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pud = hw2_tasklet_data.hw2_trace[i].code_end_pud ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pmd = hw2_tasklet_data.hw2_trace[i].code_end_pmd ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pte = hw2_tasklet_data.hw2_trace[i].code_end_pte ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pgd_val = hw2_tasklet_data.hw2_trace[i].code_end_pgd_val ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pud_val = hw2_tasklet_data.hw2_trace[i].code_end_pud_val ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pmd_val = hw2_tasklet_data.hw2_trace[i].code_end_pmd_val ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pte_val = hw2_tasklet_data.hw2_trace[i].code_end_pte_val ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pfn = hw2_tasklet_data.hw2_trace[i].code_end_pfn ;

            hw2_tasklet_data.hw2_trace[i-1].code_start_phys = hw2_tasklet_data.hw2_trace[i].code_start_phys ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_phys = hw2_tasklet_data.hw2_trace[i].code_end_phys ;
            hw2_tasklet_data.hw2_trace[i-1].data_start_phys = hw2_tasklet_data.hw2_trace[i].data_start_phys ;
            hw2_tasklet_data.hw2_trace[i-1].data_end_phys= hw2_tasklet_data.hw2_trace[i].data_end_phys;
            hw2_tasklet_data.hw2_trace[i-1].heap_start_phys = hw2_tasklet_data.hw2_trace[i].heap_start_phys ;
            hw2_tasklet_data.hw2_trace[i-1].heap_end_phys = hw2_tasklet_data.hw2_trace[i].heap_end_phys ;
            hw2_tasklet_data.hw2_trace[i-1].stack_start_phys = hw2_tasklet_data.hw2_trace[i].stack_start_phys ;
            // Debugging
            hw2_tasklet_data.hw2_trace[i-1].code_start_pgd_index = hw2_tasklet_data.hw2_trace[i].code_start_pgd_index ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_p4d_index = hw2_tasklet_data.hw2_trace[i].code_start_p4d_index ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pud_index = hw2_tasklet_data.hw2_trace[i].code_start_pud_index ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pmd_index = hw2_tasklet_data.hw2_trace[i].code_start_pmd_index ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pte_index = hw2_tasklet_data.hw2_trace[i].code_start_pte_index ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_p4d_base = hw2_tasklet_data.hw2_trace[i].code_start_p4d_base ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pud_base = hw2_tasklet_data.hw2_trace[i].code_start_pud_base ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pmd_base = hw2_tasklet_data.hw2_trace[i].code_start_pmd_base ;
            hw2_tasklet_data.hw2_trace[i-1].code_start_pte_base = hw2_tasklet_data.hw2_trace[i].code_start_pte_base ;
            // Debugging
            hw2_tasklet_data.hw2_trace[i-1].code_end_pgd_index = hw2_tasklet_data.hw2_trace[i].code_end_pgd_index ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_p4d_index = hw2_tasklet_data.hw2_trace[i].code_end_p4d_index ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pud_index = hw2_tasklet_data.hw2_trace[i].code_end_pud_index ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pmd_index = hw2_tasklet_data.hw2_trace[i].code_end_pmd_index ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pte_index = hw2_tasklet_data.hw2_trace[i].code_end_pte_index ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_p4d_base = hw2_tasklet_data.hw2_trace[i].code_end_p4d_base ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pud_base = hw2_tasklet_data.hw2_trace[i].code_end_pud_base ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pmd_base = hw2_tasklet_data.hw2_trace[i].code_end_pmd_base ;
            hw2_tasklet_data.hw2_trace[i-1].code_end_pte_base = hw2_tasklet_data.hw2_trace[i].code_end_pte_base ;

            // hw2_tasklet_data.hw2_trace[i-1].code_end_debugging = hw2_tasklet_data.hw2_trace[i].code_end_debugging ;
        }
    }
    
    // Find the task which have the largest start_time field in current existing tasks
    struct task_struct *task;
    unsigned long max_start_time = 0;

    for_each_process(task){
        if(task->mm){
            if(task->start_time > max_start_time){
                if(hw2_trace_len == 5)
                    extract_info_from_task(&tasklet_data->hw2_trace[hw2_trace_len - 1], task);
                else
                    extract_info_from_task(&tasklet_data->hw2_trace[hw2_trace_len], task);
                max_start_time = task->start_time;
            }    

        }
    }
    // if(hw2_trace_len == 5){
    //     extract_info_from_task(&tasklet_data->hw2_trace[hw2_trace_len - 1], max_start_time_task);
    // }else{
    //     // Extract the info of the task and insert it into the ith trace
    //     extract_info_from_task(&tasklet_data->hw2_trace[hw2_trace_len], max_start_time_task);
    //     hw2_trace_len += 1;
    // }
    if(hw2_trace_len < 5)
        hw2_trace_len += 1;
    spin_unlock_irqrestore(&hw2_lock, hw2_flags);
    // Reschedule the timer for the next run
    mod_timer(&hw2_timer, get_jiffies_64() + msecs_to_jiffies(10000)); // 10000 milliseconds = 10 seconds 
}

// Timer function to schedule the tasklet
static void hw2_timer_function(struct timer_list *unused){
    tasklet_schedule(&hw2_tasklet_data.tasklet);
}
static void *hw2_seq_start(struct seq_file *s, loff_t *pos)
{
    static unsigned long counter = 0;

    // Get the current uptime by using jiffies
    unsigned long uptime_seconds = (unsigned long)ktime_to_ms(ktime_get_boottime()) / 1000; // 1000 milliseconds = 1 seconds

    seq_printf(s, "[System Programming Assignment #2]\n");
    seq_printf(s, "ID: %d\n", 2018147593);
    seq_printf(s, "Name: Yang, Jaehoon\n");
    seq_printf(s, "Uptime (s): %ld\n", uptime_seconds);
    seq_printf(s, "--------------------------------------------------\n");

    if (*pos == 0)
        return &counter;
    else
    {
        *pos = 0;
        return NULL;
    }
}

static void *hw2_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    unsigned long *tmp_v = (unsigned long *)v;

    (*tmp_v)++;
    (*pos)++;

    return NULL;
}

static void hw2_seq_stop(struct seq_file *s, void *v) {
}

static int hw2_seq_show(struct seq_file *s, void *v)
{
    struct hw2_trace_struct hw2_trace;
    // Iterate all trace list
    unsigned long hw2_flags;
    spin_lock_irqsave(&hw2_lock, hw2_flags);
    for(int i = 0; i < hw2_trace_len; i++){
        hw2_trace = hw2_tasklet_data.hw2_trace[i];
        // Print the info
        seq_printf(s, "[Trace #%d]\n", i);
        seq_printf(s, "Uptime (s): %ld\n", hw2_trace.uptime);
        seq_printf(s, "Command: %s\n", hw2_trace.comm);
        seq_printf(s, "PID: %d\n", hw2_trace.pid);
        seq_printf(s, "Start time (s): %ld\n", hw2_trace.start_time);
        seq_printf(s, "PGD base address: 0x%lx\n", hw2_trace.pgd);
        seq_printf(s, "Code Area\n");
        seq_printf(s, "- start (virtual): 0x%lx\n", hw2_trace.code_start_virt_by_vm);
        seq_printf(s, "- start (PGD): 0x%lx, 0x%lx\n", hw2_trace.code_start_pgd, hw2_trace.code_start_pgd_val);
        seq_printf(s, "- start (PUD): 0x%lx, 0x%lx\n", hw2_trace.code_start_pud, hw2_trace.code_start_pud_val);
        seq_printf(s, "- start (PMD): 0x%lx, 0x%lx\n", hw2_trace.code_start_pmd, hw2_trace.code_start_pmd_val);
        seq_printf(s, "- start (PTE): 0x%lx, 0x%lx\n", hw2_trace.code_start_pte, hw2_trace.code_start_pte_val);
        seq_printf(s, "- start (physical): 0x%lx\n", hw2_trace.code_start_phys);

        seq_printf(s, "- end (virtual): 0x%lx\n", hw2_trace.code_end_virt_by_vm);
        seq_printf(s, "- end (PGD): 0x%lx, 0x%lx\n", hw2_trace.code_end_pgd, hw2_trace.code_end_pgd_val);
        seq_printf(s, "- end (PUD): 0x%lx, 0x%lx\n", hw2_trace.code_end_pud, hw2_trace.code_end_pud_val);
        seq_printf(s, "- end (PMD): 0x%lx, 0x%lx\n", hw2_trace.code_end_pmd, hw2_trace.code_end_pmd_val);
        seq_printf(s, "- end (PTE): 0x%lx, 0x%lx\n", hw2_trace.code_end_pte, hw2_trace.code_end_pte_val);
        seq_printf(s, "- end (physical): 0x%lx\n", hw2_trace.code_end_phys);
        seq_printf(s, "Data Area\n");
        seq_printf(s, "- start (virtual): 0x%lx\n", hw2_trace.data_start_virt_by_vm);
        seq_printf(s, "- start (physical): 0x%lx\n", hw2_trace.data_start_phys);
        seq_printf(s, "- end (virtual): 0x%lx\n", hw2_trace.data_end_virt_by_vm);
        seq_printf(s, "- end (physical): 0x%lx\n", hw2_trace.data_end_phys);
        seq_printf(s, "Heap Area\n");
        seq_printf(s, "- start (virtual): 0x%lx\n", hw2_trace.heap_start_virt_by_vm);
        seq_printf(s, "- start (physical): 0x%lx\n", hw2_trace.heap_start_phys);
        seq_printf(s, "- end (virtual): 0x%lx\n", hw2_trace.heap_end_virt_by_vm);
        seq_printf(s, "- end (physical): 0x%lx\n", hw2_trace.heap_end_phys);
        seq_printf(s, "Stack Area\n");
        seq_printf(s, "- start (virtual): 0x%lx\n", hw2_trace.stack_start_virt_by_vm);
        seq_printf(s, "- start (physical): 0x%lx\n", hw2_trace.stack_start_phys);
        seq_printf(s, "- end (virtual): 0x%lx\n", hw2_trace.stack_end_virt_by_vm);
        seq_printf(s, "- end (physical): 0x%lx\n", hw2_trace.stack_end_phys);
        seq_printf(s, "--------------------------------------------------\n");
    }
    spin_unlock_irqrestore(&hw2_lock, hw2_flags);
    return 0;
}

static struct seq_operations hw2_seq_ops = {.start = hw2_seq_start,
    .next = hw2_seq_next,
    .stop = hw2_seq_stop,
    .show = hw2_seq_show};

static int hw2_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &hw2_seq_ops);
}

static const struct proc_ops hw2_proc_ops = {.proc_open = hw2_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release};

static int __init hw2_init(void)
{
    struct proc_dir_entry *proc_file_entry;
    proc_file_entry = proc_create(PROC_NAME, 0, NULL, &hw2_proc_ops);

    // Schedule a tasklet right after initializing the kernel module
    tasklet_init(&hw2_tasklet_data.tasklet, hw2_tasklet_function, (unsigned long)&hw2_tasklet_data);
    tasklet_schedule(&hw2_tasklet_data.tasklet);
    
    // Set up and add the timer to schedule the tasklet periodically  
    timer_setup(&hw2_timer, hw2_timer_function, 0);
    hw2_timer.expires = get_jiffies_64() +  msecs_to_jiffies(10000); // 10000 milliseconds = 10 seconds
    add_timer(&hw2_timer);

    return 0;
}

static void __exit hw2_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    // Remove the tasklet and timer
    tasklet_kill(&hw2_tasklet_data.tasklet);
    del_timer_sync(&hw2_timer);
}

module_init(hw2_init);
module_exit(hw2_exit);
