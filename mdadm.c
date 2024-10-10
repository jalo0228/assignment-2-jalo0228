#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"

static int check_mount = 0; //check mount

int jbod_operation(uint32_t op, uint8_t *block); 
//helper function to run jbod_operation
int operator(int DiskID, int BlockID, int Command){
  int op = DiskID | BlockID<<4 | Command<<12;
  return op;
}

int mdadm_mount(void) {
  // Complete your code here
  if(check_mount == 1){  //check if it is already mounted
    return -1;
  }
  //if not mounted mount
  int result = operator(0,0,JBOD_MOUNT);
  if (check_mount == 0){ 
    if (jbod_operation(result,NULL)==0){
          check_mount = 1;
    return 1;
    };
  }
  return -1;
}

int mdadm_unmount(void) {
  //Complete your code here
  //check already mounted
  if(check_mount == 0){ 
    return -1;
  }
  //if mounted unmount
  int result = operator(0,0,JBOD_UNMOUNT);
  if (check_mount == 1){ //
    if (jbod_operation(result,NULL)==0){
          check_mount = 0;
    return 1;
    };
  }
  return -1;
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf){
  //Complete your code here
  //check the total size
  int total_size = JBOD_NUM_DISKS * JBOD_NUM_BLOCKS_PER_DISK  * JBOD_BLOCK_SIZE;
  //check valide address space
  if(start_addr + read_len > total_size){
    return -1;
  } 
  //check if read_len exceed 1024 bytes
  if (read_len > 1024){
    return -2;
  }
  //check whether it is not mounted
  if (check_mount == 0){
    return -3;
  }
  //additional restriction;
  if (read_buf == NULL && read_len > 0 ) {
    return -4;
  }
  //while currnet_ad > block_size, //seek_to_block -> seek_to_block -> find the offset -> read_block
  //after reading a block read_len - block_size
  uint32_t current_addr = start_addr;
  uint32_t end_address = start_addr + read_len;
  uint8_t temp[JBOD_BLOCK_SIZE];
  while (current_addr < end_address){
    uint32_t DiskID = current_addr / JBOD_DISK_SIZE;
    uint32_t BlockID = (current_addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;

    //seek to disk
    uint32_t checkdisk= operator(DiskID,0,JBOD_SEEK_TO_DISK); //seek to disk
    if(jbod_operation(checkdisk,NULL) == -1){
      return -4;
    }

    //seek to block
    uint32_t checkblock= operator(0,BlockID,JBOD_SEEK_TO_BLOCK);
    if(jbod_operation(checkblock,NULL) == -1){
      return -4;
    }

    //read block
    
    uint32_t readblock = operator(0,0,JBOD_READ_BLOCK);
    if(jbod_operation(readblock,temp)== -1){
      return -4;  
    }

    //when offset exists. offset : when current_addr is not the beginning of the block
    int offset = (current_addr) % JBOD_BLOCK_SIZE;
    int remaining_len = end_address - current_addr;
    int bytes_to_read;
    //calculte bytes to read from block
    if(remaining_len < (JBOD_BLOCK_SIZE - offset)){
      bytes_to_read = remaining_len;
    }
    else{
      bytes_to_read = JBOD_BLOCK_SIZE - offset;
    }

    //copy the calculted bytes into read_buf
    memcpy(read_buf + (current_addr - start_addr), temp+offset , bytes_to_read);

    current_addr += bytes_to_read;
  }
  return read_len;
}
