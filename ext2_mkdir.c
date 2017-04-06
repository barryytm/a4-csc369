#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include "ext2.h"
#include "filesystem.h"


/*
int string_size(char *c){
    int i = 0;
    
    while(c[i] != '\0'){
        i++;
    }
    
    return i;
}

int *make_bitmap(unsigned char * bitmap, int bitmap_length){
	int *map = malloc(sizeof(int) * bitmap_length);
	int i,j;
	for (i = 0; i < bitmap_length / 8; i++) {
		for (j = 0; j < 8; j++) {
			map[i*8 + j] = (bitmap[i] & 1 << j) >> j;
		}
	}

	return map;
}

int num_directories_in_path(char * path){
	int count = 0;
	int i = 0;
	
	while(path[i] != '\0'){
		if(path[i] == '/'){
			count++;
		}
		i++;
	}

	return count + 1;
}

void split_path(char * path, char ** result){
	int i = 0;

	char * temp_path = malloc(sizeof(path));

	strcpy(temp_path, path);

	char * temp = strtok(temp_path, "/");

	while(temp != NULL){
		result[i] = malloc(sizeof(temp));
		strcpy(result[i], temp);
		temp = strtok(temp_path, "/");
		i++;
	}
}

char ** make_path(char *path, int *num_directories){
	char ** result;
	*num_directories = num_directories_in_path(path);

	result = malloc((*num_directories) * sizeof(char *));

	split_path(path, result);

	return result;
}

int check_block(unsigned char * disk, int curr_block, char * dir_name){
	int rec_length = 0;
	int dir_name_len = string_size(dir_name);

	char * temp = malloc(dir_name_len);

	strncpy(temp, dir_name, dir_name_len);

	struct ext2_dir_entry * directory_entry = (struct ext2_dir_entry *) (disk + curr_block * EXT2_BLOCK_SIZE);

	while(rec_length < EXT2_BLOCK_SIZE){
		if(strcmp(temp, directory_entry->name)){
			return directory_entry->inode;
		}
		rec_length += directory_entry->rec_len;
	}

	return -1;
}

int find_free_inode(unsigned char * inode_bitmap, int num_inodes){
	int bit;
	int i;

	for(i = 0; i < num_inodes; i++){
		if(i == 1 || i >= 11){
			bit = (inode_bitmap[i/8] & 1 << (i % 8)) >> (i % 8);
			if(bit == 0){
				return i + 1;
			}
		}
	}

	perror("No free inode.\n");
	exit(ENOENT);
}

int find_free_block(unsigned char * block_bitmap, int num_blocks){
	int bit;
	int i;
	
	for(i = 0; i < num_blocks; i++){
		if(i == 1 || i >= 11){
			bit = (block_bitmap[i/8] & 1 << (i % 8)) >> (i % 8);
			if(bit == 0){
				return i + 1;
			}
		}
	}

	perror("No free inode.\n");
	exit(ENOENT);
}
*/

