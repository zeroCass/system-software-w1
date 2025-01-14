;programa que testa há um label invalida
;output esperado: erro de label invalido e/ou arquivo
;obj com posicao de mem com valor -1


SECTION TEXT
L1: INPUT OLD_DATA
LOAD NON_DATA
DIV DOIS
L2: STOP
SECTION DATA
DOIS: CONST 2
OLD_DATA: SPACE
NEW_DATA: SPACE
TMP_DATA: SPACE
