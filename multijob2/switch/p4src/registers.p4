#ifndef _REGISTERS_
#define _REGISTERS_



Register<bit<32>, _>(1) dqueue_alert_threshold;
Register<bit<32>, _>(1) loss_counter;
Register<bit<32>, _>(40000) bitmap;
// Register<bit<32>, _>(40000) appID_and_Seq;
Register<int<32>, _>(40000) appID_and_Seq;
// Register<bit<8>, _>(40000) quantization_level;
Register<bit<32>, _>(40000) agtr_time;
Register<bit<32>, _>(40000) ecn_register;
Register<bit<32>, _>(40000) register1;
Register<bit<32>, _>(40000) register2;
Register<bit<32>, _>(40000) register3;
Register<bit<32>, _>(40000) register4;
Register<bit<32>, _>(40000) register5;
Register<bit<32>, _>(40000) register6;
Register<bit<32>, _>(40000) register7;
Register<bit<32>, _>(40000) register8;
Register<bit<32>, _>(40000) register9;
Register<bit<32>, _>(40000) register10;
Register<bit<32>, _>(40000) register11;
Register<bit<32>, _>(40000) register12;
Register<bit<32>, _>(40000) register13;
Register<bit<32>, _>(40000) register14;
Register<bit<32>, _>(40000) register15;
Register<bit<32>, _>(40000) register16;
Register<bit<32>, _>(40000) register17;
Register<bit<32>, _>(40000) register18;
Register<bit<32>, _>(40000) register19;
Register<bit<32>, _>(40000) register20;
Register<bit<32>, _>(40000) register21;
Register<bit<32>, _>(40000) register22;
Register<bit<32>, _>(40000) register23;
Register<bit<32>, _>(40000) register24;
Register<bit<32>, _>(40000) register25;
Register<bit<32>, _>(40000) register26;
Register<bit<32>, _>(40000) register27;
Register<bit<32>, _>(40000) register28;
Register<bit<32>, _>(40000) register29;
Register<bit<32>, _>(40000) register30;
Register<bit<32>, _>(40000) register31;
Register<bit<32>, _>(40000) register32;



#define DEFINE_REGISTERS_ACTIONS(i,j) \
RegisterAction< bit<32>, _, bit<32> >(register##i##) write_data_entry##i## = { \
    void apply(inout bit<32> value){ \
        value = hdr.p4ml_entries.data##j##; \
    } \
}; \
RegisterAction< bit<32>, _, bit<32> >(register##i##) write_read_data_entry##i## = { \
    void apply(inout bit<32> value, out bit<32> read_value){ \
        value = hdr.p4ml_entries.data##j##; \
        read_value = value; \
    } \
}; \
RegisterAction< bit<32>, _, bit<32> >(register##i##) noequ0_write_data_entry##i## = { \
    void apply(inout bit<32> value){ \
            value = value + hdr.p4ml_entries.data##j##; \
    } \
}; \
RegisterAction< bit<32>, _, bit<32> >(register##i##) noequ0_write_read_data_entry##i## = { \
    void apply(inout bit<32> value, out bit<32> read_value){ \
            value = value + hdr.p4ml_entries.data##j##; \
        read_value = value; \
    } \
}; \
RegisterAction< bit<32>, _, bit<32> >(register##i##) read_data_entry##i## = { \
    void apply(inout bit<32> value, out bit<32> read_value){ \
        read_value = value; \
    } \
}; \
RegisterAction< bit<32>, _, bit<32> >(register##i##) clean_entry##i## = { \
    void apply(inout bit<32> value){ \
        value = 0; \
    } \
}; \





// Define Actions

DEFINE_REGISTERS_ACTIONS(1,0)
DEFINE_REGISTERS_ACTIONS(2,1)
DEFINE_REGISTERS_ACTIONS(3,2)
DEFINE_REGISTERS_ACTIONS(4,3)
DEFINE_REGISTERS_ACTIONS(5,4)
DEFINE_REGISTERS_ACTIONS(6,5)
DEFINE_REGISTERS_ACTIONS(7,6)
DEFINE_REGISTERS_ACTIONS(8,7)
DEFINE_REGISTERS_ACTIONS(9,8)
DEFINE_REGISTERS_ACTIONS(10,9)
DEFINE_REGISTERS_ACTIONS(11,10)
DEFINE_REGISTERS_ACTIONS(12,11)
DEFINE_REGISTERS_ACTIONS(13,12)
DEFINE_REGISTERS_ACTIONS(14,13)
DEFINE_REGISTERS_ACTIONS(15,14)
DEFINE_REGISTERS_ACTIONS(16,15)
DEFINE_REGISTERS_ACTIONS(17,16)
DEFINE_REGISTERS_ACTIONS(18,17)
DEFINE_REGISTERS_ACTIONS(19,18)
DEFINE_REGISTERS_ACTIONS(20,19)
DEFINE_REGISTERS_ACTIONS(21,20)
DEFINE_REGISTERS_ACTIONS(22,21)
DEFINE_REGISTERS_ACTIONS(23,22)
DEFINE_REGISTERS_ACTIONS(24,23)
DEFINE_REGISTERS_ACTIONS(25,24)
DEFINE_REGISTERS_ACTIONS(26,25)
DEFINE_REGISTERS_ACTIONS(27,26)
DEFINE_REGISTERS_ACTIONS(28,27)
DEFINE_REGISTERS_ACTIONS(29,28)
DEFINE_REGISTERS_ACTIONS(30,29)
DEFINE_REGISTERS_ACTIONS(31,30)
DEFINE_REGISTERS_ACTIONS(32,31)


#endif