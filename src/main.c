#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
Simple command-line skeleton.
Usage examples:
    ./clisuite help
    ./clisuite todo add "task name"
    ./clisuite note list
*/

void print_help(void) {
    printf("Command-Line Productivity Suite\n");
    printf("Usage:\n");
    printf("  clisuite todo [add|list|done]\n");
    printf("  clisuite note [new|list|view]\n");
    printf("  clisuite git  [status|push|log]\n");
    printf("  clisuite help\n");
}

void handle_todo(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: clisuite todo [add|list|done]\n");
        return;
    }
    if (strcmp(argv[2], "add") == 0) {
        printf("TODO: add new todo → '%s'\n", (argc > 3) ? argv[3] : "(no text)");
    } else if (strcmp(argv[2], "list") == 0) {
        printf("TODO: list todos\n");
    } else if (strcmp(argv[2], "done") == 0) {
        printf("TODO: mark item done\n");
    } else {
        printf("Unknown todo command.\n");
    }
}

void handle_note(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: clisuite note [new|list|view]\n");
        return;
    }
    if (strcmp(argv[2], "new") == 0) {
        printf("NOTE: create new note\n");
    } else if (strcmp(argv[2], "list") == 0) {
        printf("NOTE: list notes\n");
    } else if (strcmp(argv[2], "view") == 0) {
        printf("NOTE: view specific note\n");
    } else {
        printf("Unknown note command.\n");
    }
}

void handle_git(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: clisuite git [status|push|log]\n");
        return;
    }
    if (strcmp(argv[2], "status") == 0) {
        printf("GIT: show repo status\n");
    } else if (strcmp(argv[2], "push") == 0) {
        printf("GIT: auto add/commit/push\n");
    } else if (strcmp(argv[2], "log") == 0) {
        printf("GIT: show commit log\n");
    } else {
        printf("Unknown git command.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "help") == 0) {
        print_help();
    } else if (strcmp(argv[1], "todo") == 0) {
        handle_todo(argc, argv);
    } else if (strcmp(argv[1], "note") == 0) {
        handle_note(argc, argv);
    } else if (strcmp(argv[1], "git") == 0) {
        handle_git(argc, argv);
    } else {
        printf("Unknown command. Type 'clisuite help'.\n");
    }

    return 0;
}
