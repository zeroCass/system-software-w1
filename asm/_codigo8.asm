;programa que testa utilizacao correta da macro,
;assim como a chama de macro dentro de macros

SECTION TEXT
TROCA: MACRO &Arg1, &B, &C
COPY &Arg1, &C
COPY &B, &arg1
COPY &C,&B
endmacro

out: macro &valor
output &valor
endmacro
soma: macro &valor
load y
ADD &valor
STORE y
out &valor
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
soma y
JMPP MOD_B
SOMA val
troca x, y,z
L1: STOP
SECTION DATA
VAL: CONST -0xB END