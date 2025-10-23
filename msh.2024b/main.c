
#include <stddef.h>			/* NULL */
#include <stdio.h>			/* setbuf, printf */
#include <stdlib.h>
#include <unistd.h>			/* exit */
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <glob.h>




// Sustituye en argvv todos los $VAR por sus valores de entorno
void expand_vars(char ***argvv) {
    for (int i = 0; argvv[i]; i++) {
        for (int j = 0; argvv[i][j]; j++) {
            char *arg = argvv[i][j];
            if (arg[0] == '$') {
                const char *val = getenv(arg + 1); // Saltamos el $
                if (val) {
                    argvv[i][j] = strdup(val);
                } else {
                    argvv[i][j] = strdup(""); // Si no existe, dejamos cadena vacía
                }
                free(arg);
            }
        }
    }
}

void expand_tildes(char ***argvv) {
    for (int i = 0; argvv[i]; i++) {
        for (int j = 0; argvv[i][j]; j++) {
            char *arg = argvv[i][j];
            if (arg[0] == '~') {
                char *replacement = NULL;

                if (arg[1] == '\0') {
                    // Caso: ~ solo → $HOME
                    replacement = getenv("HOME");
                } else {
                    // Caso: ~usuario
                    struct passwd *pwd = getpwnam(arg + 1);
                    if (pwd) {
                        replacement = pwd->pw_dir;
                    }
                }

                if (replacement) {
                    argvv[i][j] = strdup(replacement);
                    free(arg);
                }
            }
        }
    }
}

void expand_wildcards(char ****argvvp) {
    char ***argvv = *argvvp;
    char ***new_argvv = NULL;

    for (int i = 0; argvv[i]; i++) {
        char **argv = argvv[i];
        char **new_argv = NULL;
        int new_argc = 0;

        for (int j = 0; argv[j]; j++) {
            char *arg = argv[j];

            if (strchr(arg, '?') && !strchr(arg, '/')) {
                glob_t globbuf;
                if (glob(arg, 0, NULL, &globbuf) == 0 && globbuf.gl_pathc > 0) {
                    for (size_t k = 0; k < globbuf.gl_pathc; k++) {
                        new_argv = realloc(new_argv, sizeof(char *) * (new_argc + 2));
                        new_argv[new_argc++] = strdup(globbuf.gl_pathv[k]);
                        new_argv[new_argc] = NULL;
                    }
                    globfree(&globbuf);
                    continue; // Saltar añadir arg original
                }
                globfree(&globbuf);
            }

            // Copiar tal cual
            new_argv = realloc(new_argv, sizeof(char *) * (new_argc + 2));
            new_argv[new_argc++] = strdup(arg); // 👈 usar strdup SIEMPRE
            new_argv[new_argc] = NULL;
        }

        new_argvv = realloc(new_argvv, sizeof(char **) * (i + 2));
        new_argvv[i] = new_argv;
        new_argvv[i + 1] = NULL;
    }

    *argvvp = new_argvv;
}





extern int obtain_order();		/* See parser.y for description */

