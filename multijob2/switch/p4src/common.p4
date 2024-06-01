#ifndef _COMMON_
#define _COMMON_

RegisterAction<bit<32>, _, bit<32>>(agtr_time) cleaning_agtr_time  = {
    void apply(inout bit<32> value){
        value = 0;
    }
};

RegisterAction<bit<32>, _, bit<32>>(ecn_register) cleaning_ecn  = {
    void apply(inout bit<32> value){
        value = 0;
    }
};



RegisterAction<bit<32>, _, bit<32>>(bitmap) cleaning_bitmap  = {
    void apply(inout bit<32> value){
        value = 0;
    }
};
RegisterAction<bit<32>, _, bit<32>>(bitmap) read_write_bitmap  = {
    void apply(inout bit<32> value, out bit<32> read_value){
        read_value = value;
        value = value | hdr.p4ml.bitmap; // ig_md.mdata.bitmap
    }
};
RegisterAction<bit<32>, _, bit<32>>(bitmap) read_write_bitmap_resend  = {
    void apply(inout bit<32> value, out bit<32> read_value){
        read_value = value; // ig_md.mdata.bitmap
        value = 0; 
    }
};
#ifdef _MODIFY_CONT2_
RegisterAction<bit<32>, _, bit<32>>(bitmap) write_bitmap  = {
    void apply(inout bit<32> value, out bit<32> read_value){
        read_value = 0;
        value = hdr.p4ml.bitmap; // ig_md.mdata.bitmap
    }
};
#endif


#ifdef _DISABLE_CONT2_ 
RegisterAction<int<32>, _, int<32>>(appID_and_Seq) check_app_id_and_seq= {
    void apply(inout int<32> value, out int<32> read_value) {
        if(value == hdr.p4ml.PAS || value == 0){ // identical
            value = hdr.p4ml.PAS;
            read_value = value;
        }
    }
};
RegisterAction<int<32>, _, int<32>>(appID_and_Seq) check_app_id_and_seq_resend  = {
    void apply(inout int<32> value, out int<32> read_value){
        if (value == hdr.p4ml.PAS){
            read_value = value; // ig_md.mdata.isMyAppIDandMyCurrentSeq;
            value = 0;
        }
    }
};

#else
    #ifdef _MODIFY_CONT2_
        RegisterAction<int<32>, _, int<32>>(appID_and_Seq) check_app_id_and_seq= {
            void apply(inout int<32> value, out int<32> read_value) {
                if(value - hdr.p4ml.PAS == 0){ // identical
                    // read_value = value - hdr.p4ml.PAS;
                    read_value = hdr.p4ml.PAS - value;
                }
                else if((value - hdr.p4ml.PAS) < 0){ // preemption (initial case)
                    // read_value = value - hdr.p4ml.PAS;
                    read_value = hdr.p4ml.PAS - value;
                    value = hdr.p4ml.PAS;
                }
                else{ // emit and pass
                    // read_value = 1
                    read_value = 0x7FFFFFFF;
                }
            }
        };
        RegisterAction<int<32>, _, int<32>>(appID_and_Seq) check_app_id_and_seq_resend  = {
            void apply(inout int<32> value, out int<32> read_value){
                if (value - hdr.p4ml.PAS == 0){
                    read_value = value - hdr.p4ml.PAS; // ig_md.mdata.isMyAppIDandMyCurrentSeq;
                    value = 0;
                }
                else{
                    // read_value = 1;
                    read_value = 0x7FFFFFFF;
                }
            }
        };


    #else
        RegisterAction<int<32>, _, int<32>>(appID_and_Seq) check_app_id_and_seq= {
            void apply(inout int<32> value, out int<32> read_value) {
                if(value - hdr.p4ml.PAS == 0){ // identical
                    read_value = hdr.p4ml.PAS;
                }
                else if((value - hdr.p4ml.PAS) < 0){ // preemption (initial case)
                    value = hdr.p4ml.PAS;
                    read_value = hdr.p4ml.PAS;
                }
                else{ // emit and pass
                    read_value = 0;
                }
            }
        };
        RegisterAction<int<32>, _, int<32>>(appID_and_Seq) check_app_id_and_seq_resend  = {
            void apply(inout int<32> value, out int<32> read_value){
                if (value == hdr.p4ml.PAS){
                    read_value = value; // ig_md.mdata.isMyAppIDandMyCurrentSeq;
                    value = 0;
                }
            }
        };

    #endif
