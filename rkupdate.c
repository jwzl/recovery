#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include "common.h"
#include "install.h"
#include "bootloader.h"

#define URL_MAX_LENGTH 512
extern bool bSDBootUpdate;
static int start_main (const char *binary, char *args[], int* pipefd) {
    pid_t pid = fork();
    if(pid == 0){
        close(pipefd[0]);
        execv(binary, args);
        printf("E:Can't run %s (%s)\n", binary, strerror(errno));
        fprintf(stdout, "E:Can't run %s (%s)\n", binary, strerror(errno));
        _exit(-1);
    }
    close(pipefd[1]);

    char buffer[1024];
    FILE* from_child = fdopen(pipefd[0], "r");
    while (fgets(buffer, sizeof(buffer), from_child) != NULL) {	
        char* command = strtok(buffer, "=");
        if (command == NULL) {
            continue;
        } else if (strcmp(command, "progress") == 0) {
			char *tmp = strtok(NULL, "=");	
			char* fraction_s = strtok(tmp, " ");
            char* seconds_s = strtok(NULL, " ");

            float fraction = (float)strtol(fraction_s, NULL);
            int seconds = strtol(seconds_s, NULL, 10);
            ui_show_progress(fraction/100, seconds);
        } else if (strcmp(command, "set_progress") == 0) {
            char* fraction_s = strtok(NULL, "=");
            float fraction = (float)strtol(fraction_s, NULL);

            ui_set_progress(fraction/100);
        } else if (strcmp(command, "ui_print") == 0) {
            char* str = strtok(NULL, "=");
            if (str) {
                printf("ui_print = %s.\n", str);
                ui_print("%s", str);
            } else {
                ui_print("\n");
            }
        } else {
            LOGE("unknown command [%s]\n", command);
        }
    }

    fclose(from_child);

    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        LOGE("Error in %s\n(Status %d)\n", binary, WEXITSTATUS(status));
        return INSTALL_ERROR;
    }
    return INSTALL_SUCCESS;

}
int do_rk_updateEngine(const char *binary, const char *path) {
    printf("start with main.\n");
    char *update="--update";
    char *update_sdboot="--update=sdboot";
    int pipefd[2];
    pipe(pipefd);

    //updateEngine --update --image_url=path --partition=0x3a00
    char **args = malloc(sizeof(char*) * 6);
    args[0] = (char* )binary;
    args[1] = (char* )malloc(20);
    sprintf(args[1], "--pipefd=%d", pipefd[1]);
    args[2] = (char* )malloc(URL_MAX_LENGTH);
    sprintf(args[2], "--image_url=%s", path);
    if (bSDBootUpdate) {
        args[3] = (char *)update_sdboot;
    } else {
        args[3] = (char *)update;
    }

    args[4] = (char *)malloc(18);
    args[5] = NULL;

    if (bSDBootUpdate) {
        sprintf(args[4], "--partition=0x%s", "FF80");
    } else {
        struct bootloader_message boot;
        get_bootloader_message(&boot);
        sprintf(args[4], "--partition=0x%X", *((int *)(boot.needupdate)));
    }

    return start_main(binary, args, pipefd);

}
int do_rk_update(const char *binary, const char *path) {
    printf("start with main.\n");
    int pipefd[2];
    pipe(pipefd);

    char** args = malloc(sizeof(char*) * 6);
    args[0] = (char* )binary;
    args[1] = "Version 1.0";
    args[2] = (char*)malloc(10);
    sprintf(args[2], "%d", pipefd[1]);
    args[3] = (char*)path;
    args[4] = (char*)malloc(8);
    sprintf(args[4], "%d", (int)bSDBootUpdate);
    args[5] = NULL;
    return start_main(binary, args, pipefd);
#if 0
    pid_t pid = fork();
    if(pid == 0){
        close(pipefd[0]);
        execv(binary, args);
        printf("E:Can't run %s (%s)\n", binary, strerror(errno));
        fprintf(stdout, "E:Can't run %s (%s)\n", binary, strerror(errno));
        _exit(-1);
    }
    close(pipefd[1]);

    char buffer[1024];
    FILE* from_child = fdopen(pipefd[0], "r");
    while (fgets(buffer, sizeof(buffer), from_child) != NULL) {
        char* command = strtok(buffer, " \n");
        if (command == NULL) {
            continue;
        } else if (strcmp(command, "progress") == 0) {
            char* fraction_s = strtok(NULL, " \n");
            char* seconds_s = strtok(NULL, " \n");

            float fraction = strtof(fraction_s, NULL);
            int seconds = strtol(seconds_s, NULL, 10);

            ui_show_progress(fraction * (1-VERIFICATION_PROGRESS_FRACTION), seconds);
        } else if (strcmp(command, "set_progress") == 0) {
            char* fraction_s = strtok(NULL, " \n");
            float fraction = strtof(fraction_s, NULL);
            ui_set_progress(fraction);
        } else if (strcmp(command, "ui_print") == 0) {
            char* str = strtok(NULL, "\n");
            if (str) {
                printf("ui_print = %s.\n", str);
                ui_print("%s", str);
            } else {
                ui_print("\n");
            }
        } else {
            LOGE("unknown command [%s]\n", command);
        }
    }

    fclose(from_child);

    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        LOGE("Error in %s\n(Status %d)\n", path, WEXITSTATUS(status));
        return INSTALL_ERROR;
    }
    return INSTALL_SUCCESS;
#endif
}
