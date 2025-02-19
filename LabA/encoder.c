#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void encode(FILE *input, FILE *output, int encoding_mode, const char *encoding_string);
int encryptAscii(int lower, int higher, int encoding_mode, int inputCh, const char *encoding_string, int i);
int getStringLength(const char *str);
void processArguments(int argc, char **argv, FILE **input, FILE **output, int *encoding_mode, const char **encoding_string, int *debug_mode);

int main(int argc, char **argv){
    FILE *input = stdin;
    FILE *output = stdout;
    const char *encoding_string = NULL;
    int debug_mode = 1;
    int encoding_mode = 0;
    processArguments(argc, argv, &input, &output, &encoding_mode, &encoding_string, &debug_mode);
    if (encoding_mode != 0 && encoding_string != NULL){
        encode(input, output, encoding_mode, encoding_string);
    }
    fclose(input);
    fclose(output);
    return 0;
}

void processArguments(int argc, char **argv, FILE **input, FILE **output, int *encoding_mode, const char **encoding_string, int *debug_mode){
    for (int i = 1; i < argc; i++){
        if (strlen(argv[i]) > 2){
            if (argv[i][0] == '-' && argv[i][1] == 'D'){
                *debug_mode = 0;
            }
            else if (argv[i][0] == '+' && argv[i][1] == 'D'){
                *debug_mode = 1;
            }
            if (*debug_mode){
                fprintf(stderr, "Debug mode activated for: %s\n", argv[i]);
            }
            if (argv[i][0] == '+' && argv[i][1] == 'E'){
                *encoding_mode = 1;
                *encoding_string = argv[i] + 2;
            }
            else if (argv[i][0] == '-' && argv[i][1] == 'E'){
                *encoding_mode = -1;
                *encoding_string = argv[i] + 2;
            }
            if (argv[i][0] == '-' && argv[i][1] == 'o'){
                *output = fopen(argv[i] + 2, "w");
            }
            else if (argv[i][0] == '-' && argv[i][1] == 'i'){
                *input = fopen(argv[i] + 2, "r");
                if (!*input){
                    fprintf(stderr, "Error opening input file\n");
                    exit(1);
                }
            }
        }
    }
}

void encode(FILE *input, FILE *output, int encoding_mode, const char *encoding_string){
    int inputCh;
    int outputCh;
    int i = 0;
    while ((inputCh = fgetc(input)) != EOF){
        if (encoding_string[i] == '\0'){
            i = 0;  
        }
        if (inputCh >= '0' && inputCh <= '9'){
            outputCh = inputCh + encoding_mode * (encoding_string[i] - '0');
        }
        else if (inputCh >= 'A' && inputCh <= 'Z'){
            outputCh = encryptAscii('A', 'Z', encoding_mode, inputCh, encoding_string, i);
        }
        else if (inputCh >= 'a' && inputCh <= 'z'){
            outputCh = encryptAscii('a', 'z', encoding_mode, inputCh, encoding_string, i);
        }
        else{
            outputCh = inputCh;  // For non-alphanumeric characters
        }
        i++;
        fputc(outputCh, output);
    }
}

int encryptAscii(int lower, int higher, int encoding_mode, int inputCh, const char *encoding_string, int i){
    int outputCh = inputCh + encoding_mode * (encoding_string[i] - '0');
    if (outputCh > higher){
        outputCh = outputCh - (higher - lower + 1);
    }
    else if (outputCh < lower){
        outputCh = higher - (lower - outputCh - 1);
    }

    return outputCh;
}

int getStringLength(const char *str){
    int len = 0;
    while (str[len] != '\0'){
        len++;
    }
    return len;
}
