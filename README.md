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
| **`main.c`** | Contiene la implementación principal del minishell. |
| **`parser.y`** | Gramática Yacc que analiza la línea de órdenes proporcionados por la práctica. |
| **`scanner.l`** | Analizador léxico con Flex: separa tokens y operadores. |
| **`Makefile`** | Archivo de compilación automática (`make`). |
| **`autores.txt`** | Créditos: **Francisco Rosales** (base del parser, scanner y makefile) y **Alfonso Marín** (main.c). |
| **`README.md` / `memoria-minishell.pdf`** | Documentación, resultados del proyecto e instrucciones para ejecutar. |

---

## ⚙️ Tecnologías y herramientas
- **Lenguaje:** C (C99)  
- **Entorno:** Unix/Linux (servidor Triqui, ETSIINF-UPM)  
- **Compilador:** GCC + Makefile  
- **Principales llamadas al sistema:** `fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `open`, `creat`, `chdir`, `umask`, `times`, `signal`  
- **Bibliotecas:** `stdio.h`, `unistd.h`, `sys/types.h`, `sys/wait.h`, `fcntl.h`, `pwd.h`, `glob.h`  
- **Herramientas de depuración:** `gdb`, `valgrind`  