#endif






// RegisterAction<bit<32>, _, bit<32>>(appID_and_Seq) check_app_id_and_seq_force  = {
//     void apply(inout bit<32> value, out bit<32> read_value){
//         value = hdr.p4ml.appIDandSeqNum;
//         read_value = value; // ig_md.mdata.isMyAppIDandMyCurrentSeq;
//     }
// };


RegisterAction<int<32>, _, int<32>>(appID_and_Seq) clean_app_id_and_seq  = {
    void apply(inout int<32> value, out int<32> read_value){
        if (value == hdr.p4ml.PAS){
            value = 0;
            read_value = hdr.p4ml.PAS; // ig_md.mdata.isMyAppIDandMyCurrentSeq;
        }
    }
};

RegisterAction<int<32>, _, int<32>>(appID_and_Seq) clean_app_id_and_seq_force  = {
    void apply(inout int<32> value, out int<32> read_value){
        // if (value == hdr.p4ml.appIDandSeqNum){
            value = 0;
            // read_value = hdr.p4ml.appIDandSeqNum; // ig_md.mdata.isMyAppIDandMyCurrentSeq;
        // }
    }
};

RegisterAction<bit<32>, _, bit<32>>(agtr_time) check_agtrTime  = {
    void apply(inout bit<32> value, out bit<32> read_value){
        if (ig_md.mdata.isAggregate != 0){
            value = value + 1;
        }
        read_value = value; // ig_md.mdata.current_agtr_time; 
    }
};

RegisterAction<bit<32>, _, bit<32>>(agtr_time) check_resend_agtrTime  = {
    void apply(inout bit<32> value, out bit<32> read_value){
        if (ig_md.mdata.isAggregate != 0){
            value = 0;
        }
        else{
            value = 0;
        }
        read_value = (bit<32>)hdr.p4ml.agtr_time; // ig_md.mdata.mdata.current_agtr_time; 
    }
};

#ifdef _MODIFY_CONT2_
RegisterAction<bit<32>, _, bit<32>>(agtr_time) write_agtrTime  = {
    void apply(inout bit<32> value, out bit<32> read_value){
        value = 1;
        read_value = 1; // ig_md.mdata.current_agtr_time; 
    }
};
#endif


//Egress
// RegisterAction<bit<32>, _, bit<32>>(dqueue_alert_threshold) do_comp_qdepth  = {
//     void apply(inout bit<32> value, out bit<32> read_value){
//         if (eg_intr_md.deq_qdepth >= 1000){
//             read_value = eg_intr_md.deq_qdepth; // eg_md.mdata.qdepth;
//         }
//     }
// };

RegisterAction<bit<32>, _, bit<32>>(ecn_register) do_check_ecn  = {
    void apply(inout bit<32> value, out bit<32> read_value){
        if (value == 1){
            value = value | ig_md.mdata.is_ecn;
            read_value = (bit<32>)hdr.p4ml.ECN; // ig_md.mdata.value_one; 
        }
    }
};



// RegisterAction<bit<8>, _, bit<8>>(quantization_level) check_quan_level_action  = {
//     void apply(inout bit<8> value, out bit<8> read_value){
//         if( value > (bit<8>)hdr.p4ml.quantization_level || value == 0){
//             // preemption (3->1)
//             value = (bit<8>)hdr.p4ml.quantization_level;
//             read_value = 1;
//         }
//         else{
//             // not preemption (1->1, 1->3, 3->3)
//             read_value = 0;
//         }
//     }
// };

// RegisterAction<bit<8>, _, bit<8>>(quantization_level) clean_quan_level_action  = {
//     void apply(inout bit<8> value, out bit<8> read_value){
//             value = 0;
//     }
// };


















// Action
action process_bitmap() {
    ig_md.mdata.bitmap = read_write_bitmap.execute(hdr.p4ml_agtr_index.agtr);
    ig_md.mdata.isAggregate = hdr.p4ml.bitmap & ~ig_md.mdata.bitmap; ////-   
    ig_md.mdata.integrated_bitmap = hdr.p4ml.bitmap | ig_md.mdata.bitmap; ////-
 
}

action process_bitmap_resend() {
    ig_md.mdata.bitmap = read_write_bitmap_resend.execute(hdr.p4ml_agtr_index.agtr);
    ig_md.mdata.isAggregate = hdr.p4ml.bitmap & ~ig_md.mdata.bitmap; ////-
    ig_md.mdata.integrated_bitmap = hdr.p4ml.bitmap | ig_md.mdata.bitmap; ////-
 
}


