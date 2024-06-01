#ifndef _TABLES_
#define _TABLES_


// 221215 2. 테이블 하나에 모든 액션을 다넣고 key를 설정

#define NOP(i)\
action nop##i##(){\
}\

NOP(1)
NOP(2)
NOP(3)
NOP(4)
NOP(5)
NOP(6)
NOP(7)
NOP(8)
NOP(9)
NOP(10)
NOP(11)
NOP(12)
NOP(13)
NOP(14)
NOP(15)
NOP(16)
NOP(17)
NOP(18)
NOP(19)
NOP(20)
NOP(21)
NOP(22)
NOP(23)
NOP(24)
NOP(25)
NOP(26)
NOP(27)
NOP(28)
NOP(29)
NOP(30)
NOP(31)
NOP(32)


#ifdef _MODIFY_CONT2_
#define TABLE_NEWPROCESSENTRY(i,j) \
table NewprocessEntry##i## { \
    key = { \
        ig_md.mdata.isAggregate : ternary; \
        ig_md.mdata.agtr_complete : ternary; \
        ig_md.mdata.bitmap : ternary; \
        ig_md.mdata.isMyPAS : ternary; \
    } \
    actions = { \
        entry##i##WriteToPacket; \
        processentry##i##; \
        noequ0_processentry##i##; \
        noequ0_processentry##i##andWriteToPacket; \
        nop##i##; \
    } \ 
} \


#else


#define TABLE_NEWPROCESSENTRY(i,j) \
table NewprocessEntry##i## { \
    key = { \
        ig_md.mdata.isAggregate : ternary; \
        ig_md.mdata.agtr_complete : ternary; \
        ig_md.mdata.bitmap : ternary; \
    } \
    actions = { \
        entry##i##WriteToPacket; \
        processentry##i##; \
        noequ0_processentry##i##; \
        noequ0_processentry##i##andWriteToPacket; \
        nop##i##; \
    } \ 
} \

#endif

TABLE_NEWPROCESSENTRY(1,4)
TABLE_NEWPROCESSENTRY(2,4)
TABLE_NEWPROCESSENTRY(3,4)
TABLE_NEWPROCESSENTRY(4,4)
TABLE_NEWPROCESSENTRY(5,5)
TABLE_NEWPROCESSENTRY(6,5)
TABLE_NEWPROCESSENTRY(7,5)
TABLE_NEWPROCESSENTRY(8,5)
TABLE_NEWPROCESSENTRY(9,6)
TABLE_NEWPROCESSENTRY(10,6)
TABLE_NEWPROCESSENTRY(11,6)
TABLE_NEWPROCESSENTRY(12,6)
TABLE_NEWPROCESSENTRY(13,7)
TABLE_NEWPROCESSENTRY(14,7)
TABLE_NEWPROCESSENTRY(15,7)
TABLE_NEWPROCESSENTRY(16,7)
TABLE_NEWPROCESSENTRY(17,8)
TABLE_NEWPROCESSENTRY(18,8)
TABLE_NEWPROCESSENTRY(19,8)
TABLE_NEWPROCESSENTRY(20,8)
TABLE_NEWPROCESSENTRY(21,9)
TABLE_NEWPROCESSENTRY(22,9)
TABLE_NEWPROCESSENTRY(23,9)
TABLE_NEWPROCESSENTRY(24,9)
TABLE_NEWPROCESSENTRY(25,10)
TABLE_NEWPROCESSENTRY(26,10)
TABLE_NEWPROCESSENTRY(27,10)
TABLE_NEWPROCESSENTRY(28,10)
TABLE_NEWPROCESSENTRY(29,11)
TABLE_NEWPROCESSENTRY(30,11)
TABLE_NEWPROCESSENTRY(31,11)
TABLE_NEWPROCESSENTRY(32,11)

#endif