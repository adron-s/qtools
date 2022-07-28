@
@  Патч команды 11 для загрузчиков, работающих в режиме ARM
@
.syntax unified

pkt_data_len_off=8

.ORG    0xXXXXXXXX

leavecmd:

.ORG    0xXXXXXXXX

cmd_11_exec:
		PUSH    {R4,LR}
		LDR     R1, reply_buf_ptr               @ Адрес ответнго буфера
		ADD     R0, R0,	#8                      @ R0 теперь показывает на байт +4 от начал
                                                @ командного буфера
		MCR     p15, 0, R1,c7,c5,0              @ сброс I-кеша
		BLX     R0                              @ передаем туда управление
		LDR     R0, escape_state_ptr            @  Структура данных командного обработчика
		STRH    R4, [R0,#pkt_data_len_off]      @ сохраняем размер ответного буфера
		B       leavecmd

@ блок идентификации

        .word           0xdeadbeef          @ сигнатура
        .byte           X                   @ код чипсета

.ORG    0xXXXXXXXX

reply_buf_ptr:    .word  0
escape_state_ptr: .word  0