// TODO:
action check_aggregate_and_forward() {
    // this is is for aggregation needed checking
    // ig_md.mdata.isAggregate = hdr.p4ml.bitmap & ig_md.mdata.bitmap; ////-
    ig_md.mdata.isAggregate = hdr.p4ml.bitmap & ~ig_md.mdata.bitmap; ////-
    ig_md.mdata.integrated_bitmap = hdr.p4ml.bitmap | ig_md.mdata.bitmap; ////-
}


action clean_agtr_time() {
    cleaning_agtr_time.execute(hdr.p4ml_agtr_index.agtr);
}

action clean_ecn() {
    cleaning_ecn.execute(hdr.p4ml_agtr_index.agtr);
}

action clean_bitmap() {
    cleaning_bitmap.execute(hdr.p4ml_agtr_index.agtr);
}

#ifdef _ENABLE_CONT3_
action multicast(bit<16> group, bit<5> qid) { ///FIXED
    ig_intr_md_for_tm.mcast_grp_a = group;
    ig_intr_md_for_tm.qid = qid;
}
#else
action multicast(bit<16> group) {
    ig_intr_md_for_tm.mcast_grp_a = group;
}
#endif

action check_appID_and_seq() {
    // ig_md.mdata.isMyAppIDandMyCurrentSeq = (bit<16>)check_app_id_and_seq.execute(hdr.p4ml_agtr_index.agtr);
    ig_md.mdata.isMyPAS = check_app_id_and_seq.execute(hdr.p4ml_agtr_index.agtr);
    //modify_field(mdata.qdepth, 0);   
}

// action check_appID_and_seq_force() {
    // ig_md.mdata.isMyAppIDandMyCurrentSeq = (bit<16>)check_app_id_and_seq.execute(hdr.p4ml_agtr_index.agtr);
    // ig_md.mdata.isMyAppIDandMyCurrentSeq = check_app_id_and_seq_force.execute(hdr.p4ml_agtr_index.agtr);
    //modify_field(mdata.qdepth, 0);   
// }

action check_appID_and_seq_resend() {
    // ig_md.mdata.isMyAppIDandMyCurrentSeq = (bit<16>)check_app_id_and_seq_resend.execute(hdr.p4ml_agtr_index.agtr);
    ig_md.mdata.isMyPAS = check_app_id_and_seq_resend.execute(hdr.p4ml_agtr_index.agtr);
 //   modify_field(mdata.qdepth, 0);   
}

action clean_appID_and_seq() {
    // ig_md.mdata.isMyAppIDandMyCurrentSeq = (bit<16>)clean_app_id_and_seq.execute(hdr.p4ml_agtr_index.agtr);
    ig_md.mdata.isMyPAS = clean_app_id_and_seq.execute(hdr.p4ml_agtr_index.agtr);
    // ig_md.mdata.isMyAppIDandMyCurrentSeq = hdr.p4ml.appIDandSeqNum;
    // hdr.ipv4.dstAddr = clean_app_id_and_seq.execute(hdr.p4ml_agtr_index.agtr);
}

action clean_appID_and_seq_force() {
    clean_app_id_and_seq_force.execute(hdr.p4ml_agtr_index.agtr);
}

action check_agtr_time() {
    ig_md.mdata.current_agtr_time = (bit<8>)check_agtrTime.execute(hdr.p4ml_agtr_index.agtr);
    // ig_md.mdata.agtr_complete = (bit<8>)check_agtrTime.execute(hdr.p4ml_agtr_index.agtr);
    // ig_md.mdata.agtr_complete = ig_md.mdata.current_agtr_time; ///
    ig_md.mdata.agtr_complete = ig_md.mdata.current_agtr_time ^ hdr.p4ml.agtr_time; ///
    ig_md.mdata.agtr_complete = ~ig_md.mdata.agtr_complete;
    
}

action check_resend_agtr_time() {
    ig_md.mdata.current_agtr_time = (bit<8>)check_resend_agtrTime.execute(hdr.p4ml_agtr_index.agtr);
    ig_md.mdata.agtr_complete = ig_md.mdata.current_agtr_time ^ hdr.p4ml.agtr_time; ///
    ig_md.mdata.agtr_complete = ~ig_md.mdata.agtr_complete;
}

