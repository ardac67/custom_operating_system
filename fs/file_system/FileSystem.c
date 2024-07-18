#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXFILES 250
typedef struct DirectoryEntry {
    char name[64];
    char parent[64];
    int firstBlock;
    int directory;
    uint32_t size;
    int permissions;
    time_t creationTime;
    time_t modificationTime;
    char pass[20];
    int exists;
    int haspw;
}DirectoryEntry;

typedef struct SuperBlock{
    int block_size;
    int number_of_blocks;
    int free_blocks;
    int fat_blocks;
    int directory_blocks;
}SuperBlock;

int main(int argc, char * argv[]){
    if(argc < 3){
        printf("Error\n");
        exit(-1);
    }

    int block_size = 1024 * atof(argv[1]);

    int SIZE = 4 * 1024 * 1024 * atof(argv[1]);

    int number_of_blocks = SIZE / block_size;

    int fat_blocks = ((number_of_blocks * sizeof(int)) + block_size - 1) / block_size;

    int directory_blocks = ((MAXFILES * sizeof(DirectoryEntry)) + block_size - 1) / block_size; 

    int free_blocks = ((number_of_blocks * sizeof(int)) + block_size - 1) / block_size;

    char * filename = argv[2];

    FILE * file = fopen(filename, "w");
    if(file == NULL){
        printf("Error\n");
        exit(-1);
    }
    SuperBlock superBlock;
    superBlock.block_size = block_size;
    superBlock.number_of_blocks = number_of_blocks;
    superBlock.fat_blocks = fat_blocks;
    superBlock.directory_blocks = directory_blocks;
    superBlock.free_blocks = free_blocks;
    DirectoryEntry directoryEntry;
    directoryEntry.size = 0;
    directoryEntry.firstBlock = 0;
    directoryEntry.directory = 1;
    directoryEntry.permissions = 2;
    directoryEntry.creationTime = time(NULL);
    directoryEntry.modificationTime = time(NULL);
    strcpy(directoryEntry.name, "/");
    strcpy(directoryEntry.parent, "/");

    fwrite(&superBlock, sizeof(SuperBlock), 1, file);

    int table_of_free_blocks[number_of_blocks];

    for(int i = 0; i < number_of_blocks; i++){
        if(i < 1 + free_blocks + fat_blocks + directory_blocks)
                table_of_free_blocks[i] = 0; 
        else
            table_of_free_blocks[i] = 1;
        fwrite(&table_of_free_blocks[i], sizeof(int), 1, file);
    }


    int fat_table[number_of_blocks];
    for(int i = 0; i < number_of_blocks; i++){
        fat_table[i] = -1;
        fwrite(&fat_table[i], sizeof(int), 1, file);
    }
    fwrite(&directoryEntry, sizeof(DirectoryEntry), 1, file);
    fseek(file, (directory_blocks - 1) * block_size, SEEK_SET);

    char buffer[block_size];
    for(int i = 0; i < number_of_blocks; i++){
        fwrite(buffer, block_size, 1, file);
    }
    fclose(file);
    
}
