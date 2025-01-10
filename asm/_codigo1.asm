
;programa: estou assumindo que nao pode haver comentarios na declaracao de labels, 
;operandos e entre operacoes

;explicacao do programa qualquer
;SECTION DATA e SECTION TEXT
SECTION DATA
DOIS: CONST 2
OLD_DATA: SPACE ;declaracao de variavel
NEW_DATA: SPACE
TMP_DATA: SPACE

SECTION TEXT
L1:
INPUT   OLD_DATA ;


LOAD   OLD_DATA    ;comentário L1: DIV
;comentario qualquer
DIV DOIS ;ACC = OLD_DATA / 2
L2:



STOP