action modify_packet_bitmap() {
    // modify_field(p4ml.bitmap, mdata.integrated_bitmap);
    hdr.p4ml.bitmap = ig_md.mdata.integrated_bitmap;
}

// egress actions

// action do_qdepth() {
//     eg_md.mdata.qdepth = do_comp_qdepth.execute(0);
// }

// action modify_ecn() {
//     // modify_field(p4ml.ECN, 1);
//     hdr.p4ml.ECN = 1;
// }

// action mark_ecn() {
//     // bit_or(mdata.is_ecn, mdata.qdepth, mdata.is_ecn);
//     eg_md.mdata.is_ecn = eg_md.mdata.qdepth | eg_md.mdata.is_ecn;
// }

// action modify_ipv4_ecn() {
//     // modify_field(ipv4.ecn, 3);
//     hdr.ipv4.ecn = 3;
// }

action check_ecn() {
    ig_md.mdata.value_one = (bit<1>)do_check_ecn.execute(hdr.p4ml_agtr_index.agtr);
}

action setup_ecn() {
    // modify_field(mdata.is_ecn, 1);    
    ig_md.mdata.is_ecn = 1;
}

action tag_collision_incoming() {
    // modify_field(p4ml.isSWCollision, 1);
    // hdr.p4ml_bg.isSWCollision =  1;
    // modify_field(p4ml.bitmap, mdata.isMyAppIDandMyCurrentSeq);
    hdr.p4ml.isWCollision = 1;
}

#ifdef _ENABLE_CONT3_
action set_egr(bit<9> egress_spec, bit<5> qid) {
    ig_intr_md_for_tm.ucast_egress_port = egress_spec;
    ig_intr_md_for_tm.qid = qid;
}
#else
action set_egr(bit<9> egress_spec) {
    // modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_spec);
    ig_intr_md_for_tm.ucast_egress_port = egress_spec;
    // increase_p4ml_counter.execute(ig_intr_md.ingress_port);
}
#endif

action set_egr_and_set_index(bit<9> egress_spec) {
    // modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_spec);
    ig_intr_md_for_tm.ucast_egress_port = egress_spec;
    // modify_field(p4ml.dataIndex, 1);
    hdr.p4ml.dataIndex = 1;
    // increase_p4ml_counter.execute(ig_intr_md.ingress_port);
    
}

action set_egr_and_set_index_queue(bit<9> egress_spec, bit<5> qid) {
    ig_intr_md_for_tm.ucast_egress_port = egress_spec;
    hdr.p4ml.dataIndex = 1;
    ig_intr_md_for_tm.qid = qid;
    // hdr.ipv4.dstAddr = (bit<32>)ig_md.mdata.isMyPAS;
}

action select_queue(bit<5> qid){
    ig_intr_md_for_tm.qid = qid;
}

action nop()
{
}

action drop_pkt() {
    // drop();
    ig_intr_md_for_dprsr.drop_ctl = 1;

}

#ifdef _MODIFY_CONT2_
action process_bitmap_preemption(){
    ig_md.mdata.bitmap = write_bitmap.execute(hdr.p4ml_agtr_index.agtr);
    ig_md.mdata.isAggregate = hdr.p4ml.bitmap & ~ig_md.mdata.bitmap; ////-   
    ig_md.mdata.integrated_bitmap = hdr.p4ml.bitmap | ig_md.mdata.bitmap; ////-
}
table bitmap_table {
    key = {
        ig_md.mdata.isMyPAS : ternary;
    }
    actions =  {
        process_bitmap;
        process_bitmap_preemption;
    }
    // default_action = process_bitmap();
    // size = 1;
}
#else
table bitmap_table {
    // key = {
    //     ig_md.mdata.preemption : ternary;
    // }
    actions =  {
        process_bitmap;
    }
    default_action = process_bitmap();
    size = 1;
}
#endif

table bitmap_resend_table {
    // key = {
    //     ig_md.mdata.preemption : ternary;
    // }
    actions =  {
        process_bitmap_resend;
    }
    default_action = process_bitmap_resend();
    size = 1;
}

