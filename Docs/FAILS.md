# Failure Model

## 1. Autoridade única

`fail.c` é a autoridade responsável por detectar falhas em Armito.

Essa decisão evita que Core, Decoder, ArmitoBoot e outros componentes
mantenham detectores paralelos com regras divergentes.

## 2. Separação de responsabilidades

  -----------------------------------------------------------------------
  Componente                          Responsabilidade
  ----------------------------------- -----------------------------------
  `fail.c`                            Detectar e determinar que uma falha
                                      ocorreu

  Core Diagnostics                    Receber, encaminhar e apresentar
                                      informações da falha

  `Display.c`                         Exibir a informação quando o
                                      dispositivo de saída estiver
                                      disponível

  `LogGen.c`                          Registrar a informação no mecanismo
                                      de log

  Core                                Coordenar o fluxo e o encerramento
                                      seguro quando necessário

  Subsistema de origem                Retornar estado e contexto
                                      suficientes para a autoridade de
                                      falhas
  -----------------------------------------------------------------------

Core Diagnostics não deve reimplementar a detecção nem competir com
`fail.c`.

## 3. Fluxo conceitual

``` text
Subsystem / ArmitoBoot / Decoder
              |
              v
           fail.c
              |
              v
       Failure information
              |
        +-----+-----+
        |           |
        v           v
    Display.c    LogGen.c
        ^           ^
        +-----+-----+
              |
       Core Diagnostics
       (adapter / routing)
```

O fluxo exato de chamadas deverá respeitar as interfaces reais de
`fail.c`, `Display.c` e `LogGen.c`. O diagrama representa
responsabilidades, não uma API já implementada.

## 4. Dados de uma falha

A estrutura de informação de falha deve permitir, conforme aplicável: -
identificador ou código; - severidade; - componente de origem; - etapa
do ciclo de vida; - endereço ou contexto técnico, se seguro e
disponível; - descrição curta; - estado recuperável ou fatal; - ação
tomada.

Não incluir dados não inicializados ou ponteiros cujo tempo de vida não
esteja garantido.

## 5. Severidade e resposta

A política de severidade deve distinguir pelo menos: - informativa; -
aviso; - erro recuperável; - erro fatal.

A classificação final deve pertencer ao contrato do sistema de falhas,
não ser reinventada por cada subsistema.

## 6. Falha de apresentação

A ausência de Display ou de um dispositivo gráfico não pode impedir o
registro da falha. Da mesma forma, falha no logger não deve provocar
recursão infinita no próprio sistema de falhas.

O sistema deve definir fallback seguro, por exemplo: - log serial,
quando disponível; - buffer de diagnóstico em memória previamente
reservado; - encerramento controlado, quando não houver canal confiável.

Esses fallbacks são requisitos de projeto a implementar e validar.

## 7. Recursão e falhas secundárias

O tratamento de uma falha pode falhar. O mecanismo precisa impedir: -
reentrada infinita; - alocação dinâmica não segura durante falha; -
dependência circular entre fail, Display e LogGen; - perda da falha
original por uma falha secundária.

## 8. Regras obrigatórias

1.  `fail.c` permanece como detector único.
2.  Core Diagnostics apenas instrumentaliza apresentação e
    encaminhamento.
3.  Display e LogGen não assumem detecção global.
4.  A falha original deve ser preservada.
5.  Falhas durante boot devem ter caminho mínimo de diagnóstico.
6.  Ausência de GOP não pode ser tratada automaticamente como ausência
    de qualquer saída.
7.  O sistema deve conseguir encerrar com segurança quando a recuperação
    não for possível.

## 9. Integração pendente

-   Mapear a API real de `fail.c`.
-   Mapear a API real de `Display.c`.
-   Mapear a API real de `LogGen.c`.
-   Definir estrutura comum de falha.
-   Implementar adaptador no Core Diagnostics.
-   Testar falhas antes e depois da inicialização do Core.
-   Testar ausência de display, falha de log e falha durante o próprio
    diagnóstico.

## 10. Falhas de autenticação ARM64E

Uma assinatura PAC inválida é uma falha arquitetural do estado convidado,
não uma falha global do Core. A API PAC registra a falha e invalida o
resultado; helpers de branch e return autenticados também registram uma
exceção no estado AArch64. Falhas de configuração, chave indisponível ou
provedor são retornadas como erros da API, não convertidas silenciosamente
em sucesso. Ainda não há executor que encaminhe essas exceções ao
`fail.c` ou implemente a política de trap de um runtime.
