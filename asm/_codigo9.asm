;programa para testar aritimetica de ponteiros, modulos
; e constantes com hexadecimais

SECTION TEXT
MOD_A: BEGIN
Y: EXTERN
MOD_B: EXTERN
PUBLIC VAL
PUBLIC L1
INPUT Y
LOAD VAL
ADD Y
STORE Y+2 ;isso aqui eh uma arimetica de ponteiros
JMPP MOD_B
L1: STOP
SECTION DATA
;declacao de constante negativa
VAL: CONST -0xB
END