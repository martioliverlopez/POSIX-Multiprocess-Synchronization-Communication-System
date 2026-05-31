// --- Codi de Martí Oliver López ---
//compatibilitat amb estàndar c11
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#define TAMANY_BUFFER 256
#define FINESTRA_TEMPS 10 // Finestra de 10 segons

pid_t pidA = -1, pidB = -1, pidC = -1;

// Manejador de senyals per al pare (Ctrl+C o finalització de A)
void handle_shutdown(int sig) {
    if (sig == SIGINT) {
        printf("\n[Pare] Ctrl+C detectat. Iniciant aturada controlada...\n");
    } else {
        printf("\n[Pare] Senyal d'apagat rebuda. Tancant sistema...\n");
    }

    // Enviar SIGTERM als fills si existeixen
    if (pidA > 0) kill(pidA, SIGTERM);
    if (pidB > 0) kill(pidB, SIGTERM);
    if (pidC > 0) kill(pidC, SIGTERM);
}

// Xifratge simple (César +3)
void xifrar_text(char *text) {
    for (int i = 0; text[i] != '\0' && text[i] != '\n'; i++) {
        text[i] = text[i] + 3;
    }
}

int main() {
    int pipe_AB[2], pipe_BC[2];

    if (pipe(pipe_AB) == -1 || pipe(pipe_BC) == -1) {
        perror("Error creant pipes");
        exit(EXIT_FAILURE);
    }

    // Configurar el manejador de SIGINT en el pare
    struct sigaction sa;
    sa.sa_handler = handle_shutdown;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // Creació del Procés C
    pidC = fork();
    if (pidC == 0) {
        close(pipe_AB[0]); close(pipe_AB[1]);
        close(pipe_BC[1]); // Tanca escriptura de B a C

        FILE *fileC = fopen("arxiu_procesC.txt", "w");
        char buffer[TAMANY_BUFFER];
        
        while (read(pipe_BC[0], buffer, TAMANY_BUFFER) > 0) {
            fprintf(fileC, "%s\n", buffer);
            fflush(fileC);
        }
        
        fclose(fileC);
        close(pipe_BC[0]);
        exit(EXIT_SUCCESS);
    }

    // Creació del Procés B
    pidB = fork();
    if (pidB == 0) {
        close(pipe_AB[1]); // Tanca escriptura de A a B
        close(pipe_BC[0]); // Tanca lectura de B a C
        
        FILE *fileB = fopen("arxiu_procesB.txt", "w");
        char buffer[TAMANY_BUFFER];
        time_t window_end = 0;

        while (read(pipe_AB[0], buffer, TAMANY_BUFFER) > 0) {
            if (strchr(buffer, '@') != NULL) {
                window_end = time(NULL) + FINESTRA_TEMPS;
                printf("[Procés B] Caràcter '@' detectat. Finestra activada per %d segons.\n", FINESTRA_TEMPS);
            }

            xifrar_text(buffer);

            if (time(NULL) <= window_end) {
                write(pipe_BC[1], buffer, strlen(buffer) + 1);
            } else {
                fprintf(fileB, "%s\n", buffer);
                fflush(fileB);
            }
        }

        fclose(fileB);
        close(pipe_AB[0]);
        close(pipe_BC[1]);
        exit(EXIT_SUCCESS);
    }

    // Creació del Procés A
    pidA = fork();
    if (pidA == 0) {
        close(pipe_BC[0]); close(pipe_BC[1]);
        close(pipe_AB[0]); // Tanca lectura de A a B
        
        char buffer[TAMANY_BUFFER];
        printf("[Procés A] Introdueix text (escriu 'SORTIR' per a finalitzar o utilitza '@' per a la finestra):\n");

        while (fgets(buffer, TAMANY_BUFFER, stdin) != NULL) {
            if (strncmp(buffer, "SORTIR", 6) == 0) {
                printf("[Procés A] Comanda de sortida detectada.\n");
                break;
            }
            write(pipe_AB[1], buffer, strlen(buffer) + 1);
        }

        close(pipe_AB[1]);
        exit(EXIT_SUCCESS);
    }

    // Codi del Pare
    close(pipe_AB[0]); close(pipe_AB[1]);
    close(pipe_BC[0]); close(pipe_BC[1]);

    // Monitoritzar si el Procés A finalitza per si sol (per escriure SORTIR)
    int status;
    while (waitpid(pidA, &status, WNOHANG) == 0) {
        usleep(100000); // Pausa breu per a no saturar la CPU
    }

    // Si sortim del bucle, A ha finalitzat. Forcem la apagada de la resta
    handle_shutdown(0);

    // Esperar a tots per a evitar zombis
    waitpid(pidA, NULL, 0);
    waitpid(pidB, NULL, 0);
    waitpid(pidC, NULL, 0);

    printf("[Pare] Sistema tancat correctament. Sense zombis ni orfes.\n");
    return 0;
}