//
//  homewrok3.c
//  
//
//  Created by fangyan xu on 10/29/16.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
//for each word, store its content and name of the file it belong to
struct word {
    char* string;
    char* file_name;
};
//for each file, one FILE pointer to open file, also save its name
struct file_pointer{
    FILE* fp;
    char* file;
};

//in this file, assume length of a single word is no more than 100
int MAX_WORD_LENGTH = 100;

int size = 8;
int number_of_words = 0;
struct word ** output ;
int directory_size = 100;

pthread_mutex_t reallocation = PTHREAD_MUTEX_INITIALIZER;

void reallocate_memory(unsigned int * x);
int Assign_slot(unsigned int * x);

//the whattodo function will be called by each child thread
void* read_a_file(void* a){
    //s for reading word, freed at the end of this function
    char* s = (char*)calloc(MAX_WORD_LENGTH, sizeof(char));
    //file_name for store name of file, freed at the end of this function
    char* file_name = (char*)calloc(MAX_WORD_LENGTH, sizeof(char));
    //copy the struct
    struct file_pointer * current = (struct file_pointer*)malloc(sizeof(struct file_pointer));
    memcpy(current, (struct file_pointer*)a, sizeof *(struct file_pointer*)a);
    //free the input file_pointer struct pointer, i.e the corresponding current[file_num]
    free((struct file_pointer*)a);
    //get file name
    file_name = strcpy(file_name, current->file );
    free(current->file);
    //get thread ID, freed at the end of this function
    unsigned int * x = (unsigned int*)malloc(sizeof(unsigned int));
    *x = (unsigned int)pthread_self();
    while((fscanf(current->fp,"%s",s) > 0)){
        int i = 0,j = 0;
        char* tmp = (char*)calloc(MAX_WORD_LENGTH,sizeof(char));
        //get word
        for (i = 0; i < strlen(s); i++) {
            if(isalnum(s[i]) == 0){
                continue;
            }
            else{
                tmp[j] = s[i];
                j++;
            }
        }
        
        //sychronization for reallocation and assign slot for each thread
        //use my_location to save the assigned slot position
        int my_location = 0;
        pthread_mutex_lock(&reallocation);
        //reallocation if number of words exceeds size of output
            if(number_of_words > (size - 1)){
                reallocate_memory(x);
            }
        //assign slot, allocate memory for it
            my_location = Assign_slot(x) ;
        pthread_mutex_unlock(&reallocation);
        //read in word
        output[my_location-1]->string = strcpy(output[my_location-1]->string, tmp);
        output[my_location-1]->file_name =strcpy(output[my_location-1]->file_name, file_name);
        printf("THREAD %u: Added \"%s\" at index %d.\n", *x, output[my_location-1]->string, number_of_words-1);
        free(tmp);
    }
    fclose(current->fp);
    free(current);
    free(s);
    free(file_name);
    free(x);
    pthread_exit(NULL);
    return NULL;
}
//reallocate memory if number of words is going to exceed number_of_words
void reallocate_memory(unsigned int *x){
    size = size * 2;
    output = (struct word**)realloc(output,size * sizeof(struct word*));
    printf("THREAD %u: Re-allocated array of %d pointers.\n",*x, size);
}
//each thread can only be assign a slot in its critical section 
int Assign_slot(unsigned int *x){
    number_of_words++;
    output[number_of_words-1] = (struct word*)calloc(1, sizeof(struct word));
    output[number_of_words-1]->string = (char*)calloc(MAX_WORD_LENGTH,sizeof(char));
    output[number_of_words-1]->file_name = (char*)calloc(MAX_WORD_LENGTH,sizeof(char));
    return number_of_words;
}