table bitmap_aggregate_table {
    // key = {
    //     ig_md.mdata.preemption : ternary;
    // }
    actions =  {
        check_aggregate_and_forward;
    }
    default_action = check_aggregate_and_forward();
    size = 1;
}
#ifdef _MODIFY_CONT2_
action check_agtr_time_preemption(){
    ig_md.mdata.current_agtr_time = (bit<8>)write_agtrTime.execute(hdr.p4ml_agtr_index.agtr);
    ig_md.mdata.agtr_complete = ig_md.mdata.current_agtr_time ^ hdr.p4ml.agtr_time; ///
    ig_md.mdata.agtr_complete = ~ig_md.mdata.agtr_complete;
    
}
table agtr_time_table {
    key = {
        ig_md.mdata.isMyPAS : ternary;
    }
    actions =  {
        check_agtr_time;
        check_agtr_time_preemption;
    }
    // default_action = check_agtr_time();
    // size = 1;
}

#else

table agtr_time_table {
    // key = {
    //     ig_md.mdata.preemption : ternary;
    // }
    actions =  {
        check_agtr_time;
    }
    default_action = check_agtr_time();
    size = 1;
}
#endif

table agtr_time_resend_table {
    // key = {
    //     ig_md.mdata.preemption : ternary;
    // }
    actions =  {
        check_resend_agtr_time;
    }
    default_action = check_resend_agtr_time();
    size = 1;
}

// table immd_outPort_table {
//     key = {
//         // p4ml.appIDandSeqNum mask 0xFFFF0000: exact;
//         hdr.p4ml.appIDandSeqNum : ternary;

//     }
//     actions =  {
//         set_egr;
//     }
// }

table outPort_table {
    key =  {
        // p4ml.appIDandSeqNum mask 0xFFFF0000: exact;
        hdr.p4ml.PAS : ternary;
        ig_intr_md.ingress_port: exact;
        hdr.p4ml.dataIndex: exact;
        hdr.p4ml.PSIndex: exact;
        ig_md.mdata.isAggregate : ternary; ///
        ig_md.mdata.agtr_complete : ternary; /// 
    }
    actions =  {
		nop;
        set_egr;
        set_egr_and_set_index;
        set_egr_and_set_index_queue;
        drop_pkt;
    }
    default_action = drop_pkt();
}

table outPort_table2 {
    key =  {
        // p4ml.appIDandSeqNum mask 0xFFFF0000: exact;
        hdr.p4ml.PAS : ternary;
        ig_intr_md.ingress_port: exact;
        hdr.p4ml.dataIndex: exact;
        hdr.p4ml.PSIndex: exact;
        ig_md.mdata.isAggregate : ternary; ///
        ig_md.mdata.agtr_complete : ternary; /// 
    }
    actions =  {
		nop;
        set_egr;
        set_egr_and_set_index;
        set_egr_and_set_index_queue;
        drop_pkt;
    }
    default_action = drop_pkt();
}

table select_queue_table{
    key = {
        hdr.p4ml.isACK : exact;
    }
    actions =  {
		nop;
        select_queue;
    }

}


table bg_outPort_table {
    key =  {
        // useless here, just can't use default action for variable
        // hdr.p4ml_bg.isACK : exact;
    }
    actions =  {
        set_egr;
		nop;
    }
}

table multicast_table {
    key =  {
        hdr.p4ml.isACK: exact;
        // hdr.p4ml.appIDandSeqNum mask 0xFFFF0000: exact;
        hdr.p4ml.PAS : ternary;
        ig_intr_md.ingress_port: exact;
        hdr.p4ml.dataIndex: exact;
    }
    actions =  {
        multicast; drop_pkt; set_egr_and_set_index;
    }
    default_action = drop_pkt();
}

// @pragma stage 3
table clean_agtr_time_table {
    actions =  {
        clean_agtr_time;
    }
    default_action = clean_agtr_time();
    size = 1;
}

table clean_ecn_table {
    actions =  {
        clean_ecn;
    }
    default_action = clean_ecn();
    size = 1;
}


table clean_bitmap_table {
    actions =  {
        clean_bitmap;
    }
    default_action = clean_bitmap();
    size = 1;
}

/* Counter */
Register<bit<32>, bit<32>>(1) p4ml_counter;

RegisterAction<bit<32>, _, bit<32>>(p4ml_counter) increase_p4ml_counter = {
    void apply(inout bit<32> value){
        value = value + 1;
    }
};

// table forward_counter_table {
//         actions =  {
//         increase_counter;
//     }
//     default_action = increase_counter();
// }

table appID_and_seq_table {
        actions =  {
        check_appID_and_seq;
    }
    default_action = check_appID_and_seq();
}

