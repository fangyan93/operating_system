#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h> 
#include <fcntl.h>

#define BUFFER_SIZE 1024
//store information of each file
struct file_info {
    char* file_name;
    int size;
};
//pass as the single argument of function executed by threads
struct all_info{
  struct file_info* file_list;
  int* newsock;
};
int file_list_size = 8;
int file_num = 0;
int client_num = 5;
int count_client = 0;
pthread_mutex_t reallocation = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t store = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sort_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t read_m = PTHREAD_MUTEX_INITIALIZER;

//Assume the command will be correct
int parse_command(char command[BUFFER_SIZE]){
  if(command[0] == 'S'){//Store
    return 1;
  }
  else if(command[0] == 'R'){//read
    return 2;
  }
  else if(command[0] == 'L'){//list
    return 3;
  }
  else{//unknown command
  }
  return 0;
}
//for list all file names in alphabetical order
int compare_fileName(const void* a, const void* b){
  return ( (*(struct file_info *)a).file_name[0] - (*(struct file_info*)b).file_name[0] );
}

//the function executed by threads
void* communicate(void* whattodo){
  struct all_info* now = (struct all_info*) whattodo;
  struct file_info* file_list = now->file_list;

  int* newsock = (int*)malloc(sizeof(int));
  *newsock = *now->newsock;
  free(now->newsock);
  int n;
  char buffer[ BUFFER_SIZE ];

do
{
      n = recv( *newsock, buffer, BUFFER_SIZE, 0 );
      int i;
      int m;
      if ( n < 0 )
      {
        printf( "[child %u] ERROR recv() failed", (unsigned int)pthread_self());
      }
      else if(n == 0){
        break;
      }
      else
      {
        int command = parse_command(buffer);
      //1 store, 2 read, 3 list
        if(command == 1){//-------------------STORE------------------------//
          int start = 6;
          int size = 0;
          //get file name
          char* file_name = (char*)calloc(100, sizeof(char));
          while(buffer[start] != ' '){
            start++;
          }
          strncpy(file_name, &buffer[6], start - 6);
          file_list[file_num].file_name = (char*)calloc(start - 6, sizeof(char));
          //get size
          char* size_s = (char*)calloc(10, sizeof(char));
          int end = start;
          while(buffer[end] != '\n'){
            end++;
          }  
          strncpy(size_s, &buffer[start + 1], end - start - 1);
          size = atoi(size_s);
          printf("[child %u] Received STORE %s %d\n", (unsigned int)pthread_self() , file_name, size);
          fflush(NULL);
          int i ;
          if(strlen(file_name) == 0){
            m = send(*newsock, "ERROR INVALID REQUEST\n", 22, 0);
            if(m != 22){
              printf("[child %u] ERROR SEND FAILED\n", (unsigned int)pthread_self());
            }
            printf("[child %u] Sent ERROR INVALID REQUEST\n", (unsigned int)pthread_self());
            fflush(NULL);
            continue;
          }
          int not_valid = 0;
          for(i = 0; i < strlen(file_name); i++){
            if(!isalnum(file_name[i]) && file_name[i] != '.'){
              m = send(*newsock, "ERROR INVALID REQUEST\n", 22, 0);
              if(m != 22){
                printf("[child %u] ERROR SEND FAILED\n", (unsigned int)pthread_self());              
              }
              printf("[child %u] Sent ERROR INVALID REQUEST\n", (unsigned int)pthread_self());
              fflush(NULL);
              not_valid = 1;
              break;
            }
          }
          if(not_valid == 1){
            continue;
          }
          //check if it is already exist
          int not_exist = 0;
          for(i = 0; i < file_num; i++){
            if(strcmp(file_name, file_list[i].file_name) == 0){
              m = send(*newsock, "ERROR FILE EXISTS\n", 19, 0);
              if(m != 19){
                printf("[child %u] ERROR SEND FAILED\n", (unsigned int)pthread_self());              
              }
              printf("[child %u] Sent ERROR FILE EXISTS\n", (unsigned int)pthread_self());
              fflush(NULL);
              not_exist =1;
              break;
            }
          }
          if(not_exist == 1){
            continue;
          }
          
          FILE* fp = NULL;
          int fd;
          if ((fd = open(file_name, O_RDWR | O_CREAT, 0666)) >= 0)
          fp = fdopen(fd, "rb+");
          //begin store
          int count = 0;
          pthread_mutex_lock(&store);
          if(size <= BUFFER_SIZE - end){
            count = BUFFER_SIZE - end - 1;
            char* content = (char*)malloc((BUFFER_SIZE - end )*sizeof(char)) ;
            strncpy(content, buffer + end + 1, BUFFER_SIZE - end - 1);
            write(fd, content, BUFFER_SIZE - end - 1);
          }
          while(count < size){
            int tmp = 0;
            if(count + BUFFER_SIZE <= size){
              tmp = recv( *newsock, buffer, BUFFER_SIZE, 0 );
              if(tmp == 0){
                break;
              }
            }else{
              tmp = recv( *newsock, buffer, BUFFER_SIZE, 0 );
              if(tmp == 0){
                break;
              }
            }
            count = count + tmp ;
            tmp = write(fd, buffer, tmp);
          }

          printf("[child %u] Stored file \"%s\" (%d bytes)\n", (unsigned int)pthread_self() , file_name, size);
          fflush(NULL);
          //store file information
          strcpy(file_list[file_num].file_name, file_name);
          file_list[file_num].size = size;
          file_num++;
          pthread_mutex_unlock(&store);
          //reallocation the file_list is also only allowed in one thread
          pthread_mutex_lock(&reallocation);
            if(file_num >= file_list_size){
              file_list_size *= 2;
              file_list = (struct file_info*)realloc(file_list, file_list_size * sizeof(struct file_info));
            }
          pthread_mutex_unlock(&reallocation);
          m = send( *newsock, "ACK\n", 4, 0 );
          if(m != 4){
            printf("[child %u] ERROR SEND FAILED\n", (unsigned int)pthread_self());
          }
          printf("[child %u] Sent ACK\n", (unsigned int)pthread_self());
          fflush(NULL);
          fclose(fp);
          free(size_s);
          free(file_name);
          chmod(file_name, 0666);
          fflush(NULL);
        }
        else if(command == 2){//-------------------READ------------------------//
          int start = 5;
          int length = 0;
          int byte_offset = 0;
          char* file_name = (char*)calloc(100, sizeof(char));
          while(buffer[start] != ' '){
            start++;
          }
          strncpy(file_name, &buffer[5], start - 5);
          //get byte-offset
          char* offset = (char*)calloc(10, sizeof(char));
          int end = start + 1;
          start = start + 1;
          while(buffer[end] != ' '){
            end++;
          }  
          strncpy(offset, &buffer[start], end - start);
          byte_offset = atoi(offset);
          //get length
          char* length_s = (char*)calloc(10, sizeof(char));
          start = end + 1;
          end = end + 1;
          while(buffer[end] != '\n'){
            end++;
          }  
          strncpy(length_s, &buffer[start], end - start );
          length = atoi(length_s);
          printf("[child %u] Received READ %s %d %d\n", (unsigned int)pthread_self() ,file_name, byte_offset, length );
          fflush(NULL);
          free(offset);
          //check if file name empty or invalid
          int i ;
          if(strlen(file_name) == 0){
            m = send(*newsock, "ERROR INVALID REQUEST\n", 22, 0);
            if(m != 22){
              printf("[child %u] ERROR SEND FAILED\n", (unsigned int)pthread_self());              
            }
            printf("[child %u] Sent ERROR INVALID REQUEST\n", (unsigned int)pthread_self());
            fflush(NULL);
            continue;
          }
          int not_valid = 0;
          for(i = 0; i < strlen(file_name); i++){
            if(!isalnum(file_name[i]) && file_name[i] != '.'){
              m = send(*newsock, "ERROR INVALID REQUEST\n", 22, 0);
              if(m != 22){
                printf("[child %u] ERROR SEND FAILED\n", (unsigned int)pthread_self());              
              }
              printf("[child %u] Sent ERROR INVALID REQUEST\n", (unsigned int)pthread_self());
              fflush(NULL);
              not_valid = 1;
              break;
            }
          }
          if(not_valid == 1){
            continue;
          }
          //check if it does not exist
          int found = 0;
          for(i = 0; i < file_num; i++){
            if(strcmp(file_name, file_list[i].file_name) == 0){
              found = 1;
            }
          }
          if(found == 0){
            m = send(*newsock, "ERROR NO SUCH FILE\n", 19, 0);
            if(m != 19){
              printf("[child %u] ERROR SEND FAILED\n", (unsigned int)pthread_self());              
            }
            printf("[child %u] Sent ERROR NO SUCH FILE\n", (unsigned int)pthread_self());
            fflush(NULL);
            continue;
          }
          //check if the range is valid
          int range_invalid = 0;
          for(i = 0; i < file_num; i++){
            if(strcmp(file_list[i].file_name , file_name) == 0){
              if(file_list[i].size - byte_offset < length){
                m = send(*newsock, "ERROR INVALID BYTE RANGE\n", 25, 0);
                if(m != 25){
                  printf("[child %u] ERROR SEND FAILED\n", (unsigned int)pthread_self());              
                }
                printf("[child %u] Sent ERROR INVALID BYTE RANGE\n", (unsigned int)pthread_self());
                fflush(NULL);
                range_invalid = 1;
                break;
              }
            }
          }
          if(range_invalid == 1){
            continue;
          }
          //read 
          // pthread_mutex_lock(&read_m);
          FILE * fp  = fopen(file_name, "rb");
          // pthread_mutex_unlock(&read_m);
          fseek(fp, byte_offset, SEEK_SET);
          char content[length];
          int fang = fread(content, length, 1, fp);
          fclose(fp);
          if(fang != 0){
            char* tmp = (char*)malloc(5+strlen(length_s)*sizeof(char));
            int read_size  = sprintf(tmp, "ACK %d\n",length);
            m = send(*newsock, tmp, read_size, 0);
            free(tmp);
            usleep(100);
            m = send(*newsock, content, length, 0);
            if(m != length){
              printf("[child %u] ERROR SEND FAILED\n", (unsigned int)pthread_self());              
            }
            printf("[child %u] Sent ACK %d\n", (unsigned int)pthread_self(), length);
            fflush(NULL);
            printf("[child %u] Sent %d bytes of \"%s\" from offset %d\n", (unsigned int)pthread_self(), length, file_name, byte_offset);
            fflush(NULL);
            free(length_s);
            free(file_name);
          }
          fflush(NULL);
        }
        else if(command == 3 ){//-------------------LIST------------------------//
          printf("[CHILD %u] Received LIST\n", (unsigned int)pthread_self());
          fflush(NULL);
          pthread_mutex_lock(&sort_m);
          qsort(file_list, file_num, sizeof(struct file_info), compare_fileName);
          pthread_mutex_unlock(&sort_m);
          //assume file num less than 100,000,000
          char* tmp = (char*)calloc(10, sizeof(char));
          int list_size = sprintf(tmp, "%d", file_num);
          for(i = 0; i < file_num; i++){
            list_size += strlen(file_list[i].file_name) + 1;
          }
          list_size += 1;//new line character
          char* file_num_s = (char*)calloc(list_size, sizeof(char)) ;
          sprintf(file_num_s, "%d", file_num);
          for(i = 0; i < file_num ; i++){
            strcat(file_num_s, " ");
            strcat(file_num_s, file_list[i].file_name);
          }
          strcat(file_num_s, "\n");
          m = send(*newsock, file_num_s, list_size, 0);
          if(m != list_size){
            printf("[child %u] ERROR SEND FAILED\n", (unsigned int)pthread_self());              
          }
          printf("[child %u] Sent %d", (unsigned int)pthread_self(), file_num);
          fflush(NULL);
          for(i = 0; i < file_num; i++){
            printf(" %s", file_list[i].file_name);
          }
          printf("\n");
          fflush(NULL);
          free(file_num_s);
        }
      }
      usleep(100);
}
while ( n > 0 );
      printf( "[child %u] Client disconnected\n", (unsigned int)pthread_self() );
      fflush(NULL);
      close( *newsock );
      free(newsock);
      free(now);
      pthread_exit(NULL);
      return NULL;  
      /* thread terminates here! */
}


