#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <stdbool.h>

#define SIZED_ARRAY 5

// Map function to apply a transformation to each character in the array
char *map(char *array, int array_length, char (*f)(char))
{

    char *mapped_array = (char *)(malloc(array_length * sizeof(char)));

    for (int i = 0; i < array_length; i++)
    {
        mapped_array[i] = f(array[i]);
    }
    return mapped_array;
}

char my_get(char c)
{
    return fgetc(stdin);
}

/* If c is a number between 0x20 and 0x7E, cprt prints the character of ASCII value c followed by a new line. Otherwise, cprt prints the dot ('.') character. After printing, cprt returns the value of c unchanged. */
char cprt(char c)
{
    if (c >= 0x20 && c <= 0x7E)
    {
        printf("%c\n", c);
    }
    else
    {
        printf(".\n");
    }
    return c;
}

char encrypt(char c)
{
    /* Gets a char c and returns its encrypted form by adding 1 to its value. If c is not between 0x1F and 0x7E it is returned unchanged */
    if ((c <= 0x7E) && (c >= 0x1F) )
        return c + 0x01;
    else
        return c;
}

char decrypt(char c)
{
    /* Gets a char c and returns its decrypted form by reducing 1 from its value. If c is not between 0x21 and 0x7F it is returned unchanged */
    if ((c <= 0x7F) && (c >= 0x21))
        return c - 0x01;
    else
        return c;
}

char xprt(char c)
{
// Print  in hex format

    if ((c >= 0x20) && (c <= 0x7E))
    {
        printf("%x ", c);
    }
    else
        printf(".\n");
    return c;
}

char dprt(char c)
{
// Print  in dec format

    printf("%d\n", c);

    return c;
}

typedef struct fun_desc
{

    char *name;

    char (*fun)(char);

} fundesc;

int main()
{
    char c[100];
    char *carray = (char *)malloc(SIZED_ARRAY * sizeof(char));
    if (!carray)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    fundesc menu[] = {
        {"Get string", &my_get},
        {"Print char", &cprt}, 
        {"Encrypt", &encrypt},
        {"Decrypt", &decrypt},
        {"Print hex", &xprt},
        {"Print dec", &dprt},


        {NULL, NULL}}; 
    int intInput = 0;
 
    while (true)
    {
        printf("Select operation from the following menu (ctrl^D for exit):\n");
        for (int i = 0; menu[i].fun != NULL; i++)
        {
            printf("%d) %s\n", i, menu[i].name);
        }

        printf("Option: ");
        if (fgets(c, sizeof(c), stdin) != NULL)
        {
            intInput = atoi(c);

            if (intInput >= 0 && intInput <= 5)
            {
                printf("Within Bounds\n");

                char *newArray = map(carray, SIZED_ARRAY, menu[intInput].fun);


                printf("\n");

                free(carray); 
                carray = newArray; 
                printf("DONE.\n\n");
            }
            else

            {
                printf("Option out of bounds. Exiting.\n");
                free(carray);

                exit(1);
            }
        }
        else
        {

            printf("\nExiting...\n");

            free(carray);
            exit(0);
        }
    }
}