// table appID_and_seq_table_force {
//         actions =  {
//         check_appID_and_seq_force;
//     }
//     default_action = check_appID_and_seq_force();
// }

table appID_and_seq_resend_table {
        actions =  {
        check_appID_and_seq_resend;
    }
    default_action = check_appID_and_seq_resend();
}

table clean_appID_and_seq_table {
        actions =  {
        clean_appID_and_seq;
    }
    default_action = clean_appID_and_seq();
}

table clean_appID_and_seq_table_force {
        actions =  {
        clean_appID_and_seq_force;
    }
    default_action = clean_appID_and_seq_force();
}

table modify_packet_bitmap_table {
    key =  {
        hdr.p4ml.dataIndex: exact;
        ig_md.mdata.isAggregate : ternary; ///
        ig_md.mdata.agtr_complete : ternary; ///        
    }
        actions =  {
        modify_packet_bitmap; nop;
    }
    default_action = modify_packet_bitmap(); //
    // default_action = nop(); //
}

// table qdepth_table {
//     actions =  {
//         do_qdepth;
//     }
//     default_action = do_qdepth();
// }

// table modify_ecn_table {
//     actions =  {
//         modify_ecn;
//     }
//     default_action = modify_ecn();
// }

// table mark_ecn_ipv4_table {
//     actions =  {
//         modify_ipv4_ecn;
//     }
//     default_action = modify_ipv4_ecn();
// }

// unused
// table ecn_mark_table {
//     actions =  {
//         mark_ecn;
//     }
//     default_action = mark_ecn();
// }

table ecn_register_table {
    actions =  {
        check_ecn;
    }
    default_action = check_ecn();
}

table setup_ecn_table {
    actions =  {
        setup_ecn;
    }
    default_action = setup_ecn();
}


#ifdef _ENABLE_CONT3_
table forward {
    key =  {
        hdr.ethernet.dstAddr : ternary;
        hdr.ipv4.dstAddr : ternary;
        hdr.ipv4.protocol : ternary;
        hdr.p4ml.isACK : ternary;
    }
    actions =  {
        set_egr; nop; drop_pkt;
    }
    default_action = drop_pkt();
}

#else
table forward {
    key =  {
        hdr.ethernet.dstAddr : exact;
    }
    actions =  {
        set_egr; nop; drop_pkt;
    }
    default_action = drop_pkt();
}
#endif