int main(int argc, char** argv)
{
  if(argc != 2){
    printf("ERROR NO ENOUGH COMMAND ARGUMENTS!\n");
  }
  //Create the listener socket as TCP socket 
  int sd = socket( PF_INET, SOCK_STREAM, 0 );
  if ( sd < 0 )
  {
    perror( "socket() failed" );
    exit( EXIT_FAILURE );
  }
  struct sockaddr_in server;
  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  unsigned short port = atoi(argv[1]);
  server.sin_port = htons( port );
  int len = sizeof( server );
  if ( bind( sd, (struct sockaddr *)&server, len ) < 0 )
  {
    perror( "bind() failed" );
    exit( EXIT_FAILURE );
  }
  listen( sd, 5 ); 
  printf( "Started server; listening on port\n" );
  fflush(NULL);
  struct sockaddr_in client;
  int fromlen = sizeof( client );
  pthread_t* tid = (pthread_t *)calloc( client_num , sizeof(pthread_t));
  
  struct file_info* file_list = (struct file_info*)calloc(file_list_size, sizeof(struct file_info));
  while ( 1 )
  {
    int newsock = accept( sd, (struct sockaddr *)&client,
                          (socklen_t*)&fromlen );
    printf( "Received incoming connection from: %s\n",  inet_ntoa( (struct in_addr)client.sin_addr ));
    fflush(NULL);
    count_client++;
    if(count_client >= client_num ){
      client_num *= 2;
      tid = (pthread_t *)realloc( tid , client_num * sizeof(pthread_t));
    }
    //pass command, file_list
    struct all_info* tmp = (struct all_info*)malloc(sizeof(struct all_info));
    tmp->file_list = file_list;
    tmp->newsock = (int* )malloc(sizeof(int));
    *tmp->newsock = newsock;
    int rc;
    //create one thread for each client
    rc = pthread_create(&tid[count_client], NULL, communicate, tmp);
    if (rc != 0) {
      perror("ERROR: fail to create thread");
      exit( EXIT_FAILURE );
    }   
  }
  int i;
  for (i = 0; i < count_client ; i++) {
    unsigned int * x;
    pthread_join(tid[i], (void **)&x);
    free(x);
  }
  for(i = 0; i < file_num; i++){
    free(file_list[i].file_name);
  }
  free(file_list);
  free(tid);
  close( sd );
  return EXIT_SUCCESS;
}