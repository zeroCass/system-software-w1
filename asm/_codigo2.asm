;programa que testa quebra de linha entre operacoes e diretivas,
;o que segunda a especificacao eh permitido. Tambem ha a testagem de instrucoes case insenstivity

;comm
SECTION TEXT
L1: INPUT 
OLD_DATA ;comm
LOAD 
OLD_DATA DIV
DOIS
L2: 
stop
;l2: stop
SECTION DATA
DOIS: 
const

2
OLD_DATA: 
SPACE
NEW_DATA: SPACE ;comma
TMP_DATA: SPACE
