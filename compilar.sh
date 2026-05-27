#!/bin/bash
echo "==================================================="
echo "  COMPILANDO O JOGO DOS PATOS (ALERRANDRO, JOÃO, KAUALY)"
echo "==================================================="

# Usando $(...) que evita o erro de excesso de argumentos no Linux
gcc main.c -o jogo $(pkg-config --cflags --libs allegro-5 allegro_image-5)

# Verifica se o comando anterior deu certo
if [ $? -eq 0 ]; then
    echo -e "\n[SUCESSO] Jogo compilado com sucesso! Abrindo..."
    echo "==================================================="
    ./jogo
else
    echo -e "\n[ERRO] Falha na compilação."
    echo "Verifique se o seu arquivo de código se chama exatamente 'main.c'"
    echo "E se você tem os pacotes de desenvolvimento do Allegro instalados (liballegro5-dev)."
fi