#ifdef _ARP_
    action send_ipv4(bit<9> port, bit<48> mac) {
        ig_intr_md_for_tm.ucast_egress_port = port;
        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = mac;

    }
    action send_ip(bit<9> port) {
        ig_intr_md_for_tm.ucast_egress_port = port;
        ig_intr_md_for_tm.bypass_egress = 1;
        // hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
        // hdr.ethernet.dstAddr = 
    }



    action send_back() {
        ig_intr_md_for_tm.ucast_egress_port = ig_intr_md.ingress_port;
    }

    action send_arp_reply(bit<48> switch_mac, bit<32> switch_ip) {
        hdr.ethernet.dstAddr = hdr.arp_ipv4.src_hw_addr;
        hdr.ethernet.srcAddr = switch_mac;

        hdr.arp.opcode = 2;
        hdr.arp_ipv4.dst_hw_addr    = hdr.arp_ipv4.src_hw_addr;
        hdr.arp_ipv4.dst_proto_addr = hdr.arp_ipv4.src_proto_addr;
        hdr.arp_ipv4.src_hw_addr    = switch_mac;
        hdr.arp_ipv4.src_proto_addr = switch_ip;

        send_back();
    }

    action send_icmp_echo_reply(bit<48> switch_mac, bit<32> switch_ip) {
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = switch_mac;

        hdr.ipv4.dstAddr = hdr.ipv4.srcAddr;
        hdr.ipv4.srcAddr = switch_ip;

        // hdr.icmp.msg_type = icmp_type_t.ECHO_REPLY;
        // hdr.icmp.checksum = 0;

        send_back();
    }


    table l3 {
        key = {
            hdr.ipv4.dstAddr : ternary;
        }
        actions = {
            nop;
            send_ipv4;
        }
        default_action = nop();
        // const entries = {
        //     0x140a0005 : send_ipv4(168, 0x3cecef428bff); // host9 
        //     0x140a0006 : send_ipv4(169, 0x3cecef428bbb); // gpu3 (tofino)
        //     0x140a0007 : send_ipv4(170, 0x3cecef428c5f); // host7
        //     0x140a0008 : send_ipv4(171, 0x3cecef41ffcd); // host8
        //     _ : send_ipv4(171, 0x3cecef41ffcd); // host8
        // }
    }

    table arp_icmp {
        key = {
            hdr.arp_ipv4.isValid()      : exact;
            // hdr.icmp.isValid()          : exact;
            hdr.arp.opcode              : ternary;
            hdr.arp_ipv4.dst_proto_addr : ternary;
            // hdr.icmp.msg_type           : ternary;
            // hdr.ipv4.dstAddr           : ternary;
        }
        actions = {
            send_arp_reply;
            send_icmp_echo_reply;
            send_ipv4;
            send_ip;
        }
        // const entries = {
        //     (true, 1, 0x00000009) : send_ip(168); // 9를 알고싶으니
        //     (true, 1, 0x140a0006) : send_ip(169);
        //     (true, 1, 0x140a0007) : send_ip(170);
        //     (true, 1, 0x140a0008) : send_ip(171);
            
        //     (true, 2, 0x140a0005) : send_ip(168);
        //     (true, 2, 0x140a0006) : send_ip(169);
        //     (true, 2, 0x140a0007) : send_ip(170);
        //     (true, 2, 0x140a0008) : send_ip(171);
        //     // (true, 1, 0x3cecef41ffcd) : send_arp_reply(0x123456789012, 0x140a00fe);
        //     // () : send_icmp_echo_reply();
            
        // }
    }

    action action_req_arp(bit<48> mac_addr){
        bit<48> temp_mac;
        bit<32> temp_ip;
        temp_mac = hdr.arp_ipv4.src_hw_addr;
        temp_ip = hdr.arp_ipv4.src_proto_addr;

        // 출발지 MAC/IP를 목적지MAC값/목적지IP값으로 수정
        hdr.arp_ipv4.src_hw_addr = mac_addr;
        hdr.arp_ipv4.src_proto_addr = hdr.arp_ipv4.dst_proto_addr;

        // 목적지 MAC/IP를 송신자MAC값/송신자IP값으로 수정
        hdr.arp_ipv4.dst_hw_addr = temp_mac;
        hdr.arp_ipv4.dst_proto_addr = temp_ip;

        // 출력 포트를 입력 포트로.
        ig_intr_md_for_tm.ucast_egress_port = ig_intr_md.ingress_port;

    }

    table req_arp{
        key = {
            hdr.arp_ipv4.dst_proto_addr : exact;
        }
        actions = {
            action_req_arp;
            nop;
        }
        default_action = nop();
        const entries ={
            0x0101010D : action_req_arp(0x00261403f127);
            0x01010109 : action_req_arp(0x3cecef0ca0e6);
        }
    }


#endif



table drop_table {
    key =  {
        ig_intr_md.ingress_port: exact;
        hdr.p4ml.dataIndex : exact;
        ig_md.mdata.isAggregate : ternary; ///
        ig_md.mdata.agtr_complete : ternary; ///
        ig_intr_md.resubmit_flag : exact; ///
    }
    actions =  {
        drop_pkt; set_egr; set_egr_and_set_index; nop;
    }
    // default_action = drop_pkt();
    default_action = nop();
}

table tag_collision_incoming_table {
    actions =  {
        tag_collision_incoming;
    }
    default_action = tag_collision_incoming();
}



// action preemption(){
    // ig_md.mdata.preemption = 1;
    // ig_md.mdata.isMyAppIDandMyCurrentSeq = 0;
    // hdr.p4ml.isWCollision = 1;
// }

// action not_preemption(){
//     ig_md.mdata.preemption = 0;
//     hdr.p4ml.isWCollision = 1;
// }

// action check_action(){
//     ig_md.mdata.preemption = check_quan_level_action.execute(hdr.p4ml_agtr_index.agtr)[0:0];

// }

// action clean_quantization_level_action(){
//     clean_quan_level_action.execute(hdr.p4ml_agtr_index.agtr);
// }



// table check_quantization_level_table {
//     actions = {
//         check_action;
//     }
//     default_action = check_action;
// }

// table clean_quantization_level_table {
//     actions = {
//         clean_quantization_level_action;
//     }
//     default_action = clean_quantization_level_action;
// }




#endif