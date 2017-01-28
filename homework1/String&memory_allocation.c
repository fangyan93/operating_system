/* main.c */
/* Number of words that cause out-of-memory is unknown*/
/* I have tested 16563669 words, but memory is still not full*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
int main(int argc, char * argv[]){

	//open the input file
	FILE * fp;
	fp = fopen(argv[1],"r");
	if(!fp){
		fprintf(stderr,"fail to open %s file", argv[1]);
		return EXIT_FAILURE;
	}
	char* substr = argv[2];
	if(substr == NULL){
		fprintf(stderr,"No target substring given");
		return EXIT_FAILURE;
	}
	//read and dynamically save all words
	int size = 8;
	char** output = (char**)calloc(size, sizeof(char*));
	printf("Allocated initial array of 8 character pointers.\n");
	//use "s" save each word scaned from input file, suppose the number of 
	//characters in the word is no larger than 20
	char* s = (char*)calloc(20, sizeof(char));
	//use "num_of_word" to count the number of words in the file
	int num_of_word = 0;
	
	while((fscanf(fp,"%s",s) > 0)){
		//reallocate size of output if the number of words eceeds 
		//current size
		if(num_of_word > (size - 1)){
			size = size * 2;
            
			output = (char**)realloc(output,size *2* sizeof(char*));
//            output = (char**)realloc(output,size * sizeof(char*));

            printf("Re-allocated array of %d character pointers.\n",size);
		}
		//this is like a bool variable, to check if the word got using 
		//scanf has a punctuation
		int haspunc = 0;
		int j;
		//use "end_pos" to get the position of the first punctuation  
		int end_pos = 0;
		output[num_of_word] = (char*)calloc(20, sizeof(char));
		//if there is punctuation, only save all characters before the punctuation
		for(j = 0; j < strlen(s); j++){
			if(isalnum(s[j]) == 0){
				end_pos = j;
				haspunc = 1;
				output[num_of_word] = strncpy(output[num_of_word], s, end_pos);
				break;
			}
		}
		//if there is no punctuation, save the word
		if(haspunc == 0){
			output[num_of_word] = strncpy(output[num_of_word], s, strlen(s));
		}

		num_of_word++;
	}
	fclose(fp);
	printf("All done (successfully read %d words).\n", num_of_word);

	
	printf("Words containing substring \"%s\" are:\n", substr);
	int i,j,k;
	if(substr){
	//for each word, check if there is substring the same as the given string
		for(i = 0; i < num_of_word; i++){
			for(j = 0; j < strlen(output[i]); j++){
				for(k = 0; k < strlen(substr);k++){
					if(output[i][j+k] != substr[k]){
						break;
					}
				}
				if(k == strlen(substr)){
					printf("%s\n", output[i]);
				}
			}
			//after checking, free the allocated memory to the word;
			free(output[i]);
		}
		free(output);
	}
	return 0;
}
