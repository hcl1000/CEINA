#ifndef _EGRESS_ACTIONS_
#define _EGRESS_ACTIONS_


action do_qdepth() {
    eg_md.mdata.qdepth = (bit<16>)do_comp_qdepth.execute(0);
}

action modify_ecn() {
    // modify_field(p4ml.ECN, 1);
    hdr.p4ml.ECN = 1;
}

action mark_ecn() {
    // bit_or(mdata.is_ecn, mdata.qdepth, mdata.is_ecn);
    eg_md.mdata.is_ecn = (bit<32>)eg_md.mdata.qdepth | eg_md.mdata.is_ecn;
}

action modify_ipv4_ecn() {
    // modify_field(ipv4.ecn, 3);
    hdr.ipv4.ecn = 3;
}

action nop() {
}





// ingress actions

// action check_ecn() {
//     ig_md.mdata.value_one = do_check_ecn.execute(hdr.p4ml_agtr_index.agtr);
// }

// action setup_ecn() { 
//     // modify_field(mdata.is_ecn, 1);    
//     ig_md.mdata.is_ecn = 1;
// }

// action tag_collision_incoming() {
//     // modify_field(p4ml.isSWCollision, 1);
//     hdr.p4ml.isSWCollision =  1;
//     // modify_field(p4ml.bitmap, mdata.isMyAppIDandMyCurrentSeq);
// }

// action set_egr(bit<9> egress_spec) {
//     // modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_spec);
//     ig_intr_md_for_tm.ucast_egress_port = egress_spec;
//     // increase_p4ml_counter.execute(ig_intr_md.ingress_port);
// }

// action set_egr_and_set_index(bit<9> egress_spec) {
//     // modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_spec);
//     ig_intr_md_for_tm.ucast_egress_port = egress_spec;
//     // modify_field(p4ml.dataIndex, 1);
//     hdr.p4ml.dataIndex = 1;
//     // increase_p4ml_counter.execute(ig_intr_md.ingress_port);
// }

#endif