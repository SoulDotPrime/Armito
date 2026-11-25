# What is Armito?

## Definição

Armito é uma infraestrutura de tradução e compatibilidade entre
arquiteturas de processadores. Seu propósito é receber instruções ou
blocos de código de uma arquitetura de origem, compreender sua
semântica, convertê-los para uma representação intermediária e permitir
que componentes posteriores produzam uma forma executável na arquitetura
de destino.

A primeira direção de pesquisa estabelecida é **ARM64 Apple → x86_64**.
Essa direção inicial não limita a arquitetura final do projeto: o
desenho deve permitir outros pares de arquiteturas sem acoplar o núcleo
a um único processador.

## O problema que o Armito pretende resolver

Arquiteturas diferentes possuem conjuntos de instruções, convenções de
chamada, modelos de memória, registradores, exceções e interfaces de
sistema distintos. Executar software de uma arquitetura em outra exige
mais do que trocar códigos de operação.

Uma solução precisa tratar, conforme o escopo:

-   decodificação correta das instruções;
-   semântica e estado dos registradores;
-   flags e operações aritméticas;
-   memória, alinhamento e ordenação;
-   desvios, chamadas e retornos;
-   exceções e condições de falha;
-   convenções ABI;
-   chamadas de sistema e serviços de plataforma;
-   instruções privilegiadas e recursos específicos do hardware.

O Armito organiza essas responsabilidades em componentes independentes
para que possam ser desenvolvidas, verificadas e substituídas
separadamente.

## O que ele é

-   Um projeto de tradução entre arquiteturas.
-   Uma infraestrutura modular de compatibilidade.
-   Um sistema com decoder e representação intermediária.
-   Uma base extensível para backends de arquiteturas de destino.
-   Um ecossistema que inclui inicialização e orquestração próprias por
    meio do ArmitoBoot e do Core.

## O que ele não é

-   Não é apenas um desassemblador.
-   Não é apenas um emulador de CPU.
-   Não é, por si só, uma máquina virtual completa.
-   Não é um substituto automático de drivers, firmware, kernel ou
    serviços do sistema operacional.
-   Não deve ser descrito como compatibilidade universal já alcançada.

A tradução pode coexistir com emulação de partes do ambiente, serviços
de compatibilidade e execução nativa no destino. A escolha depende do
comportamento que precisa ser reproduzido.

## Modelo conceitual

1.  **Inicializar:** ArmitoBoot prepara o ambiente e transfere o
    controle ao Core.
2.  **Orquestrar:** Core valida e inicializa os módulos necessários.
3.  **Decodificar:** Decoder interpreta instruções em blocos ordenados.
4.  **Representar:** as instruções são convertidas para uma Universal
    IR.
5.  **Traduzir:** um tradutor/backend mapeia a IR para a arquitetura de
    destino.
6.  **Executar:** o código traduzido é executado sob os contratos de
    plataforma definidos.
7.  **Diagnosticar:** falhas são detectadas pela autoridade designada e
    apresentadas pelos mecanismos de diagnóstico.

## Direção de longo prazo

O objetivo é que Armito evolua de uma implementação inicial de tradução
ARM64 Apple → x86_64 para uma infraestrutura extensível, capaz de
suportar múltiplas arquiteturas e diferentes ambientes de execução. Isso
exige contratos estáveis entre módulos, testes semânticos e documentação
rigorosa.

## Critério de sucesso

O projeto só poderá afirmar suporte a uma combinação de arquiteturas
quando houver:

-   cobertura documentada das instruções suportadas;
-   testes de equivalência semântica;
-   tratamento definido de instruções não suportadas;
-   comportamento validado para memória, fluxo de controle e estado da
    CPU;
-   backend funcional na arquitetura de destino;
-   testes integrados no ambiente declarado como suportado.
