#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int* calculate(const char* Equation){
	//initialize
	int* result = (int*) calloc(2, sizeof(int));
	result[0] = 0;
	result[1] = 0;
	int length = strlen(Equation);
	int operand_num = 0;
    
	int i, j;
	char c ;
	int numOfParenth = 0;
    //print current expression
    printf("PID %d: My expression is \"%s\"\n", getpid(), Equation);
    fflush(NULL);
    int x = 0;
    for (i = 1; i < length; i++) {
        c = Equation[i];
        if ( c == ' ') {
            break;
        }
        x++;
        
    }
    //print operator, deal with unkonwn operator
    char* operator = (char*)malloc(x*sizeof(char));
    for (i = 0; i < x; i++) {
        operator[i] = Equation[i + 1];
    }
    if (x != 1) {
        result[1] = -1;
        printf("PID %d: ERROR: unknown \"%s\" operator; exiting", getpid(),operator);
        fflush(NULL);
        return result;
    }
    else if(operator[0] != '+' &&
            operator[0] != '-' &&
            operator[0] != '*' &&
            operator[0] != '/' )
    {
        result[1] = -1;
        printf("PID %d: ERROR: unknown \"%s\" operator; exiting", getpid(), operator);
        fflush(NULL);
        return result;
    }
    
    
    printf("PID %d: Starting \"%c\" operation\n", getpid(), operator[0]);
    fflush(NULL);
    
    
    //get current the number of operands in current equation
	for(i = 1; i < length; i++){
		c = Equation[i];
		if (c == ' ')
		{
			operand_num++;
		}
		else if (c == '(')
		{
			numOfParenth++;
			for(j = i + 1;j < length; j++){
				if(Equation[j] == '('){
					numOfParenth++;
				}
				if(Equation[j] == ')'){
					numOfParenth--;
				}
				if(numOfParenth == 0){
					i = j;
					break;
				}
			}	
		}
		else{
			continue;
		}

	}
    //store all operands in "operands" points, each number is
    //is an operand, each subequation is an operand
	char** operands = (char**)calloc(operand_num, sizeof(char*));
	int operand_index = 0;

	for(i = 1; i < length; i++){
		c = Equation[i];

		if (c == ' ')
		{	
			operand_index++;

			if(isdigit(Equation[i + 1]) || Equation[i + 1] == '-')
			{
				char operand[100] = "";
				int index = 0;
				for(j = i + 1; j < length; j++){

					if (Equation[j] == ' ' || Equation[j] == ')')
					{
						i = j - 1;
						break;
					}
					operand[index] = Equation[j];
                    index++;
					
				}
				operands[operand_index -1] = (char*) malloc(strlen(operand));
				strncpy(operands[operand_index - 1], operand, strlen(operand));
			}
			else if(Equation[i + 1] == '('){
					int numOfParenth1 = 0;
					char sub_equation[100] = "";
					int sub_length = 0;
					for (j = i + 1; j < length; j++)
					{
						if (Equation[j] == '(')
						{
							numOfParenth1++;
						}
						if(Equation[j] == ')'){
							numOfParenth1--;
						}
						sub_equation[sub_length] = Equation[j];
						sub_length++;
						if(numOfParenth1 == 0){
							i = j;
							break;
						}
					
					}
					operands[operand_index - 1] = (char*) malloc(strlen(sub_equation));
					strncpy(operands[operand_index - 1], sub_equation, strlen(sub_equation));
			}
		}
		else{
			continue;
		}
	}
    
	int rc ;
	int** p = (int**) calloc(operand_num,sizeof(int*));
	int* sub_result;
	int index = 0;
    pid_t pid;
    //pipe and fork once for each operand
	for (i = 0; i < operand_index ; i++)
	{
		//deal with the operand is a number
		if(isdigit(operands[i][0]) || (operands[i][0] == '-')){
			p[index] = (int *) malloc(sizeof(int) * 2);
        	rc = pipe(p[index]);
        	if( rc == -1){
				perror("Pipe() failed");
				result[1] = -1;
				return result;
			}
			//pipe and fork
			pid_t pid;
			pid = fork();
			if (pid == -1)
			{
				perror("Fork() failed");
				result[1] = -1;
				return result;
			}
			if (pid == 0)
			{//for child process, write its expression in pipe
                close(p[index][0]);
                int bytes_written = write(p[index][1] , operands[i], strlen(operands[i])) ;
                if (bytes_written == -1) {
                    perror("Fail to write to pipe");
                    result[1] = -1;
                    return result;
                }
                close(p[index][1]);
                if (atoi(operands[i]) != 0)
                {
                    printf("PID %d: My expression is \"%s\"\nPID %d: Sending \"%s\" on pipe to parent\n", getpid() , operands[i], getpid(), operands[i]);
                    fflush(NULL);
                }
				exit(EXIT_SUCCESS);
			}
			else{//for parent, get expression from pipe
				close(p[index][1]);
				int bytes_read = read(p[index][0] , operands[i], 100);
                if (bytes_read == -1) {
                    perror("Fail to read from pipe");
                    result[1] = -1;
                }
                close(p[index][0]);
                if (atoi(operands[i]) == 0) {
                    printf("PID %d: ERROR: division by zero is not allowed; exiting", getpid());
                    result[1] = -1;
                    return result;
                }
				operands[i][bytes_read] = '\0';
				close(p[index][0]);

			}
			index++;

		}
		else{//deal with the operand is an equation
			p[index] = (int *) malloc(sizeof(int) * 2);
        	rc = pipe(p[index]);
        	if( rc == -1){
				perror("pipe() failed");
				result[1] = -1;
				return result;
			}
			
			
			pid = fork();
			if (pid == -1)
			{
				perror("fork() failed");
				result[1] = -1;
				return result;
			}
			if (pid == 0)
			{//for child, write the answer of sub_equation in pipe.
                close(p[index][0]);
				sub_result =  calculate(operands[i]);
				char buffer[100];
				char buffer1[2];
				snprintf (buffer, sizeof(buffer), "%d",sub_result[0]);
				snprintf (buffer1, sizeof(buffer1), "%d",sub_result[1]);
				if (sub_result[1] != -1)
				{
					printf("PID %d: Processed \"%s\"; sending \"%s\" on pipe to parent\n", getpid(), operands[i], buffer);
                    fflush(NULL);
				}
				int bytes_written = write(p[index][1] , buffer1, strlen(buffer1)) ;
				bytes_written = write(p[index][1] , buffer, strlen(buffer)) ;
                close(p[index][1]);
                if (bytes_written == -1) {
                    perror("Fail to write to pipe");
                    result[1] = -1;
                    return result;
                }
                exit(EXIT_SUCCESS);
			}
			else{//for parent, get the get answer of the sub_equation,
                //store it as the current operand, replacing the sub_equation in "operands"
				close(p[index][1]);
				char buffer2[2];
				int bytes_read = read(p[index][0] ,buffer2, 1);
                //result of sub_equation is invalid
				if (buffer2[0] == '-')
				{
					result[1] = -1;
					return result;
				}
				bytes_read = read(p[index][0] , operands[i], 100);
                if (bytes_read == -1) {
                    perror("Fail to read from pipe");
                    result[1] = -1;
                }
                close(p[index][0]);
                if (atoi(operands[i]) == 0) {
                    printf("PID %d: ERROR: division by zero is not allowed; exiting", getpid());
                    fflush(NULL);
                    result[1] = -1;
                    return result;
                }
				operands[i][bytes_read] = '\0';
				close(p[index][0]);
			}
			index++;
        }
		
	}
    //deal with child process terminated abnormally
    if (pid > 0)
    {
        int status;
        wait(&status);
        
        if (WIFSIGNALED(status))
        {
            printf("PID %d: child %d terminated abnormally\n", getpid(), pid);
            fflush(NULL);
            result[1] = -1;
        }
        else if (WIFEXITED(status))
        {
            int rc = WEXITSTATUS(status);
            if (rc != 0)
            {
                printf("PID %d: child %d terminated with nonzero exit status %d\n", getpid(), pid, rc);
                fflush(NULL);
            }
        }
    }
    
    while (waitpid(-1, NULL, 0))
    {
        if (errno == ECHILD)
        {
            break;
        }
    }
    
    
    //deal with no enough operands
	if (operand_num < 2)
				{
					printf("PID %d: ERROR: not enough operands; exiting", getpid());
                    fflush(NULL);
					result[1] = -1;
					return result ;
				}
    //process
	result[0] = atoi(operands[0]);
	for (i = 1; i < operand_index ; i++)
	{
			if (operator[0] == '+')
			{
				result[0] = result[0] + atoi(operands[i]);
			}
			if (operator[0] == '-')
			{
				result[0] = result[0] - atoi(operands[i]);
			}
			if (operator[0] == '*')
			{
				result[0] = result[0] * atoi(operands[i]);
			}
			if (operator[0] == '/')
			{

                result[0] = result[0] / atoi(operands[i]);
                
			}
	}
	return result;
}

int main(int argc, char* argv[]){
	if(argc <= 1){
		perror("ERROR: Invalid arguments\nUSAGE: ./a.out<input-file>");		
		return EXIT_FAILURE;

	}
	else{
		char* in_file = argv[1];
		FILE* fp;
		fp = fopen(in_file, "r");

		if(fp == NULL){
			perror("ERROR: no input file");
			return EXIT_FAILURE;

		}
		char* equation = (char*) malloc(100 * sizeof(char));
		while(fgets (equation, 100, fp) != NULL)

		{
			//get the equation, ignore lines start from "#" or consist of spaces
			if(equation[0] == '#' || equation[0] == ' '){
				continue;
			}
			if (equation[strlen(equation) - 1] == '\n')
			{	
				equation[strlen(equation) - 1] = '\0';
			}
			int* r;
			r = calculate(equation);
			if (r[1] != -1)
			{
				printf("PID %d: Processed \"%s\"; final answer is \"%d\"", getpid(), equation, r[0]);
                fflush(NULL);
			}
		}
	}
	return EXIT_SUCCESS;
}