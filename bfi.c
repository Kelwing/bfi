/*
   Copyright 2014 Jacob Wiltse

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// STD Libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// File IO
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// String and mem stuff
#include <string.h>

// Error handling
#include <errno.h>

// Globals for stack stuff
int stack_pos = 0;
off_t * loop_stack;
int stack_size;

void init_stack(int size){
    stack_size = size;
    if((loop_stack = malloc(sizeof(off_t)*size)) == NULL){
        fprintf(stderr, "Could not allocate memory: %s", strerror(errno));
        exit(1);
    }
}

void push_stack(off_t offset){
    stack_pos++;
    if(stack_pos > stack_size){
        if((loop_stack = realloc(loop_stack,sizeof(off_t)*stack_size*2)) == NULL){
            fprintf(stderr, "Could not realloc memory: %s", strerror(errno));
            exit(1);
        }
        stack_size = stack_size*2;
    }
    loop_stack[stack_pos] = offset;
}

off_t pop_stack(){
    if(stack_pos == 0){
        fprintf(stderr, "Syntax Error: No matching opening bracket");
        exit(1);
    }
    off_t temp = loop_stack[stack_pos];
    stack_pos--;
    return temp;
}

void free_stack(){
    free(loop_stack);
}

int main(int argc, char** argv){
    static char usage[] = "usage: %s -f filename [-m memsize]\n";
    extern char *optarg;
    char * filename, insize;
    int c, fd, fflag = 0, err = 0, memsize = 1024, debug = 0;
    // Initialize the loop stack with a size of 10
    init_stack(50);
    while ((c = getopt(argc, argv, "m:f:d")) != -1)
        switch(c){
            case 'f':
                filename = optarg;
                fflag = 1;
                break;
            case 'm':
                memsize = atoi(optarg);
                break;
            case 'd':
                debug = 1;
                break;
            default:
                //error state
                err = 1;
                break;
        }
    if(fflag == 0){
        fprintf(stderr, "%s: missing filename parameter\n", argv[0]);
        fprintf(stderr, usage, argv[0]);
        exit(1);
    } else if (err){
        fprintf(stderr, usage, argv[0]);
        exit(1);
    }

    char memspace[memsize];
    char * mem = memspace;
    memset(memspace,0,memsize);
    if((fd = open(filename, O_RDONLY)) == -1){
        fprintf(stderr, "Cannot open file: %s", strerror(errno));
        exit(1);
    }

    lseek(fd, 0, SEEK_END);
    long file_size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);

    if(debug)
        printf("file_size: %d\n", file_size);

    char * program = malloc(sizeof(char) * file_size);
    int read_in;
    if((read_in = read(fd, program, file_size)) < 0){
        fprintf(stderr, "Could not read in program: %s", strerror(errno));
        exit(1);
    }

    printf("Brainfuck Interpreter by Jacob Wiltse\n");
    printf("Memory Size: %d\n\n", memsize);

    long pos = 0;
    char input;
    while(pos < file_size){
        //printf("in loop");
        input = program[pos];
        pos++;
        if(debug)
            printf("input: %c\n", input);
        switch(input){
            case '>':
                ++mem;
                break;
            case '<':
                --mem;
                break;
            case '+':
                ++*mem;
                break;
            case '-':
                --*mem;
                break;
            case '.':
                putchar(*mem);
                break;
            case ',':
                *mem = getchar();
                break;
            case '[':
                if(*mem == 0) {
                    int depth = 0;
                    while(input != ']'){
                        input = program[pos];
                        pos++;
                        if(input == '[')
                            depth++;
                        if(input == ']' && depth > 0){
                            input = ' ';
                            depth--;
                        }
                    }
                } else {
                    push_stack(pos);
                }
                break;
            case ']':
                if(*mem != 0){
                    long last_pos = pop_stack();
                    push_stack(last_pos);
                    pos = last_pos;
                } else pop_stack();
                break;
            default:
                // Skip over non-brainfuck characters
                break;
        }
    }

    if(insize == -1){
        fprintf(stderr, "Connot read file: %s", strerror(errno));
        exit(1);
    }
    free_stack();
    free(program);
    return 0;
}
