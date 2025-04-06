/*

    simple_harness: compile.c

    Copyright (C) 2025  Johnathan K Burchill

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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
    char *name = "simple_harness";
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
gcc simple_harness.c -o simple_harness\
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


