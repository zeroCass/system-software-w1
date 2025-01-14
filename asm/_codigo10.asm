;programa que testa a quantidade de operandos
;para uma instrucao
;output esperado: erro de quantidade de operandos invalidos


SECTION TEXT
L1: INPUT OLD_DATA
LOAD NON_DATA
DIV
L2: STOP
SECTION DATA
DOIS: CONST
OLD_DATA: SPACE
NEW_DATA: SPACE
TMP_DATA: SPACE