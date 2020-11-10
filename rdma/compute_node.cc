#include <iostream>
#include "rdma.h"
#include "../test/test_suite.h"

/*
 * GetEmptyBTree() - Returns an empty Btree multimap object created on the heap
 */ 
BTreeType *GetEmptyBTree() {
  BTreeType *t = new BTreeType{KeyComparator{1}};
  
  return t; 
}

void client_thread(RDMA_Manager* rdma_manager){

  BTreeType *root = GetEmptyBTree();
  int key_num = 30 * 1024 * 1024;
  printf("Using key size = %d (%f million)\n",
           key_num,
           key_num / (1024.0 * 1024.0));

  auto myid = std::this_thread::get_id();
  std::stringstream ss;
  ss << myid;
  std::string thread_id = ss.str();

  long int start_key = key_num / 40 * (long)1;

  root->insert(start_key, start_key);

  rdma_manager->Remote_Memory_Register(100*1024*1024);
  rdma_manager->Remote_Query_Pair_Connection(thread_id);
  std::cout << rdma_manager->remote_mem_pool[0];
  ibv_mr mem_pool_table[2];
  rdma_manager->local_mem_pool[0] = (ibv_mr *)root;
  mem_pool_table[0] = *(rdma_manager->local_mem_pool[0]);
  mem_pool_table[1] = *(rdma_manager->local_mem_pool[0]);
  mem_pool_table[1].addr = (void*)((char*)mem_pool_table[1].addr + sizeof("root"));// PROBLEM Could be here.

  // char *msg = static_cast<char *>(rdma_manager->local_mem_pool[0]->addr);
  // strcpy(msg, "message from computing node");
  int msg_size = sizeof(root);
  rdma_manager->RDMA_Write(rdma_manager->remote_mem_pool[0], &mem_pool_table[0],
                           msg_size, thread_id);

  rdma_manager->RDMA_Read(rdma_manager->remote_mem_pool[0], &mem_pool_table[1],
                          msg_size, thread_id);

  std::cout << "write buffer: " << (char*)mem_pool_table[0].addr << std::endl;

  std::cout << "read buffer: " << (char*)mem_pool_table[1].addr << std::endl;

}
int main()
{
  struct config_t config = {
      NULL,  /* dev_name */
      NULL,  /* server_name */
      19875, /* tcp_port */
      1,	 /* ib_port */ //physical
      -1, /* gid_idx */
      4*10*1024*1024 /*initial local buffer size*/
  };
  auto Remote_Bitmap = new std::unordered_map<ibv_mr*, In_Use_Array>;
  auto Local_Bitmap = new std::unordered_map<ibv_mr*, In_Use_Array>;
  RDMA_Manager* rdma_manager = new RDMA_Manager(config, Remote_Bitmap, Local_Bitmap);
//  RDMA_Manager rdma_manager(config, Remote_Bitmap, Local_Bitmap);
  rdma_manager->Client_Set_Up_Resources();
  std::thread thread_object(client_thread, rdma_manager);
  while(1);

  return 0;
}
