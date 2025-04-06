#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void compile_usage(void) 
{
    printf("usage:\n");
    printf("compile\n");
    printf("  compiles the program.\n");
    printf("compile me\n");
    printf("  compiles this command.\n");

    return;

}

int main(int argc, char **argv)
{
    char *name = "harness";
    int result = 0;
	// Run 'compile me' for example, to compile this first
	if (argc == 2) {
        if (strcmp("me", argv[1]) == 0) {
            result = system("gcc -o compile compile.c -O3");
            if (result != 0) {
                fprintf(stdout, "Unable to compile compile.c\n");
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;

        }
        else {
            compile_usage();
            return EXIT_FAILURE;
        }
    }
	//  compile the tracing program
    const char *compilerCommand = "\
MACOSX_DEPLOYMENT_TARGET=14.6 \
gcc harness.c -o harness\
 -framework CoreVideo \
 -framework IOKit \
 -framework Cocoa \
 -framework GLUT \
 -framework OpenGL \
 -lraylib \
 -O3";
// -g";
    result = system(compilerCommand);
    if (result != 0) {
        fprintf(stdout, "Unable to compile program\n");
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Compiled program '%s'\n", name);

    return EXIT_SUCCESS;
}


