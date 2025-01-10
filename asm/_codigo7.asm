;prgrama que testa macros: nao eh permitido passar parametros para macros sem
;sem utilizar a sintaxe correta: &param
;output esperado: erro de passagem de parametro para macro

SECTION TEXT

TROCA: MACRO A, B, C
COPY A, C
COPY B, A
COPY C, B
endmacro

soma: macro 
load Y
ADD VAL
STORE y
ENDMACRO


MOD_A: BEGIN
Y: EXTERN
MOD_B: EXTERN
PUBLIC VAL
PUBLIC L1
INPUT Y
LOAD VAL
ADD Y
STORE Y
TROCA Y,Z, X
soma
JMPP MOD_B
SOMA
troca x, y,z
L1: STOP
SECTION DATA
VAL: CONST -0xB END