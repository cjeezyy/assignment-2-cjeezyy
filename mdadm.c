#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"

int mount_count = 0; //Nothing has been mounted so mount_count = 0//setting count

uint32_t encode_op(int cmd, int disk_num, int block_num){

  uint32_t op = 0;
  op |= cmd << 12;
  op |= disk_num << 0;
  op |= block_num << 4;

  return op;
 
}

int mdadm_mount(void) {
  if (mount_count == 1){return -1;}         //if mount count = 1 the disks have been mounted already and mount will fail


  uint32_t op = encode_op(JBOD_MOUNT,0,0); //initiate mount disk command
  jbod_operation(op, NULL); //mount disk
  mount_count = 1;

  if (mount_count == 1){return 1;} //if a mount is attempted before mount_count = 1 it fails

  
return -1;   
}

int mdadm_unmount(void) {
  if (mount_count == 0){return -1;}         //if unmount count = 0 the disks have been already been unmounted and will return -1 for failure

  

  uint32_t op = encode_op(JBOD_UNMOUNT,0,0); //initiate unmount disk command
  jbod_operation(op, NULL); //unmount disk
  mount_count = 0;      //'global' variable changed to unmount
  if (mount_count == 0){return 1;} //if a disk is unmounted(mount_count =0), then unmount succeeds
  
  return -1;
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
   uint32_t disk_num, block_num, disk_offset, current_addr = start_addr;
   uint8_t myBuf[JBOD_BLOCK_SIZE];
   uint32_t len = read_len;

  // Check if the disk is mounted  
  if (mount_count == 0) {return -3;} //if disk is unmounted fail

  //0-length read should succeed with a NULL pointer
  if (len == 0 && read_buf == NULL) {return 1;}
   
  // Check for valid read length
  if (len == 0 || len > 1024) {return -2;}
  
  // Check for null buffer
  if (read_buf == NULL) {return -4;} //if buffer space is null the disk read should fail
  
  
  
  // Check if read exceeds maximum address space
  if (start_addr + len > 1048576) {return -1;} //check that you have valid address space before disk read
  
  // Calculate disk, block, and offset
  disk_num = (start_addr / JBOD_DISK_SIZE);
  disk_offset = (start_addr % JBOD_DISK_SIZE);
  block_num = (disk_offset / JBOD_BLOCK_SIZE);

  while (len > 0) {
    // Seek to the intended disk
    jbod_operation(encode_op(JBOD_SEEK_TO_DISK, disk_num, 0), NULL);

    // Seek to the intended block
    jbod_operation(encode_op(JBOD_SEEK_TO_BLOCK, 0, block_num), NULL);

    // Read the block into myBuf
    jbod_operation(encode_op(JBOD_READ_BLOCK, 0, 0), myBuf);

    // Calculate how many bytes we can read from this block
    uint32_t bytes_to_read = JBOD_BLOCK_SIZE - (current_addr % JBOD_BLOCK_SIZE);
    if (bytes_to_read > len) {bytes_to_read = len;}

    // Copy bytes from myBuf to read_buf
    memcpy(read_buf, myBuf + (current_addr % JBOD_BLOCK_SIZE), bytes_to_read);

    // Update variables
    current_addr += bytes_to_read;
    read_buf += bytes_to_read;
    len -= bytes_to_read;

    // Move to the next block or disk
    block_num++;
    if (block_num >= (JBOD_DISK_SIZE / JBOD_BLOCK_SIZE)) {
      block_num = 0;
      disk_num++;
    }
  }

  return read_len;  // Return the number of bytes read
  
}