int main( int argc, char* argv[]){
    //Handling Error
    if (argc != 3) {
        printf("ERROR: Invalid arguments\nUSAGE: ./a.out <directory> <substring>");
    }
    //initialize memory, 8 character first
    output = (struct word**)calloc(size, sizeof(struct word*));
    printf("MAIN THREAD: Allocated initial array of 8 pointers.\n");
    
    //open directory
    DIR *dirp;
    struct dirent *dp;
    if ((dirp = opendir(argv[1])) == NULL) {
        perror("ERROR: Can not open this directory!" );
        return -1;
    }
    //current is an array of file_pointer pointers, each of them have a FILE pointer to open a file and a 
    //char pointer to save the file name
    struct file_pointer** current = (struct file_pointer**)calloc(directory_size,sizeof(struct file_pointer*));
    char * directory_name = (char*)calloc(MAX_WORD_LENGTH,sizeof(char));
    strcpy(directory_name, argv[1]);
    int file_num = 0;
    //dynamically allocate memory for saving threads ID
    pthread_t* tid = (pthread_t *)calloc( directory_size , sizeof(pthread_t));
    int i, rc;
    while ((dp = readdir(dirp)) != NULL) {
        //ignore such files
        if (!strcmp (dp->d_name, "."))
        {
            continue;
        }
        if (!strcmp (dp->d_name, "..")){
            continue;
        }
        if (!strcmp (dp->d_name, ".DS_Store")){
            continue;
        }
        //reallocate if number of files is going to exceeds directory_size
        if (file_num > directory_size - 1) {
            directory_size = directory_size * 2;
            tid = (pthread_t *)realloc( tid ,sizeof(pthread_t));
            current = (struct file_pointer**)realloc(current,directory_size * sizeof(struct file_pointer*));
        }
        //get file path
        char* file_path = (char*)calloc(MAX_WORD_LENGTH,sizeof(char));
        file_path = strcpy(file_path,directory_name);
        file_path = strcat(file_path,"/");
        file_path = strcat(file_path, dp->d_name);
        //open file
        current[file_num] = (struct file_pointer*)malloc(sizeof(struct file_pointer));
        current[file_num]->fp = fopen(file_path, "r");
        current[file_num]->file = (char*)calloc(MAX_WORD_LENGTH, sizeof(char));
        strcpy(current[file_num]->file, dp->d_name);
        //get file name
        char* name = (char*)calloc(MAX_WORD_LENGTH,sizeof(char));
        strcpy(name, dp->d_name);
        
        if (current[file_num]->fp == NULL) {
            fprintf(stderr,"ERROR: fail to open %s file\n", file_path);
            return EXIT_FAILURE;
        }
        //create child thread and assign files to them
        rc = pthread_create(&tid[file_num], NULL, read_a_file, current[file_num]);
        if (rc != 0) {
            fprintf(stderr, "ERROR: Fail to create thread for %s file!\n", current[file_num]->file);
        }
        printf("MAIN THREAD: Assigned \"%s\" to child thread %u.\n", name, (unsigned int)tid[file_num]);
        free(file_path);
        free(name);
        file_num++;
    }
    free(directory_name);
    closedir(dirp);
    free(current);
    for (i = 0; i < file_num ; i++) {
        unsigned int * x;
        pthread_join(tid[i], (void **)&x);
        free(x);
    }
    free(tid);

    //then main thread examine substring
    char* substr = argv[2];
    printf("MAIN THREAD: All done (successfully read %d words from %d files).\n", number_of_words, file_num);
    printf("MAIN THREAD: Words containing substring \"%s\" are:\n", substr);
    int j,k;
    
    if(substr){
        //for each word, check if there is substring the same as the given string
        for(i = 0; i < number_of_words; i++){
            for(j = 0; j < strlen(output[i]->string) ; j++){
                for(k = 0; k < strlen(substr);k++){
                    if(output[i]->string[j+k] != substr[k]){
                        break;
                    }
                }
                if(k == strlen(substr)){
                    printf("MAIN THREAD: %s (from \"%s\")\n", output[i]->string, output[i]->file_name);
                }
            }
            //after checking, free the allocated memory to the word;
            free(output[i]->string);
            free(output[i]->file_name);
            free(output[i]);
        }
        free(output);
    }
    return EXIT_SUCCESS;
}



