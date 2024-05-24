// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];  //FPO LAB8

void
kinit() 
{
  //initlock(&kmem.lock, "kmem"); // FPO LAB8
  for (int i = 0; i < NCPU; i++) {
    char name[9] = {0};
    snprintf(name, 8, "kmem-%d", i);
    initlock(&kmem[i].lock, name);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  push_off();// FPO LAB8
  int cpu = cpuid();

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem[cpu].lock);// FPO LAB8
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  release(&kmem[cpu].lock);

  pop_off();// FPO LAB8
}

// Try steal a free physical memory page from another core
// interrupt should already be turned off
// return NULL if not found free page FPO LAB8
void *
ksteal(int cpu) {
  struct run *r;
  for (int i = 1; i < NCPU; i++) {
    // 从右边的第一个邻居开始偷
    int next_cpu = (cpu + i) % NCPU;
    // --- critical session ---
    acquire(&kmem[next_cpu].lock);
    r = kmem[next_cpu].freelist;
    if (r) {
      // steal one page
      kmem[next_cpu].freelist = r->next;
    }
    release(&kmem[next_cpu].lock);
    // --- end of critical session ---
    if (r) {
      break;
    }
  }
  // 有可能返回NULL, 如果邻居也都没有空余页的话
  return r;
}


// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();//FPO LAB8
  int cpu = cpuid();//FPO LAB8
  
  acquire(&kmem[cpu].lock);//FPO LAB8
  r = kmem[cpu].freelist;
  if(r)
    kmem[cpu].freelist = r->next;
  release(&kmem[cpu].lock);

  if(r == 0){//FPO LAB8
    r = ksteal(cpu);
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  
  pop_off();//FPO LAB8
  return (void*)r;
}

// void *  //取走一半
// kalloc(void)
// {
//   struct run *r;
//   push_off();
//   int id = cpuid();
//   acquire(&kmem[id].lock);
//   r = kmem[id].freelist;
//   if(r) {
//     kmem[id].freelist = r->next;
//     release(&kmem[id].lock);
//   } else {
//     release(&kmem[id].lock);
//     for (int i = 0; i < NCPU; ++i) {
//       if (i == id) {
//         continue;
//       }
//       if (i < id) {
//         acquire(&kmem[i].lock);
//         acquire(&kmem[id].lock);
//       } else {
//         acquire(&kmem[id].lock);
//         acquire(&kmem[i].lock);
//       }
//       struct run *slow = kmem[i].freelist; 
//       struct run *fast = slow;
//       while (fast && fast->next) {
//         fast = fast->next->next;
//         slow = slow->next;
//       }
//       if (slow) {
//         kmem[id].freelist = kmem[i].freelist;
//         kmem[i].freelist = slow->next;
//         slow->next = 0;
//         r = kmem[id].freelist;
//         kmem[id].freelist = r->next;
//         release(&kmem[id].lock);
//         release(&kmem[i].lock);
//         break;
//       } 
//       release(&kmem[id].lock);
//       release(&kmem[i].lock); 
//     }
//   }

//   if(r)
//     memset((char*)r, 5, PGSIZE); // fill with junk
//   pop_off();
//   return (void*)r;
// }