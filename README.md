# Fun-Programing-Language-Compiler
Conception d'un langage de programmation
/!\ correction faite avec l'IA Claude d'Anthropic, conception et développement initial fait à la main (moi-même en utilisant les ressources disponibles)

Objectif: Apprentissage et Développement des compétences en programmation

Etape de conception:
1. Lexer -> transformer les mots ou les phrases en tokens
2. Parser -> utiliser un arbre et générer les règles grammaticales du langage
3. code_generation -> commencer la traduction des jetons parsés en langage assembleur de votre choix
4. code_emission -> écrire le code traduit en assembleur dans un fichier (.asm ou s)

Ce repo ne contient pas pour l'instant de driver logiciel pour le compiler.

Assembleur utilisé: NASM - convention Intel x86 64-bit (RAX, RCX, RDX, RSP, RBP, RDI, RSI, ...)
Système d'exploitation fonctionnel: Linux Mint
Langage de programmation: Langage C

Commandes pour la compilation et l'exécution :
gcc fpl-c.c -o fpl-c.o
./fpl-c.o
nasm -f elf64 out.asm -o out.o
gcc out.o -o out
./out.o (il ne se passe rien car il n'y a pas de fonction d'affichage, mais le code fonctionne)

En image:
<img width="642" height="337" alt="image" src="https://github.com/user-attachments/assets/5e1a43d7-6729-47b4-ad9c-fd97e224cd42" />

Ressources:
1. Sandler, Nora. Writing a C Compiler - Build a Real Programming Language from Scratch. San Francisco, CA : No Starch Press. 2024. 792 p.
2. Recherches sur le web
