# 🐚 Minishell – Intérprete de comandos en C

## 🧩 Descripción general
Este proyecto consiste en el desarrollo de un **intérprete de comandos tipo shell** implementado en **C sobre sistemas Unix/Linux**, como parte de la asignatura **Sistemas Operativos (UPM)**.  
El programa permite ejecutar **comandos, secuencias y redirecciones**, gestionando **procesos, señales, tuberías y entornos** mediante llamadas al sistema como `fork`, `execvp`, `pipe`, `dup2`, `waitpid` o `signal`.

El objetivo principal fue **comprender el funcionamiento interno de un intérprete de comandos (shell)**, reforzando los conceptos de **gestión de procesos y comunicación entre ellos**.

---

## 🎯 Objetivos principales
- Implementar un **intérprete funcional de comandos** en lenguaje C.  
- Gestionar **procesos concurrentes** y **ejecución en background (&)**.  
- Implementar **redirecciones** (`<`, `>`, `>&`) y **tuberías** (`|`).  
- Desarrollar **comandos internos** (`cd`, `umask`, `read`, `time`).  
- Implementar **expansiones**: variables (`$VAR`), tildes (`~usuario`), comodines (`?`).  
- Manejar correctamente **señales** y **variables de entorno dinámicas**.  

---

## 🧱 Estructura del proyecto
| Archivo / Carpeta | Descripción |
|--------------------|-------------|
| **`main.c`** | Contiene el bucle principal del minishell y las llamadas a `obtain_order()` del parser. |
| **`parser.l / parser.y`** | Analizadores léxico y sintáctico (Lex/Yacc) proporcionados por la práctica. |
| **`functions.c`** | Funciones auxiliares de ejecución y manejo de redirecciones. |
| **`expand_vars.c`** | Implementación de expansión de variables, tildes y comodines. |
| **`Makefile`** | Archivo de compilación automática (`make`). |
| **`msh`** | Ejecutable resultante tras la compilación. |
| **`README.md` / `memoria-minishell.pdf`** | Documentación y resultados del proyecto. |

---

## ⚙️ Tecnologías y herramientas
- **Lenguaje:** C (C99)  
- **Entorno:** Unix/Linux (servidor Triqui, ETSIINF-UPM)  
- **Compilador:** GCC + Makefile  
- **Principales llamadas al sistema:** `fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `open`, `creat`, `chdir`, `umask`, `times`, `signal`  
- **Bibliotecas:** `stdio.h`, `unistd.h`, `sys/types.h`, `sys/wait.h`, `fcntl.h`, `pwd.h`, `glob.h`  
- **Herramientas de depuración:** `gdb`, `valgrind`  
