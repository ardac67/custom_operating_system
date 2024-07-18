#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#define MAXFILES 250
typedef struct DirectoryEntry {
    char name[64];
    char parent[64];
    int firstBlock;
    int directory;
    uint32_t size;
    int permissions; // 0 = read, 1 = write, 2 = read/write
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
int change_in_directory = 0;
int change_in_fat = 0;
int change_in_free_table = 0;
char * fs;

char **splitpath(const char delim[], const char *path);
void mkdirr(char **splitpath, DirectoryEntry directoryEntries[], int size_path);
void dir(char **splitpath, DirectoryEntry directoryEntries[], int size_path);
void dumpe2fs(SuperBlock superBlock, int free_table[], int fat_table[], DirectoryEntry directoryEntries[]);
int check_existence(const char *dir_name, const char *parent_name, DirectoryEntry directoryEntries[])   ;
int splitpath_size(const char delim[], const char *path);
void print_directory(DirectoryEntry directoryEntries[]);
void write(char **splitpath, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[],char *filename);
void read(char **splitpath, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[],char *filename);
void chmodd(char **splitpath, DirectoryEntry directoryEntries[], int size_path,char *file);
void addpw(char **splitpath, DirectoryEntry directoryEntries[], int size_path,char *pw);
int file_has_pw(char **splitpath, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[]);
void delete_file(char **splitpath, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[]);
void remove_dir(char **splitpath, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[]);
void delete_file_2(char *name, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[]);

int main(int argc, char * argv[]){
    if(argc < 3){
        printf("Not valid argument count\n");
        exit(-1);
    }

    fs = argv[1];

    char * operation = argv[2];

    char *paths = argv[3];

    FILE * file = fopen(fs, "rb");
    if(file == NULL){
        perror("fopen");
        exit(-1);
    }
 
    SuperBlock superBlock;

    fread(&superBlock, sizeof(SuperBlock), 1, file);

    int free_table[superBlock.number_of_blocks];

    fread(&free_table, sizeof(int), superBlock.number_of_blocks, file);
    
    int fat_table[superBlock.number_of_blocks];

    fread(&fat_table, sizeof(int), superBlock.number_of_blocks, file);

    fseek(file, sizeof(SuperBlock) + (superBlock.fat_blocks + superBlock.free_blocks) * superBlock.block_size, SEEK_SET);

    DirectoryEntry directoryEntries[MAXFILES];

    fread(directoryEntries, sizeof(DirectoryEntry), MAXFILES, file);
    
    fclose(file);
    char ** path;

    //printf("Path : %s\n", paths);
    if(argc > 3){
        path = splitpath("/", paths);
    }

    if(strcmp(operation, "mkdir") == 0){
        mkdirr(path,directoryEntries,splitpath_size("/", paths));
        print_directory(directoryEntries);
    }
    else if(strcmp(operation, "dir") == 0){
        dir(path, directoryEntries, splitpath_size("/",paths));
    }
    else if(strcmp(operation,"dumpe2fs") ==0){
        dumpe2fs(superBlock, free_table, fat_table, directoryEntries);
    }
    else if(strcmp(operation,"write") == 0){
        char* name = argv[4];
        char *last_name = path[splitpath_size("/", paths) - 1];
        write(path, directoryEntries, splitpath_size("/", paths), superBlock, free_table, fat_table, name);
    }
    else if(strcmp(operation,"read") == 0){
        char* name = argv[4];
        char *last_name = path[splitpath_size("/", paths) - 1];
        for(int i = 0; i < MAXFILES; i++){
            if(strcmp(directoryEntries[i].name, last_name) == 0){
                if(directoryEntries[i].haspw == 1 && (directoryEntries[i].permissions == 0 || directoryEntries[i].permissions == 2 )){
                    if(argv[5] == NULL){
                        printf("Password is NULL\n");
                        return 0;
                    }
                    else{
                        char *password = argv[5];
                        if(strcmp(password, directoryEntries[i].pass) != 0){
                            printf("Wrong password\n");
                            return 0;
                        }
                        else{
                            read(path, directoryEntries, splitpath_size("/", paths), superBlock, free_table, fat_table, name);
                        }
                    }
                }
                else if((directoryEntries[i].permissions == 0 || directoryEntries[i].permissions == 2 )){
                    read(path, directoryEntries, splitpath_size("/", paths), superBlock, free_table, fat_table, name);
                }
                else{
                    printf("You do not have permission to read this file\n");
                    return 0;
                }
            }
        }
    }
    else if(strcmp(operation,"chmod") == 0){
        char *last_name = path[splitpath_size("/", paths) - 1];
        for(int i = 0; i < MAXFILES; i++){
            if(strcmp(directoryEntries[i].name, last_name) == 0){
                if(directoryEntries[i].haspw == 1){
                    if(argv[4] == NULL){
                        printf("Password is NULL\n");
                        return 0;
                    }
                    else{
                        char *password = argv[4];
                        if(strcmp(password, directoryEntries[i].pass) != 0){
                            printf("Wrong password\n");
                            return 0;
                        }
                        else{
                            chmodd(path, directoryEntries, splitpath_size("/", paths), argv[5]);
                        }
                    }
                }
                else{
                    chmodd(path, directoryEntries, splitpath_size("/", paths), argv[4]);
                }
            }
        }
    }
    else if(strcmp(operation,"del") == 0){
        char *last_name = path[splitpath_size("/", paths) - 1];
        for(int i = 0; i < MAXFILES; i++){
            if(strcmp(directoryEntries[i].name, last_name) == 0){
                if(directoryEntries[i].haspw == 1){
                    if(argv[4] == NULL){
                        printf("Password is NULL\n");
                        return 0;
                    }
                    else{
                        char *password = argv[4];
                        if(strcmp(password, directoryEntries[i].pass) != 0){
                            printf("Wrong password\n");
                            return 0;
                        }
                        else{
                            delete_file(path, directoryEntries, splitpath_size("/", paths), superBlock, free_table, fat_table);
                        }
                    }
                }
                else{
                    delete_file(path, directoryEntries, splitpath_size("/", paths), superBlock, free_table, fat_table);
                }
            }
        }
    }
    else if(strcmp(operation,"rmdir") == 0){
        remove_dir(path, directoryEntries, splitpath_size("/", paths), superBlock, free_table, fat_table);
    }
    else if(strcmp(operation,"addpw") == 0){
        if(argv[4] == NULL){
            printf("Password is NULL\n");
            return 0;
        }
        addpw(path, directoryEntries, splitpath_size("/", paths), argv[4]);
    }
    else{
        printf("Invalid operation...\n");
    }

    file = fopen(fs, "rb+");
    if(file == NULL){
        perror("fopen");
        exit(-1);
    }
    if(change_in_directory == 1){
        rewind(file);

        fseek(file, sizeof(SuperBlock) + (superBlock.fat_blocks + superBlock.free_blocks) * superBlock.block_size, SEEK_SET);

        fwrite(directoryEntries, sizeof(DirectoryEntry), MAXFILES, file);
    }
    if(change_in_fat == 1){
        rewind(file);

        fseek(file, sizeof(SuperBlock) + (superBlock.block_size * superBlock.free_blocks), SEEK_SET);

        for(int i = 0; i < superBlock.number_of_blocks; i++){
            fwrite(&fat_table[i], sizeof(int), 1, file);
        }
    }
    if(change_in_free_table == 1){
        rewind(file);

        fseek(file, sizeof(SuperBlock), SEEK_SET);

        for(int i = 0; i < superBlock.number_of_blocks; i++){
            fwrite(&free_table[i], sizeof(int), 1, file);
        }
    }
    return 0;
}


char **splitpath(const char delim[], const char *path) {
    char *path_copy = strdup(path);
    char **returnVal = malloc(sizeof(char *) * 64);
    if (!returnVal) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if(strcmp(path,"/") == 0){
        returnVal[0] = strdup("/");
        returnVal[1] = NULL;
        return returnVal;
    }

    char *token = strtok(path_copy, delim);
    int counter = 0;

    while (token != NULL) {
        returnVal[counter] = strdup(token);
        if (!returnVal[counter]) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        counter++;
        token = strtok(NULL, delim);
    }
    returnVal[counter] = NULL;
    free(path_copy);
    return returnVal;
}

int splitpath_size(const char delim[], const char *path) {
    char *path_copy = strdup(path);
    if (!path_copy) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    if(strcmp(path,"/") == 0){
        return 1;
    }
    char *token = strtok(path_copy, delim);
    int counter = 0;

    while (token != NULL) {
        counter++;
        token = strtok(NULL, delim);
    }

    free(path_copy);
    //printf("Counter : %d\n", counter);
    return counter;
}


void mkdirr(char **splitpath, DirectoryEntry directoryEntries[], int size_path) {
    for (int i = 0; i < size_path - 1; i++) {
        const char *current_dir = splitpath[i];
        const char *parent_dir = (i == 0) ? "/" : splitpath[i - 1];

        printf("Checking existence of: %s in parent: %s\n", current_dir, parent_dir);
        if (!check_existence(current_dir, parent_dir, directoryEntries)) {
            printf("Directory does not exist: %s in parent: %s\n", current_dir, parent_dir);
            return;
        }
    }

    const char *last_dir = splitpath[size_path - 1];
    const char *parent_dir = (size_path == 1) ? "/" : splitpath[size_path - 2];

    if (!check_existence(last_dir, parent_dir, directoryEntries)) {
        printf("Creating directory: %s in parent: %s\n", last_dir, parent_dir);
        for (int i = 0; i < MAXFILES; i++) {
            if (directoryEntries[i].exists == 999) continue;

            strcpy(directoryEntries[i].name, last_dir);
            strcpy(directoryEntries[i].parent, parent_dir);
            directoryEntries[i].directory = 1;
            directoryEntries[i].size = 0;
            directoryEntries[i].exists = 999;
            time(&directoryEntries[i].creationTime);
            time(&directoryEntries[i].modificationTime);
            directoryEntries[i].permissions = 0;
            change_in_directory = 1;
            printf("Directory created: %s in parent: %s\n", last_dir, parent_dir);
            return;
        }
        printf("Failed to create directory: %s (no space left)\n", last_dir);
    } else {
        printf("Directory already exists: %s in parent: %s\n", last_dir, parent_dir);
    }
}






void print_directory(DirectoryEntry directoryEntries[]){
    for(int i = 0; i < MAXFILES; i++){
        if(directoryEntries[i].exists != 999) continue;
        printf("Name: %s Parent: %s Exists: %d\n", directoryEntries[i].name, directoryEntries[i].parent, directoryEntries[i].exists);
    }
}

void write(char **splitpath, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[],char *filename){

    for (int i = 0; i < size_path - 1; i++) {
        const char *current_dir = splitpath[i];
        const char *parent_dir = (i == 0) ? "/" : splitpath[i - 1];

        if (!check_existence(current_dir, parent_dir, directoryEntries)) {
            printf("Directory does not exist: %s in parent: %s\n", current_dir, parent_dir);
            return;
        }
    }
    const char *last_dir = splitpath[size_path - 1];
    const char *parent_dir = (size_path == 1) ? "/" : splitpath[size_path - 2];

    DirectoryEntry current_directory;
    for(int i =0 ;i<MAXFILES;i++){
        if(strcmp(directoryEntries[i].name,last_dir) == 0 ){
            current_directory = directoryEntries[i];
            break;
        }
    }
    if (!check_existence(last_dir, parent_dir, directoryEntries)) {
        FILE *fptr = fopen(filename, "r");
        if(fptr == NULL){
            printf("error\n");
            return;
        }
        struct stat fileStat;
        if (stat(filename, &fileStat) < 0) {
            perror("Failed to get file stats");
            return;
        }
        printf("Owner can read: %s\n", (fileStat.st_mode & S_IRUSR) ? "yes" : "no");
        printf("Owner can write: %s\n", (fileStat.st_mode & S_IWUSR) ? "yes" : "no");
        printf("Owner can execute: %s\n", (fileStat.st_mode & S_IXUSR) ? "yes" : "no");
        int permissionsRead,permissionWrite = -1;
        int totalPermissions = -1;
        if((fileStat.st_mode & S_IRUSR) != 0){
            permissionsRead = 1;
        }
        if((fileStat.st_mode & S_IWUSR) != 0){
            permissionWrite = 1;
        }
        if(permissionsRead == 1 && permissionWrite == 1){
            totalPermissions = 2;
        }
        else if(permissionsRead == 1 && permissionWrite == -1){
            totalPermissions = 0;
        }
        else if(permissionsRead == -1 && permissionWrite == 1){
            totalPermissions = 1;
        }
        if(permissionsRead == -1){
            printf("Owner can read: no\n");
            return;
        }
        
        fseek(fptr, 0, SEEK_END);
        int size = ftell(fptr);
        rewind(fptr);
        //problemli olabilir
        int file_block_allocation = (size / superBlock.block_size) +1;
        char **fileholder  = malloc(sizeof(char *) * file_block_allocation * sizeof(char *));
        for(int i = 0; i < file_block_allocation; i++){
            fileholder[i] = malloc(sizeof(char) * superBlock.block_size);
            fread(fileholder[i], sizeof(char),superBlock.block_size,fptr);
        }
        int current_block = 0;
        FILE *file = fopen(fs, "rb+");
        if(file == NULL){
            perror("fopen");
            exit(-1);
        }
        int start = -1;
        for(int i = 0; i < superBlock.number_of_blocks && current_block < file_block_allocation; i++){
            if(free_table[i] == 1){
                //printf("Writing to fat\n");
                fat_table[i] = start;
                free_table[i] = 0;
                start = i;
                fseek(file, sizeof(SuperBlock) + i * superBlock.block_size, SEEK_SET);
                fwrite(fileholder[file_block_allocation - current_block - 1], sizeof(char), superBlock.block_size, file);
                rewind(file);
                printf("Writing to block: %d\n", i);
                printf("Content: %s\n", fileholder[file_block_allocation - current_block - 1]);
                current_block++;
            }
        }
        fclose(fptr);
        if(current_block != file_block_allocation){
            printf("Not enough space\n");
            return;
        }
        for(int i = 0; i < MAXFILES; i++){
            if(directoryEntries[i].exists == 999) continue;
            strcpy(directoryEntries[i].name, last_dir);
            strcpy(directoryEntries[i].parent, parent_dir);
            directoryEntries[i].directory = 0;
            directoryEntries[i].size = size;
            directoryEntries[i].exists = 999;
            time(&directoryEntries[i].creationTime);
            time(&directoryEntries[i].modificationTime);
            directoryEntries[i].permissions = totalPermissions;
            directoryEntries[i].firstBlock = start;
            change_in_directory = 1;
            directoryEntries[i].haspw = 0;
            for(int i =0 ; i < MAXFILES ;i++){
                if(strcmp(directoryEntries[i].name, parent_dir) == 0){
                    directoryEntries[i].size += size;
                    break;
                }
            }
            change_in_free_table = 1;
            change_in_fat = 1;
            return;
        }
        
    } else {
        printf("File exists: %s in parent: %s\n", last_dir, parent_dir);
    }
}

void read(char **splitpath, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[],char *filename){
    for (int i = 0; i < size_path - 1; i++) {
        const char *current_dir = splitpath[i];
        const char *parent_dir = (i == 0) ? "/" : splitpath[i - 1];

        if (!check_existence(current_dir, parent_dir, directoryEntries)) {
            printf("Directory does not exist: %s in parent: %s\n", current_dir, parent_dir);
            return;
        }
    }
    const char *last_dir = splitpath[size_path - 1];
    const char *parent_dir = (size_path == 1) ? "/" : splitpath[size_path - 2];

    DirectoryEntry current_directory;
    for(int i =0 ;i<MAXFILES;i++){
        if(strcmp(directoryEntries[i].name,last_dir) == 0 ){
            current_directory = directoryEntries[i];
            break;
        }
    }
    printf("File: %s\n", last_dir); 
    if (check_existence(last_dir, parent_dir, directoryEntries)) {
        FILE *file = fopen(filename,"w");
        if(file == NULL){
            printf("error\n");
            return;
        }
        FILE *fptr = fopen(fs, "rb");
        if(fptr == NULL){
            perror("fopen");
            exit(-1);
        }
        printf("File: %s\n", current_directory.name);
        int start = current_directory.firstBlock;
        printf("Start: %d\n", start);
        while(start != -1){
            char buffer[superBlock.block_size];
            fseek(fptr, sizeof(SuperBlock) + start * superBlock.block_size, SEEK_SET);
            fread(buffer, sizeof(char), superBlock.block_size, fptr);
            printf("Buffer: %s\n", buffer);
            rewind(fptr);
            fwrite(buffer, sizeof(char), strlen(buffer), file);
            memset(buffer, 0, sizeof(buffer));
            
            start = fat_table[start];
        }
        fclose(fptr);
        fclose(file);
    }
    else{
        printf("File does not exist: %s in parent: %s\n", last_dir, parent_dir);
        return;
    }
     

}
//chmod(path, directoryEntries, splitpath_size("/", paths), argv[4]);
void chmodd(char **splitpath, DirectoryEntry directoryEntries[], int size_path,char *mod){
    for (int i = 0; i < size_path - 1; i++) {
        const char *current_dir = splitpath[i];
        const char *parent_dir = (i == 0) ? "/" : splitpath[i - 1];

        if (!check_existence(current_dir, parent_dir, directoryEntries)) {
            printf("Directory does not exist: %s in parent: %s\n", current_dir, parent_dir);
            return;
        }
    }
    const char *last_dir = splitpath[size_path - 1];
    const char *parent_dir = (size_path == 1) ? "/" : splitpath[size_path - 2];

    DirectoryEntry current_directory;
    for(int i =0 ;i<MAXFILES;i++){
        if(strcmp(directoryEntries[i].name,last_dir) == 0 ){
            current_directory = directoryEntries[i];
            break;
        }
    }

    if (check_existence(last_dir, parent_dir, directoryEntries)) {

        for(int i = 0; i < MAXFILES; i++) {
            if(strcmp(directoryEntries[i].name, current_directory.name) == 0) {

                int mod_new = 0;
                if(strcmp(mod,"r") == 0){
                    mod_new = 0;
                }
                else if(strcmp(mod,"w") == 0){
                    mod_new = 1;
                }
                else if(strcmp(mod,"+rw") == 0){
                    mod_new = 2;
                }
                else if(strcmp(mod,"-rw") == 0){
                    mod_new = -1;
                }
                else{
                    printf("Invalid permission\n");
                    return;
                }
                directoryEntries[i].permissions = mod_new;
                change_in_directory = 1;
                printf("Permissions changed to %i: %s in parent: %s\n",directoryEntries[i].permissions, last_dir, parent_dir);
                return;
            }
        }
    } else {
        printf("Directory does not exists: %s in parent: %s\n", last_dir, parent_dir);
    }
}

void addpw(char **splitpath, DirectoryEntry directoryEntries[], int size_path,char *pw){
    for (int i = 0; i < size_path - 1; i++) {
        const char *current_dir = splitpath[i];
        const char *parent_dir = (i == 0) ? "/" : splitpath[i - 1];

        if (!check_existence(current_dir, parent_dir, directoryEntries)) {
            printf("Directory does not exist: %s in parent: %s\n", current_dir, parent_dir);
            return;
        }
    }
    const char *last_dir = splitpath[size_path - 1];
    const char *parent_dir = (size_path == 1) ? "/" : splitpath[size_path - 2];

    DirectoryEntry current_directory;
    for(int i =0 ;i<MAXFILES;i++){
        if(strcmp(directoryEntries[i].name,last_dir) == 0 ){
            current_directory = directoryEntries[i];
            break;
        }
    }

    if (check_existence(last_dir, parent_dir, directoryEntries)) {

        for(int i = 0; i < MAXFILES; i++) {
            if(strcmp(directoryEntries[i].name, current_directory.name) == 0) {
                int mod_new = 0;
                if(pw == NULL){
                    printf("Password is NULL\n");
                    return;
                }
                else{
                    strcpy(directoryEntries[i].pass, pw);
                    directoryEntries[i].haspw = 1;
                    change_in_directory = 1;
                    printf("Pasword add to: %s in parent: %s\n", last_dir, parent_dir);
                    return;
                }
            }
        }
    } else {
        printf("Directory does not exists: %s in parent: %s\n", last_dir, parent_dir);
    }
}
void dir(char **splitpath, DirectoryEntry directoryEntries[], int size_path){
    if(strcmp(splitpath[0],"/") == 0 && splitpath[1]==NULL){
        printf("%-20s %-20s %-12s %-10s %-10s %-12s %-20s %-20s %-10s %-6s\n",
        "Name", "Parent", "First Block", "Directory", "Size", "Permissions", "Creation Time", "Modification Time", "Password", "Exists");

        for(int i = 0; i < MAXFILES; i++) {
            if(strcmp(directoryEntries[i].parent, "/") == 0) {

                printf("%-20s %-20s %-12d %-10d %-10u %-12d %-20ld %-20ld %-10s %-6d\n",
                    directoryEntries[i].name, 
                    directoryEntries[i].parent, 
                    directoryEntries[i].firstBlock, 
                    directoryEntries[i].directory, 
                    directoryEntries[i].size, 
                    directoryEntries[i].permissions, 
                    directoryEntries[i].creationTime, 
                    directoryEntries[i].modificationTime, 
                    directoryEntries[i].pass, 
                    directoryEntries[i].exists);
            }
        }
        return;
    } 
    for (int i = 0; i < size_path - 1; i++) {
        const char *current_dir = splitpath[i];
        const char *parent_dir = (i == 0) ? "/" : splitpath[i - 1];

        if (!check_existence(current_dir, parent_dir, directoryEntries)) {
            printf("Directory does not exist: %s in parent: %s\n", current_dir, parent_dir);
            return;
        }
    }
    const char *last_dir = splitpath[size_path - 1];
    const char *parent_dir = (size_path == 1) ? "/" : splitpath[size_path - 2];

    DirectoryEntry current_directory;
    for(int i =0 ;i<MAXFILES;i++){
        if(strcmp(directoryEntries[i].name,last_dir) == 0 ){
            current_directory = directoryEntries[i];
            break;
        }
    }
    if (check_existence(last_dir, parent_dir, directoryEntries)) {
        printf("%-20s %-20s %-12s %-10s %-10s %-12s %-20s %-20s %-10s %-6s\n",
        "Name", "Parent", "First Block", "Directory", "Size", "Permissions", "Creation Time", "Modification Time", "Password", "Exists");

        for(int i = 0; i < MAXFILES; i++) {
            if(strcmp(directoryEntries[i].parent, current_directory.name) == 0) {
                printf("%-20s %-20s %-12d %-10d %-10u %-12d %-20ld %-20ld %-10s %-6d\n",
                    directoryEntries[i].name, 
                    directoryEntries[i].parent, 
                    directoryEntries[i].firstBlock, 
                    directoryEntries[i].directory, 
                    directoryEntries[i].size, 
                    directoryEntries[i].permissions, 
                    directoryEntries[i].creationTime, 
                    directoryEntries[i].modificationTime, 
                    directoryEntries[i].pass, 
                    directoryEntries[i].exists);
            }
        }
    } else {
        printf("Directory does not exists: %s in parent: %s\n", last_dir, parent_dir);
    }

}

void dumpe2fs(SuperBlock superBlock, int free_table[], int fat_table[], DirectoryEntry directoryEntries[]){

    for(int i = 0; i < MAXFILES; i++){
        if(directoryEntries[i].exists == 999 && directoryEntries[i].directory == 0){
            printf("Blocks %s: ", directoryEntries[i].name);
            int start = directoryEntries[i].firstBlock;
            while(start != -1){
                printf("%d ", start);
                start = fat_table[start];
            }
            printf("\n");
        }
    }
    
    int free_blocks_counter = 0, file_counter = 0, directory_counter = 0;


    for(int i = 0; i < MAXFILES; i++){
        if(directoryEntries[i].exists == 999){
            if(directoryEntries[i].directory == 1){
                directory_counter++;
            }else{
                file_counter++;
            }
        }
    }
    printf("File count: %d\n", file_counter);
    printf("Directory Count: %d\n", directory_counter);

    for(int i = 0; i < superBlock.number_of_blocks; i++){
        free_blocks_counter += free_table[i];
    }
    printf("Free Block: %d\n", free_blocks_counter);

    printf("Block size: %d\nNumber of blocks: %d\n", superBlock.block_size, superBlock.number_of_blocks);

}

int check_existence(const char *dir_name, const char *parent_name, DirectoryEntry directoryEntries[]) {
    for (int i = 0; i < MAXFILES; i++) {
        if (directoryEntries[i].exists == 999 &&
            strcmp(directoryEntries[i].name, dir_name) == 0 &&
            strcmp(directoryEntries[i].parent, parent_name) == 0) {
            return 1;
        }
    }
    return 0;
}

int file_has_pw(char **splitpath, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[]){
    for (int i = 0; i < size_path - 1; i++) {
        const char *current_dir = splitpath[i];
        const char *parent_dir = (i == 0) ? "/" : splitpath[i - 1];

        if (!check_existence(current_dir, parent_dir, directoryEntries)) {
            printf("Directory does not exist: %s in parent: %s\n", current_dir, parent_dir);
            return 0;
        }
    }
    const char *last_dir = splitpath[size_path - 1];
    const char *parent_dir = (size_path == 1) ? "/" : splitpath[size_path - 2];

    DirectoryEntry current_directory;
    for(int i =0 ;i<MAXFILES;i++){
        if(strcmp(directoryEntries[i].name,last_dir) == 0 ){
            current_directory = directoryEntries[i];
            break;
        }
    }
    if (!check_existence(last_dir, parent_dir, directoryEntries)) {
        for(int i =0 ;i<MAXFILES;i++){
            if(strcmp(directoryEntries[i].name,last_dir) == 0 ){
                current_directory = directoryEntries[i];
                if(current_directory.haspw == 1){
                    return 1;
                }
                else{
                    return 0;
                }
            }
        }
    }
    return 0;
}

void delete_file(char **splitpath, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[]){
    for (int i = 0; i < size_path - 1; i++) {
        const char *current_dir = splitpath[i];
        const char *parent_dir = (i == 0) ? "/" : splitpath[i - 1];

        if (!check_existence(current_dir, parent_dir, directoryEntries)) {
            printf("Directory does not exist: %s in parent: %s\n", current_dir, parent_dir);
            return;
        }
    }
    const char *last_dir = splitpath[size_path - 1];
    const char *parent_dir = (size_path == 1) ? "/" : splitpath[size_path - 2];

    DirectoryEntry current_directory;
    for(int i =0 ;i<MAXFILES;i++){
        if(strcmp(directoryEntries[i].name,last_dir) == 0 ){
            current_directory = directoryEntries[i];
            break;
        }
    }
    // Check if the last directory already exists
    if (check_existence(last_dir, parent_dir, directoryEntries)) {

        for(int i = 0; i < MAXFILES; i++) {
            if(strcmp(directoryEntries[i].name, current_directory.name) == 0) {
                printf("Delete file: %s\n", last_dir);
                int current = directoryEntries[i].firstBlock; 
                int next;
                do{
                    next = fat_table[current]; 
                    free_table[current] = 1;
                    fat_table[current] = -1;
                    current = next; 

                }while(current != -1);
                strcpy(directoryEntries[i].name, "");
                strcpy(directoryEntries[i].parent, "");
                directoryEntries[i].modificationTime = 0;
                directoryEntries[i].directory = 0;
                directoryEntries[i].size = 0;
                directoryEntries[i].exists = 0;
                change_in_directory = 1;
                change_in_fat = 1;
                change_in_free_table = 1;
                return;
            }
        }
    } else {
        printf("Directory does not exists: %s in parent: %s\n", last_dir, parent_dir);
    }
}

void delete_file_2(char *name, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[]){
    
    for(int i = 0; i < MAXFILES; i++) {
        
        if(strcmp(directoryEntries[i].name, name) == 0) {
            printf("Delete file: %s\n", directoryEntries[i].name);
            int current = directoryEntries[i].firstBlock; 
            int next;
            do{
                next = fat_table[current]; 
                free_table[current] = 1;
                fat_table[current] = -1;
                current = next; 

            }while(current != -1);
            strcpy(directoryEntries[i].name, "");
            strcpy(directoryEntries[i].parent, "");
            directoryEntries[i].modificationTime = 0;
            directoryEntries[i].directory = 0;
            directoryEntries[i].size = 0;
            directoryEntries[i].exists = 0;
            change_in_directory = 1;
            change_in_fat = 1;
            change_in_free_table = 1;
            return;
        }
    }
}


void remove_dir(char **splitpath, DirectoryEntry directoryEntries[], int size_path, SuperBlock superBlock, int free_table[], int fat_table[]){
    for (int i = 0; i < size_path - 1; i++) {
        const char *current_dir = splitpath[i];
        const char *parent_dir = (i == 0) ? "/" : splitpath[i - 1];

        if (!check_existence(current_dir, parent_dir, directoryEntries)) {
            printf("Directory does not exist: %s in parent: %s\n", current_dir, parent_dir);
            return;
        }
    }
    const char *last_dir = splitpath[size_path - 1];
    const char *parent_dir = (size_path == 1) ? "/" : splitpath[size_path - 2];

    DirectoryEntry current_directory;
    for(int i =0 ;i<MAXFILES;i++){
        if(strcmp(directoryEntries[i].name,last_dir) == 0 ){
            current_directory = directoryEntries[i];
            break;
        }
    }
    if (check_existence(last_dir, parent_dir, directoryEntries)) {
        printf("Parent: %s\n", parent_dir);
        printf("Last: %s\n", last_dir);
        for(int i=0;i<MAXFILES;i++){
            if(strcmp(directoryEntries[i].parent, last_dir) == 0){
                if(directoryEntries[i].directory != 1){
                    printf("Files are: %s\n", directoryEntries[i].name);
                    //delete_file(path, directoryEntries, splitpath_size("/", paths), superBlock, free_table, fat_table);
                    delete_file_2(directoryEntries[i].name, directoryEntries, size_path, superBlock, free_table, fat_table);
                }
            }   
        }
        for(int i=0;i<MAXFILES;i++){
            if(strcmp(directoryEntries[i].parent, last_dir) == 0){
                if(directoryEntries[i].directory == 1){
                    remove_dir(splitpath, directoryEntries, size_path, superBlock, free_table, fat_table);
                }
            }
            if(strcmp(directoryEntries[i].name, last_dir) == 0){
                strcpy(directoryEntries[i].name, "");
                strcpy(directoryEntries[i].parent, "");
                directoryEntries[i].modificationTime = 0;
                directoryEntries[i].directory = 0;
                directoryEntries[i].size = 0;
                directoryEntries[i].exists = 0;
                change_in_directory = 1;
            }
        }
        change_in_fat = 1;
        change_in_free_table = 1;
        
    } else {
        printf("Directory does not exists: %s in parent: %s\n", last_dir, parent_dir);
    }
}
