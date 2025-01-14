;programa: verificar eliminacao de comentarios e espacos desnecessarios
;alem disso, verifica simbolos indefinidos
;output esperado para .obj: erro simbolo indefinidos (-1 em espacos de mem)

SECTION DATA
SECTION TEXT
load N1;comentario extenso para caramba
;outro comentário STORE     VAR   ;comentario random
;COMENTARIO INCRIVEL E extenso
INPUt   n2 ;comentario extenso para caramba
;outro comentário OUTPUT VAR
;comentario1 COPY N1, N2


;comentario muito grande mesmo COPY VAR, N1 ;comentario

COPY VAR,   N1 ;comentario muito grande mesmo
EXIT: 
OUTPUT N1