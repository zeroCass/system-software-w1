# Trabalho 1 de Software Básico (System Software)

## Montador de Assembly Simplificado

O objetivo desse trabalho era fazer um montador para um assembly inventado visto em aulas na disciplina. A especificação do trabalho feita pelo professor encontra-se em `docs/especificacao.pdf`.

### Como rodar o código:
Para compilar o código é necessário executar este comando:
```bash
g++ montador.cpp -std=c++17 -o montador
```

Para rodar basta executar o comando:
```bash
./montador <arquivo.asm> | <arquivo.pre>
```

### Considerações Importantes a respeito do código e do montador:
- **Compilação:**
  - O comando para compilar é `g++ montador.cpp -DTHROW_ERRORS=1 -std=c++17 -o montador`. O argumento `-DTHROW_ERRORS=1` habilita o modo que finaliza o programa ao identificar um erro. Foi recomendado pelo professor que os erros fossem exibidos no output ao invés de finalizar o programa, portanto por default, -DTHROW_ERROS=0.
  - Ao rodar o programa com `-DTHROW_ERRORS=0`, se uma LABEL não for identificada, o programa continuará sua execução, o que pode gerar erros como acesso a dados indefinidos. Por exemplo, se uma LABEL for indefinida, a posição de memória no arquivo `.obj` será possivelmente `-1`.

- **Pré-processamento:**
  - Ao fornecer um arquivo `.pre` como input, o código **não** realizará novamente o pré-processamento. Isso significa que não serão removidos espaços, tabulações desnecessárias ou comentários. O arquivo `.pre` deve estar corretamente formatado (em UPPERCASE e sem comentários ou espaços inúteis).

- **Otimização:**
  - O código não foi construído com foco primário em otimização, portanto podem existir trechos desnecessários ou mal otimizados.

- **Diretiva BEGIN:**
  - A diretiva `BEGIN` não pode ser uma LABEL (e.g., `BEGIN: MOD_B`). Ela **sempre** será interpretada como uma diretiva e deve estar no formato: `MOD_B: BEGIN`.
  - Esta escolha se deve à ideia de que, se `MOD_B` for usado em outro módulo (`PUBLIC MOD_B`), ele é uma LABEL. Usar a declaração `BEGIN: MOD_B` faz com que `MOD_B` não seja tratado como LABEL, o que pode levar a erros de montagem (veja os arquivos `_codigo5.asm` e `_codigo6.asm` como exemplos).

- **Operações aritméticas:**
  - Não é permitido que operações de aritmética (e.g., soma de ponteiros) sejam quebradas por linha, como:
    ```
    Y
    + 2
    ```
  - Apenas operações na mesma linha são permitidas, como `Y+2`.

- **Comentários:**
  - Comentários entre operações, labels, etc., não são permitidos. Assim que um comentário é identificado, aquela parte do código é desconsiderada.

- **Compatibilidade:**
  - Por enquanto, o programa não funciona no Windows devido ao uso de verificações de arquivos/diretórios como `./asm`, `./pre` e `./obj`.

- **Estrutura do arquivo:**
  - Os arquivos `.asm/.pre` precisam obrigatoriamente conter `SECTION TEXT` e `SECTION DATA`.
  - O programa assume que LABELS declaradas como variáveis (e.g., `X: SPACE`) estão na `SECTION DATA` e que instruções estão na `SECTION TEXT`, pois isso não foi explicitamente verificado no código.

- **Módulos:**
  - Caso a entrada seja um arquivo módulo (com diretivas `BEGIN/END`), ambas devem estar presentes obrigatoriamente.
  - Se um arquivo módulo não possuir `BEGIN/END`, ele será tratado como um arquivo comum e gerará a saída correspondente.

- **Saída:**
  - A saída para arquivos `.asm` será sempre um arquivo `.pre` com todos os caracteres convertidos para UPPERCASE, inclusive números hexadecimais (e.g., `0xA` será convertido para `0XA`).
  - Não há distinção entre `0xA` e `0XA` durante o processamento.

- **Hexadecimais:**
  - O programa interpreta números hexadecimais como sendo de 16 bits em complemento de 2 e os converte para inteiros durante a montagem (segunda passagem).

- **SPACE e limites de memória:**
  - O programa não verifica **Out-of-Bounds/Segmentation Fault** para operações de soma de ponteiros com `SPACE` (e.g., `VAL: SPACE 3`, `VAL + 3` pode resultar em transposição de memória, ultrapassando os limites permitidos).

