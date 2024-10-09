#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"

int mount_count = 0; //Nothing has been mounted so mount_count = 0//setting count

uint32_t encode_op(int cmd, int disk_num, int block_num){

  uint32_t op = 0;
  op |= cmd << 26;
  op |= disk_num << 22;
  op |= block_num<< 0;

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
  mount_count = 0;      //global variable changed to unmount
  if (mount_count == 0){return 1;} //if a disk is unmounted(mount_count =0), then unmount succeeds
  
  return -1;
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
  /* uint32_t disk_num;
  uint32_t block_num;
  uint32_t disk_offset;
  uint32_t current_Addr = start_addr;
  uint8_t myBuf[JBOD_BLOCK_SIZE] = ;
  uint32_t len = read_len;

   //check to see if disk is mounted

  if (mount_count == 0){return -1;}  //If the disk is not mounted read will fail
  if (len > 1024){return -1;}        //If the specified length is not 1024 the read will fail
  if ((buf == NULL) && (len == 0)){return len;}   //If buff is NULL and there is a length of 0 read will pass
  if ((addr+len) > 1048576){return -1;}   //If the addr and the length of the read exceed the max bits the statement will fail
  if (buf == NULL && addr >= 0 && len > 0) {return -1;}
  


  
 
  disk_num = (addr / JBOD_DISK_SIZE);
  disk_offset = (addr % JBOD_DISK_SIZE);
  block_num = (disk_offset / JBOD_BLOCK_SIZE);

  //Read from within the same block
  if ((addr + len) < JBOD_BLOCK_SIZE){
    
    //SEEK TO INTENDED DISK & BLOCK
     jbod_operation(encode_op(JBOD_SEEK_TO_DISK,disk_num,0), NULL); //seek to disk

     jbod_operation(encode_op(JBOD_SEEK_TO_BLOCK,0,block_num), NULL); //seek to block

     
    
     jbod_operation(encode_op(JBOD_READ_BLOCK,0,0), myBuf);  // reading into mybuf
     

     
     memcpy(buf, (myBuf + addr), len);  //Copy bytes from mybuf to buf

     return len;
  }

  //Reading Across blocks
  else if ((int)disk_num == (int)((addr + len )/ JBOD_DISK_SIZE)){

    int leftoverBytes = ((addr+len) - JBOD_BLOCK_SIZE);

    //SEEK TO INTENDED DISK & BLOCK
    jbod_operation(encode_op(JBOD_SEEK_TO_DISK,disk_num,0), NULL); //seek to disk

    jbod_operation(encode_op(JBOD_SEEK_TO_BLOCK,0,block_num), NULL); //seek to block


    jbod_operation(encode_op(JBOD_READ_BLOCK,0,0), myBuf);  // reading into mybuf

    memcpy(buf, myBuf + (addr % JBOD_DISK_SIZE), (len - leftoverBytes));  //Copy bytes from mybuf to buf

		 

    while (leftoverBytes > 0){

      //Updating lcal variables once after block is read
      current_Addr += len - leftoverBytes;
      disk_num = (current_Addr / JBOD_DISK_SIZE);
      block_num = (current_Addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;

       //SEEK TO INTENDED DISK & BLOCK
      jbod_operation(encode_op(JBOD_SEEK_TO_DISK,disk_num,0), NULL); //seek to disk

      jbod_operation(encode_op(JBOD_SEEK_TO_BLOCK,0,block_num), NULL); //seek to block

      jbod_operation(encode_op(JBOD_READ_BLOCK,0,0), myBuf);  // reading into mybuf

      if (leftoverBytes >= JBOD_BLOCK_SIZE){

	memcpy(buf + (current_Addr - addr), myBuf, JBOD_BLOCK_SIZE);
	current_Addr--;
      }
      
      else{
	memcpy(buf + (current_Addr - addr), myBuf, leftoverBytes);
      }

      leftoverBytes -= JBOD_BLOCK_SIZE;
    }

    return len;

    }


  //Reading across disks
  else {

    int leftoverBytes = (addr + len - JBOD_DISK_SIZE *  (disk_num + 1));

    //SEEK TO INTENDED DISK & BLOCK
    jbod_operation(encode_op(JBOD_SEEK_TO_DISK,disk_num,0), NULL); //seek to disk

    jbod_operation(encode_op(JBOD_SEEK_TO_BLOCK,0,block_num), NULL); //seek to block

     
    
    jbod_operation(encode_op(JBOD_READ_BLOCK,0,0), myBuf);  // reading into mybuf
     

     
    memcpy(buf, myBuf + (addr % (JBOD_DISK_SIZE - 1)), (len - leftoverBytes));  //Copy bytes from mybuf to buf

    while (leftoverBytes > 0){

      current_Addr += len - leftoverBytes;
      disk_num = (current_Addr / JBOD_DISK_SIZE);
      block_num = (current_Addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;

      //SEEK TO INTENDED DISK & BLOCK
      jbod_operation(encode_op(JBOD_SEEK_TO_DISK,disk_num,0), NULL); //seek to disk

      jbod_operation(encode_op(JBOD_SEEK_TO_BLOCK,0,block_num), NULL); //seek to block

      jbod_operation(encode_op(JBOD_READ_BLOCK,0,0), myBuf);  // reading into mybuf

      //copying bytes in buffer

      if (leftoverBytes >= JBOD_BLOCK_SIZE){

	memcpy(buf, myBuf + (current_Addr - addr), JBOD_BLOCK_SIZE);
	current_Addr --;

      }

      else {

	memcpy(buf + (current_Addr - addr), myBuf, leftoverBytes);
      }
      
      leftoverBytes -= JBOD_BLOCK_SIZE;

    }

    return len;
  }      
  */
  return -1;
}