int main(int argc, char **argv){
	unsigned char *disk;
	if(argc != 3) {
        fprintf(stderr, "Usage: %s <image file name> <absolute path of directory>\n", argv[0]);
        exit(-1);
    }

    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    char *path = argv[2];
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 2 * EXT2_BLOCK_SIZE);

    struct ext2_inode * it = (struct ext2_inode *)(disk + gd->bg_inode_table * EXT2_BLOCK_SIZE);

	unsigned char *block_bitmap = disk + gd->bg_block_bitmap * EXT2_BLOCK_SIZE;
	unsigned char *inode_bitmap = disk + gd->bg_inode_bitmap * EXT2_BLOCK_SIZE;

    int num_dirs;
	char ** seperated_path = make_path(path, &num_dirs);

	int num_checked_dirs = 0;
	int not_done = 1;
	int curr_inode = 2, parent_inode = 2;
	int curr_block;

	struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
	int num_inodes = sb->s_inodes_count;
	int num_blocks = sb->s_blocks_count;

	while(not_done){
		curr_block = it[curr_inode - 1].i_block[0];

		int temp = curr_inode;
		printf("%s\n", seperated_path[num_checked_dirs]);
		curr_inode = check_block(disk, curr_block, seperated_path[num_checked_dirs]);

		printf("Curr_inode: %d\n", curr_inode);
		num_checked_dirs++;

		if(num_checked_dirs == num_dirs){
			not_done = 0;
			if(curr_inode){
				perror("Path already exists.\n");
				exit(EEXIST);
			}
		}else if(curr_inode == 0){
			perror("Path does not exist.\n");
			exit(ENOENT);
		}else{
			parent_inode = temp;
		}
	}


 	int free_inode = get_empty_inode(inode_bitmap, num_inodes);
 	int free_block = get_empty_data_block(block_bitmap, num_blocks);

 	printf("Current Inode: %d\nParent Inode: %d\nFree Inode: %d\nFree Block: %d\n", curr_inode, parent_inode, free_inode, free_block);

 	struct ext2_inode * ind = &it[free_inode - 1];
 	struct ext2_dir_entry * new_block =  (struct ext2_dir_entry *) (disk + (free_block * EXT2_BLOCK_SIZE));

 	
 	struct ext2_inode *parent_ind = &it[parent_inode - 1];
 	
 	//char *names[3] = {".", "..", seperated_path[num_checked_dirs - 1]};
 	printf("Block Number: %d\n", free_block);
 	new_block->rec_len = 12;
 	new_block->name_len = 1;
 	new_block->inode = free_inode;
 	strcpy(new_block->name, ".");

 	new_block = (void *)new_block + new_block->rec_len;

 	new_block->rec_len = 1012;
 	new_block->name_len = 2;
 	new_block->inode = parent_inode;
 	strcpy(new_block->name, "..");

 	block_bitmap[(free_block - 1) / 8] |= (1 << (free_block - 1) % 8);
 	gd->bg_free_blocks_count -= 1;

 	printf("current inode mode: %d\n", ind->i_mode);
 	ind->i_mode = EXT2_S_IFDIR;
 	ind->i_block[0] = free_block;
    ind->i_size = EXT2_BLOCK_SIZE;
    ind->i_blocks = 1;
    ind->i_links_count = 2;

    printf("current inode mode: %d\n", ind->i_mode);
    printf("block: %d\n", ind->i_block[0]);
    printf("size: %d\n", ind->i_size);
    printf("num of blocks: %d\n", ind->i_blocks);
    printf("link count: %d\n", ind->i_links_count);

    inode_bitmap[(free_inode - 1) / 8] |= (1 << (free_inode - 1) % 8);
    
    gd->bg_used_dirs_count += 1;
    gd->bg_free_inodes_count -= 1;

    parent_ind->i_links_count += 1;

    struct ext2_dir_entry *curr_dir;
    int rec_length = 0;
    /*
    int i;
    for(i = 0; i < 15; i++){
    	printf("%d\n", parent_ind->i_block[i]);
    }*/

    	if(parent_ind->i_block[0]){
    		curr_dir = (struct ext2_dir_entry *) (disk + parent_ind->i_block[0] * EXT2_BLOCK_SIZE);
    		//curr_dir = (void *)curr_dir + 12;
    		//printf("%s\n", curr_dir->name);
	    	while(rec_length < EXT2_BLOCK_SIZE){
	    		//printf("%d\n", curr_dir->rec_len);
	    		if(rec_length + curr_dir->rec_len >= EXT2_BLOCK_SIZE ){
	    			if(curr_dir->name_len % 4 != 0){
	    				curr_dir->rec_len = curr_dir->name_len + 12 - (curr_dir->name_len % 4);
	    			}else{
	    				curr_dir->rec_len = curr_dir->name_len + 8;
	    			}

	    			rec_length += curr_dir->rec_len;
	    			curr_dir = (void *)curr_dir + curr_dir->rec_len;

	    			curr_dir->inode = free_inode;
	    			curr_dir->rec_len = EXT2_BLOCK_SIZE - rec_length;
	    			curr_dir->name_len = strlen(seperated_path[num_dirs - 1]);
	    			strcpy(curr_dir->name, seperated_path[num_dirs - 1]);
	    			curr_dir->file_type = EXT2_FT_DIR;
	    			rec_length += curr_dir->rec_len;
	    			printf("returning\n");
	    			return 0;
	    		}
	    		else{
	    			rec_length += curr_dir->rec_len;
	    			curr_dir = (void *)curr_dir + curr_dir->rec_len;
	    		}
	    	}
	    }
    	
    printf("Failed.\n");	

	return -1;
}