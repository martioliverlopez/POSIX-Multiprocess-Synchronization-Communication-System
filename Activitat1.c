// --- Codi de Martí Oliver López ---
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define TAMANY_BUFFER 100

// Funció criptogàfica simple (xifratge César +3)
void xifrar_contrasenya(char *text) {
    for (int i = 0; text[i] != '\0'; i++) {
        text[i] = text[i] + 3;
    }
}

int main() {
    // Pipes per al fill 1 (p2c = parent to child, c2p = child to parent)
    int pipe1_p2c[2], pipe1_c2p[2];
    // Pipes per al Fill 2
    int pipe2_p2c[2], pipe2_c2p[2];

    // Crear les pipes
    if (pipe(pipe1_p2c) == -1 || pipe(pipe1_c2p) == -1 || 
        pipe(pipe2_p2c) == -1 || pipe(pipe2_c2p) == -1) {
        perror("Error al crear los pipes");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork(); // Primer fill 

    if (pid1 < 0) {
        perror("Error en el fork 1");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        //  Codi del primer fill 
        close(pipe1_p2c[1]); // Tanca escritura d'entrada
        close(pipe1_c2p[0]); // Tanca lectura de sortida
        // Tanca pipes del fill 2 que aquest fill no utilitza
        close(pipe2_p2c[0]); close(pipe2_p2c[1]);
        close(pipe2_c2p[0]); close(pipe2_c2p[1]);

        char buffer[TAMANY_BUFFER];
        read(pipe1_p2c[0], buffer, TAMANY_BUFFER);
        close(pipe1_p2c[0]);

        xifrar_contrasenya(buffer); 

        write(pipe1_c2p[1], buffer, strlen(buffer) + 1); // Retorna per a un 2n pipe 
        close(pipe1_c2p[1]);
        exit(EXIT_SUCCESS);
    }

    pid_t pid2 = fork(); // Segon fill 

    if (pid2 < 0) {
        perror("Error en el fork 2");
        exit(EXIT_FAILURE);
    } else if (pid2 == 0) {
        // Codi del segon fill
        close(pipe2_p2c[1]); // Tanca escritura d'entrada
        close(pipe2_c2p[0]); // Tanca lectura de sortida
        // Tanca pipes del fill 1 que aquest fill no utilitza
        close(pipe1_p2c[0]); close(pipe1_p2c[1]);
        close(pipe1_c2p[0]); close(pipe1_c2p[1]);

        char buffer[TAMANY_BUFFER];
        read(pipe2_p2c[0], buffer, TAMANY_BUFFER);
        close(pipe2_p2c[0]);

        xifrar_contrasenya(buffer);

        write(pipe2_c2p[1], buffer, strlen(buffer) + 1); // Retorna per a un 2n pipe 
        close(pipe2_c2p[1]);
        exit(EXIT_SUCCESS);
    }

    // Codi del Pare
    // Tanca extrems de lectura dels pipes d'enviament i extrems d'escritura dels pipes de recepció
    close(pipe1_p2c[0]); close(pipe1_c2p[1]);
    close(pipe2_p2c[0]); close(pipe2_c2p[1]);

    
    char pass1[] = "Marti";
    char pass2[] = "Oliver";

    // Enviar contrasenyes en text pla (sense xifratge)
    write(pipe1_p2c[1], pass1, strlen(pass1) + 1);
    write(pipe2_p2c[1], pass2, strlen(pass2) + 1);

    close(pipe1_p2c[1]);
    close(pipe2_p2c[1]);

    // Llegir contrasenyes xifrades
    char resultat1[TAMANY_BUFFER];
    char resultat2[TAMANY_BUFFER];

    read(pipe1_c2p[0], resultat1, TAMANY_BUFFER);
    read(pipe2_c2p[0], resultat2, TAMANY_BUFFER);

    close(pipe1_c2p[0]);
    close(pipe2_c2p[0]);

    // Esperar als fills per a evitar zombis
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    // Mostrar de manera conjunta 
    printf("--- Resultats del Xifratge ---\n");
    printf("Fill 1 | Original: %s -> Xifrat: %s\n", pass1, resultat1);
    printf("Fill 2 | Original: %s -> Xifrat: %s\n", pass2, resultat2);

    return 0;
}