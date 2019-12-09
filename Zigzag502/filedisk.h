#ifndef FILEDISK_H_INCLUDED
#define FILEDISK_H_INCLUDED

#include "global.h"
typedef union
{
    char block_char[2048][16];
    UINT16 block_int[2048][16 / sizeof(short)];
}FILEDISK;

void IncreaseAllocateFNode(void);
int getFNode(void);
int diskFormat(int DiskID);
void diskWritten(int diskID, int sector, char* str);
void diskRead(int diskID, int sector, char* str);
int create_file(char* file_name);
int create_dir(char* dir_name);
int open_dir(int diskID, char* dir_name);
int open_file(char* file_name, int* inode);
int write_file(int inode, int logical_addr, char* writtenBuffer);
int read_file(int inode, int logical_addr, char* writtenBuffer);
int close_file(int inode);
#endif // FILEDISK_H_INCLUDED
