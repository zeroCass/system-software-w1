;este programa testa se um instrucao eh valida
;output esperado: erro de operacao nao identificada
;ATENÇÃO: É provável que o código entre em loop infinito
;
SECTION TEXT
INPUT
OLD_DATA LOAD OLD_DATA
L1: DIV DOIS
STORE NEW_DATA MULT DOIS STORE
TMP_DATA
CARREGAR OLD_DATA
SUB TMP_DATA
STORE TMP_DATA
OUTPUT TMP_DATA
COPY NEW_DATA,OLD_DATA
LOAD OLD_DATA
JMPP L1
STOP
SECTION DATA
DOIS: CONST 2
OLD_DATA: SPACE
NEW_DATA: SPACE
TMP_DATA: SPACE