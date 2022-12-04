// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 17

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct buf *table[NBUCKET];
  struct spinlock bucket_locks[NBUCKET];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for(int i = 0; i < NBUCKET; i++){
    bcache.table[i] = 0;
    initlock(&bcache.bucket_locks[i], "bcache" + i);
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    int i = b->blockno % NBUCKET;
    b->next = bcache.table[i];
    bcache.table[i] = b;
  }
}

void
move_to_bucket(struct buf *prev, struct buf *b, int prev_hash, int hash) {
  acquire(&bcache.bucket_locks[hash]);
  if (prev != 0) {
    prev->next = b->next;
  } else {
    bcache.table[prev_hash] = b->next;
  }

  b->next = bcache.table[hash];
  bcache.table[hash] = b;
  release(&bcache.bucket_locks[hash]);    
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{ 
  
  struct buf *b;
  
  // Is the block already cached?
  int hash = blockno % NBUCKET;
	acquire(&bcache.bucket_locks[hash]);
  
  for(b = bcache.table[hash]; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_locks[hash]);
    
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.

	release(&bcache.bucket_locks[hash]);
  
  acquire(&bcache.lock);
  for(int i = 0; i < NBUCKET; i++){
    struct buf *prev = 0;
  
	  acquire(&bcache.bucket_locks[i]);
    
    for(b = bcache.table[i]; b != 0; b = b->next){
      if(b->refcnt == 0) {
        if (i != hash) {
          move_to_bucket(prev, b, i, hash);
        }
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        
	      release(&bcache.bucket_locks[i]);
        
        acquiresleep(&b->lock);
        release(&bcache.lock);
        return b;
      }
      prev = b;
    }
    
	release(&bcache.bucket_locks[i]);
   
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  int hash = b->blockno % NBUCKET;
  releasesleep(&b->lock);

	acquire(&bcache.bucket_locks[hash]);
  b->refcnt--;
	release(&bcache.bucket_locks[hash]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
	release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


