/*-
 * main.c
 * Minishell C source
 * Shows how to use "obtain_order" input interface function.
 *
 * Copyright (c) 1993-2002-2019, Francisco Rosales <frosal@fi.upm.es>
 * Todos los derechos reservados.
 *
 * Publicado bajo Licencia de Proyecto Educativo Práctico
 * <http://laurel.datsi.fi.upm.es/~ssoo/LICENCIA/LPEP>
 *
 * Queda prohibida la difusión total o parcial por cualquier
 * medio del material entregado al alumno para la realización
 * de este proyecto o de cualquier material derivado de este,
 * incluyendo la solución particular que desarrolle el alumno.
 *
 * DO NOT MODIFY ANYTHING OVER THIS LINE
 * THIS FILE IS TO BE MODIFIED
 */

#include <stddef.h>			/* NULL */
#include <stdio.h>			/* setbuf, printf */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>


extern int obtain_order(); /* See parser.y for description */	

int main(void)
{
	char ***argvv = NULL;
	int argvc;
	char **argv = NULL;
	int argc;
	char *filev[3] = { NULL, NULL, NULL };
	int bg;
	int ret;
	int argvcC;
	pid_t pid, bgpid;
	struct sigaction act;
	int status;
	int p[2];

		

	setbuf(stdout, NULL);			/* Unbuffered */
	setbuf(stdin, NULL);

	act.sa_handler = SIG_IGN;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);

	
	while (1) {
		fprintf(stderr, "%s", "msh> ");	/* Prompt */
		ret = obtain_order(&argvv, filev, &bg);
		if (ret == 0) break;		/* EOF */
		if (ret == -1) continue;	/* Syntax error */
		argvc = ret - 1;		/* Line */
		if (argvc == 0) continue;	/* Empty line */
		
		int fdin, fdout, fderr;

		argvcC=argvc;

		fdin = dup(0);
		// Redireccion entrada estandar <
		if(filev[0]!=NULL){
			int fd0 = open(filev[0], O_RDONLY);
			if(fd0 == -1){
				perror("open");
				close(fdin);
				continue;
			}
			dup2(fd0,0);
			close(fd0);
		}

		// Redireccion salida estandar >
		if(filev[1]!=NULL){
			int fd1 = open(filev[1], O_CREAT | O_TRUNC | O_WRONLY, 0666);
			if(fd1 == -1){
				perror("open");
				continue;
			}
			fdout=dup(1);
			dup2(fd1,1);
			close(fd1);
		}

		// Redireccion salida error >&
		if(filev[2]!=NULL){
			int fd2 = open(filev[2], O_CREAT | O_TRUNC | O_WRONLY, 0666);
			if(fd2 == -1){
				perror("open");
				continue;
			}
			fderr=dup(2);
			dup2(fd2,2);
			close(fd2);
		}

		for(argvc=0; argv=argvv[argvc]; argvc++){
			if(strcmp(argv[0], "cd") == 0){
				if(!bg && argvc == argvcC -1){
					micd(argv);
					break;
				}
			}
			// Para todo mandato no ultimo creo un pipe (Iterativa a)
			if(argvc != argvcC-1){
				pipe(p);
			}
			// El hijo hereda los fds del padre (hereda pipe).
			pid=fork();
			switch(pid){
				case -1:
					perror("fork");
					return -1;
				case 0:
				// Codigo hijo
					if(!bg){
						// Tratamiento de señales en caso de que no se encuentre en bg
						struct sigaction act;
						act.sa_handler = SIG_DFL;
						sigaction(SIGINT, &act, NULL);
						sigaction(SIGQUIT, &act, NULL);
					}
					if(argvc != argvcC - 1){
						close(p[0]);
						dup2(p[1], 1);
						close(p[1]);
					}
					if(strcmp(argv[0], "cd") == 0){
						if(bg || argvc > 1 && argvc < argvcC-1){
							micd(argv);
							break;
						}
					}
					else{
						execvp(argv[0], argv);
						perror("execvp");
					}
					
				default:
					// Codigo padre
					if(argvc != argvcC - 1){
						close(p[1]);
						dup2(p[0], 0);
						close(p[0]);
					}
					if(bg){
						bgpid=pid;
						printf("[%d]\n", bgpid);
					}
					else{
						if(argvc == argvcC-1){
							waitpid(pid, &status, 0);
						}
					}	
			}
		}

		// Restaura entrada estandar
		dup2(fdin, 0);
		close(fdin);

		// Restaura salida estandar
		if(filev[1]!=NULL){
			dup2(fdout, 1);
			close(fdout);
		}	

		// Restaura salida error
		if(filev[2]!=NULL){
			dup2(fderr, 2);
			close(fderr);
		}

	}
	exit(0);
	return 0;
}


// Funcion auxiliar mandato CD
void micd(char **argv){
		char cwd[4096];

		if(argv[1] == NULL){
			if(chdir(getenv("HOME")) == -1){
				perror("chdir");
			}else{
				printf("%s\n", getcwd(cwd, 4096));
			}
		}
		else if(argv[2] == NULL){
			if(chdir(argv[1]) == -1){
				perror("chdir");
			}else{
				printf("%s\n", getcwd(cwd, 4096));
			}
		}
		else{
			printf("Numero de argumentos incorrecto");								
		}
}
