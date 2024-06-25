gcc -pthread main.c -o main

./main t n arqA.dat arqB.dat arqC.dat arqD.dat arqE.dat

t = numero de threads
n = numero de linhas x colunas da matriz, ex: n x n sendo n = 3 -> 3 x 3

Atualmente os arquivos arq estão com matrizes de 3 x 3, então o codigo deve ser rodado com n = 3