int main(void)
{
	char ***argvv = NULL;
	int argvc;
	char **argv = NULL;
	int argc;
	char *filev[3] = { NULL, NULL, NULL };
	int bg;
	int ret;

	setbuf(stdout, NULL);			/* Unbuffered */
	setbuf(stdin, NULL);

    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    char mypid_str[64];
    snprintf(mypid_str, sizeof(mypid_str), "mypid=%d", getpid());
    putenv(strdup(mypid_str));


	while (1) {
		const char *prompt = getenv("prompt");
        fprintf(stderr, "%s", prompt ? prompt : "msh> ");

		ret = obtain_order(&argvv, filev, &bg);
		if (ret == 0) break;		/* EOF */
		if (ret == -1) continue;	/* Syntax error */
		argvc = ret - 1;		/* Line */

        
 		if (argvc < 1) continue; // Nada que ejecutar

        argv = argvv[0];

        // Comando nulo ":"
        if (strcmp(argv[0], ":") == 0) {
            continue;
        }


        expand_vars(argvv); // Expande los $VAR antes de ejecutar nada
        expand_tildes(argvv);
        expand_wildcards(&argvv);


        // Comando interno: umask
        if (strcmp(argv[0], "umask") == 0) {
            int saved_out = dup(STDOUT_FILENO);
            int saved_err = dup(STDERR_FILENO);

            if (filev[1]) {
                int fd_out = creat(filev[1], 0666);
                if (fd_out < 0) {
                    perror("umask: redir >");
                    continue;
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            if (filev[2]) {
                int fd_err = creat(filev[2], 0666);
                if (fd_err < 0) {
                    perror("umask: redir >&");
                    continue;
                }
                dup2(fd_err, STDERR_FILENO);
                close(fd_err);
            }

            mode_t old_mask = umask(0); // Obtener la actual
            umask(old_mask);            // Restaurar

            if (argv[1] && argv[2]) {
                fprintf(stderr, "umask: too many arguments\n");
            } else if (argv[1]) {
                char *endptr;
                unsigned int new_mask = strtol(argv[1], &endptr, 8);
                if (*endptr != '\0' || new_mask > 0777) {
                    fprintf(stderr, "umask: invalid value\n");
                } else {
                    umask((mode_t)new_mask);
                    printf("%03o\n", new_mask);
                }
            } else {
                printf("%03o\n", old_mask);
            }

            fflush(stdout);
            fflush(stderr);
            dup2(saved_out, STDOUT_FILENO);
            dup2(saved_err, STDERR_FILENO);
            close(saved_out);
            close(saved_err);
            continue;
        }



        // Comando interno: read
        if (strcmp(argv[0], "read") == 0) {
            int saved_out = dup(STDOUT_FILENO);
            int saved_err = dup(STDERR_FILENO);

            if (filev[1]) {
                int fd_out = creat(filev[1], 0666);
                if (fd_out < 0) {
                    perror("read: redir >");
                    continue;
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            if (filev[2]) {
                int fd_err = creat(filev[2], 0666);
                if (fd_err < 0) {
                    perror("read: redir >&");
                    continue;
                }
                dup2(fd_err, STDERR_FILENO);
                close(fd_err);
            }

            if (!argv[1]) {
                fprintf(stderr, "read: missing variable name(s)\n");
            } else {
                char *vars[64];
                int nvars = 0;
                for (int i = 1; argv[i] && nvars < 64; i++) {
                    vars[nvars++] = argv[i];
                }

                char line[1024];
                if (!fgets(line, sizeof(line), stdin)) {
                    perror("read: fgets");
                } else {
                    line[strcspn(line, "\n")] = 0; // quitar salto
                    char *token = strtok(line, " \t");
                    int i = 0;

                    while (token && i < nvars - 1) {
                        char buffer[1024];
                        snprintf(buffer, sizeof(buffer), "%s=%s", vars[i], token);
                        putenv(strdup(buffer));
                        token = strtok(NULL, " \t");
                        i++;
                    }

                    if (i < nvars && token) {
                        char *rest = token;
                        token = strtok(NULL, "");
                        if (token) {
                            strcat(rest, " ");
                            strcat(rest, token);
                        }
                        char buffer[1024];
                        snprintf(buffer, sizeof(buffer), "%s=%s", vars[i], rest);
                        putenv(strdup(buffer));
                    }
                }
            }

            fflush(stdout);
            fflush(stderr);
            dup2(saved_out, STDOUT_FILENO);
            dup2(saved_err, STDERR_FILENO);
            close(saved_out);
            close(saved_err);
            continue;
        }



        // Comando interno: time
        if (strcmp(argv[0], "time") == 0) {
            int saved_out = dup(STDOUT_FILENO);
            int saved_err = dup(STDERR_FILENO);

            if (filev[1]) {
                int fd_out = creat(filev[1], 0666);
                if (fd_out < 0) {
                    perror("time: redir >");
                    continue;
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            if (filev[2]) {
                int fd_err = creat(filev[2], 0666);
                if (fd_err < 0) {
                    perror("time: redir >&");
                    continue;
                }
                dup2(fd_err, STDERR_FILENO);
                close(fd_err);
            }

            struct tms t_start, t_end;
            clock_t r_start, r_end;
            long ticks = sysconf(_SC_CLK_TCK);

            if (!argv[1]) {
                r_start = times(&t_start);
                r_end = times(&t_end);

                int u = (int)((t_end.tms_utime + t_end.tms_cutime) * 1000 / ticks);
                int s = (int)((t_end.tms_stime + t_end.tms_cstime) * 1000 / ticks);
                int r = (int)((r_end - r_start) * 1000 / ticks);

                printf("%d.%03du %d.%03ds %d.%03dr\n", u/1000, u%1000, s/1000, s%1000, r/1000, r%1000);
            } else {
                r_start = times(&t_start);
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                } else if (pid == 0) {
                    execvp(argv[1], &argv[1]);
                    perror("execvp");
                    exit(1);
                } else {
                    int status;
                    waitpid(pid, &status, 0);
                    r_end = times(&t_end);

                    int u = (int)((t_end.tms_cutime) * 1000 / ticks);
                    int s = (int)((t_end.tms_cstime) * 1000 / ticks);
                    int r = (int)((r_end - r_start) * 1000 / ticks);

                    printf("%d.%03du %d.%03ds %d.%03dr\n", u/1000, u%1000, s/1000, s%1000, r/1000, r%1000);
                }
            }

            fflush(stdout);
            fflush(stderr);
            dup2(saved_out, STDOUT_FILENO);
            dup2(saved_err, STDERR_FILENO);
            close(saved_out);
            close(saved_err);
            continue;
        }






        if (strcmp(argv[0], "cd") == 0) {
            int saved_out = dup(STDOUT_FILENO);
            int saved_err = dup(STDERR_FILENO);

            if (filev[1]) {
                int fd_out = creat(filev[1], 0666);
                if (fd_out < 0) {
                    perror("cd: redir >");
                    continue;
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            if (filev[2]) {
                int fd_err = creat(filev[2], 0666);
                if (fd_err < 0) {
                    perror("cd: redir >&");
                    continue;
                }
                dup2(fd_err, STDERR_FILENO);
                close(fd_err);
            }

            if (argvc > 1 || argvv[1]) {
                fprintf(stderr, "cd: too many arguments\n");
            } else {
                const char *path = argv[1] ? argv[1] : getenv("HOME");
                if (!path) path = "/";
                if (chdir(path) < 0) {
                    perror("cd");
                } else {
                    char cwd[1024];
                    if (getcwd(cwd, sizeof(cwd))) {
                        printf("%s\n", cwd);
                    }
                }
            }

            fflush(stdout);
            fflush(stderr);
            dup2(saved_out, STDOUT_FILENO);
            dup2(saved_err, STDERR_FILENO);
            close(saved_out);
            close(saved_err);
            continue;
        }




        int i;
        int pipefd[2][2]; // Alternaremos entre pipefd[0] y pipefd[1]
        pid_t pid;
        int in_fd = STDIN_FILENO; // Fd de entrada inicial (por si hay <)
        pid_t last_pid;

        for (i = 0; i < argvc; i++) {
            // Si no es el último comando, crea una nueva tubería
            if (i < argvc - 1 && pipe(pipefd[i % 2]) < 0) {
                perror("pipe");
                break;
            }

            pid = fork();
            if (pid < 0) {
                perror("fork");
                break;
            } else if (pid == 0) {
                // --- HIJO ---

                // Redirección de entrada: de pipe anterior o de archivo
                if (i == 0 && filev[0]) {
                    int fd = open(filev[0], O_RDONLY);
                    if (fd < 0) { perror("open entrada"); exit(1); }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                } else if (i > 0) {
                    dup2(pipefd[(i + 1) % 2][0], STDIN_FILENO); // leer del pipe anterior
                }

                // Redirección de salida: a pipe siguiente o archivo
                if (i == argvc - 1) {
                    if (filev[1]) {
                        int fd = creat(filev[1], 0666);
                        if (fd < 0) { perror("creat salida"); exit(1); }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                    }
                    if (filev[2]) {
                        int fd = creat(filev[2], 0666);
                        if (fd < 0) { perror("creat error"); exit(1); }
                        dup2(fd, STDERR_FILENO);
                        close(fd);
                    }
                } else {
                    dup2(pipefd[i % 2][1], STDOUT_FILENO); // escribir en el pipe
                }

                // Cerrar todos los extremos de pipe heredados
                if (i > 0) {
                    close(pipefd[(i + 1) % 2][0]);
                    close(pipefd[(i + 1) % 2][1]);
                }
                if (i < argvc - 1) {
                    close(pipefd[i % 2][0]);
                    close(pipefd[i % 2][1]);
                }

                if (!bg) {
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                }

                // Ejecutar comando
                execvp(argvv[i][0], argvv[i]);
                perror("execvp");
                exit(1);
            } else {
                // --- PADRE ---

                // Si es el último comando, guardamos su PID
                if (i == argvc - 1) {
                    last_pid = pid;
                }
                // Cerrar extremos de pipe ya usados por el hijo anterior
                if (i > 0) {
                    close(pipefd[(i + 1) % 2][0]);
                    close(pipefd[(i + 1) % 2][1]);
                }
            }
        }

        if (!bg) {
            int status;
            if (last_pid > 0) {
                waitpid(last_pid, &status, 0);  // Esperar explícitamente al último hijo

                // Guardar valor de salida del último comando
                char status_str[64];
                if (WIFEXITED(status)) {
                    snprintf(status_str, sizeof(status_str), "status=%d", WEXITSTATUS(status));
                } else {
                    snprintf(status_str, sizeof(status_str), "status=%d", 255);
                }
                putenv(strdup(status_str));
            }
        } else {
            // Mostrar PID del último hijo (que ya tienes en `pid`)
            fprintf(stderr, "[%d]\n", pid);

            // Guardar en variable de entorno bgpid
            char bgpid_str[64];
            snprintf(bgpid_str, sizeof(bgpid_str), "bgpid=%d", pid);
            putenv(strdup(bgpid_str)); // strdup porque el puntero debe ser persistente
        }
    }

    exit(0);
	return 0;
}