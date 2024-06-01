#include <core.p4>
#include <tna.p4>

#define _CEINA_
#define _ENABLE_CONT3_
// #define _ARP_

#include "headers.p4"
#include "parser_ceina.p4"

control Ingress(
    inout header_t hdr,
    inout ingress_metadata_t ig_md,
    in ingress_intrinsic_metadata_t ig_intr_md,
    in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
    inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm) {

#include "registers.p4"
#include "actions.p4"
#include "tables.p4"
#include "common.p4"


    action do_recirculation(){
        // resubmit.emit(ig_md.mdata.agtr_time); //TODO
        // ig_intr_md_for_dprsr.resubmit_type = 1;
        // hdr.resubmit_meta.agtr_time = ig_md.mdata.agtr_time;
        ig_intr_md_for_tm.ucast_egress_port = 196;
        hdr.p4ml.dataIndex = 1;
    }

    table p4ml_recirculation{
        key = {
            ig_md.mdata.isAggregate : ternary; 
            ig_md.mdata.agtr_complete : ternary; ////
            // ig_intr_md.dataIndex : exact; ////
        }
        actions = {
            do_recirculation;
        }
        default_action = do_recirculation();
        size = 8;

    }


// apply
apply{


#ifdef _ENABLE_CONT3_
    if ( hdr.p4ml.isValid() && hdr.ipv4.ttl == 0) { ///ADDED
#else
    if ( hdr.p4ml.isValid() ) {
#endif
            // ack packet
            if (hdr.p4ml.isACK == 1) {

                // if (hdr.p4ml.overflow == 1 && hdr.p4ml.isResend == 0) {
                // } 
                // else {
                    clean_appID_and_seq_table.apply();
                    // clean_quantization_level_table.apply();
                    
                    if (ig_md.mdata.isMyPAS != 0) {
                        /* Clean */
                        // clean_quantization_level_table.apply(); ////
                        clean_bitmap_table.apply();
                        clean_ecn_table.apply();
                        clean_agtr_time_table.apply();
                        
                        // apply(cleanEntry1);
                    }
                // }

                /* Multicast Back */
                if(ig_intr_md.ingress_port == 196) {
                    // hdr.ipv4.dstAddr = 1234;
                    // hdr.ethernet.srcAddr = 1;
                    multicast_table.apply(); ///
                } else {
                    p4ml_recirculation.apply(); ///
                }
                
            } else {

                        if (hdr.p4ml.isResend == 1) {
                            appID_and_seq_resend_table.apply();
                        } 
                        else {
                            appID_and_seq_table.apply();
                        }

                        
                        
                        // Correct ID and Seq
                        if (ig_md.mdata.isMyPAS != 0){ // && ig_md.mdata.preemption == 1) {

                                if (hdr.p4ml.isResend == 1) {
                                    // Clean the bitmap also
                                    bitmap_resend_table.apply();
                                } else {
                                    bitmap_table.apply();
                                }

                                ecn_register_table.apply();
                                // bitmap_aggregate_table.apply();

                                if (hdr.p4ml.isResend == 1) {
                                    // Force forward and clean
                                    agtr_time_resend_table.apply();
                                } else {
                                    agtr_time_table.apply();
                                }

                                NewprocessEntry1.apply();
                                NewprocessEntry2.apply();
                                NewprocessEntry3.apply();
                                NewprocessEntry4.apply();
                                NewprocessEntry5.apply();
                                NewprocessEntry6.apply();
                                NewprocessEntry7.apply();
                                NewprocessEntry8.apply();
                                NewprocessEntry9.apply();
                                NewprocessEntry10.apply();
                                NewprocessEntry11.apply();
                                NewprocessEntry12.apply();
                                NewprocessEntry13.apply();
                                NewprocessEntry14.apply();
                                NewprocessEntry15.apply();
                                NewprocessEntry16.apply();
                                NewprocessEntry17.apply();
                                NewprocessEntry18.apply();
                                NewprocessEntry19.apply();
                                NewprocessEntry20.apply();
                                NewprocessEntry21.apply();
                                NewprocessEntry22.apply();
                                NewprocessEntry23.apply();
                                NewprocessEntry24.apply();
                                NewprocessEntry25.apply();
                                NewprocessEntry26.apply();
                                NewprocessEntry27.apply();
                                NewprocessEntry28.apply();
                                NewprocessEntry29.apply();
                                NewprocessEntry30.apply();
                                NewprocessEntry31.apply();
                                NewprocessEntry32.apply();


                                drop_table.apply(); /// isAggregator / agtr_complte / resubmit_flag 매치키에 추가 
                                // p4ml_resubmit2.apply(); /// isAggregator / agtr_complte / resubmit_flag 매치키에 추가
                                modify_packet_bitmap_table.apply(); /// isAggregator / agtr_complte 매치키에 추가
                                outPort_table.apply(); /// isAggregator / agtr_complte 매치키에 추가


                        } else {
                            /* tag collision bit in incoming one */
                            // if not empty
                            if (hdr.p4ml.isResend == 0) {
                                tag_collision_incoming_table.apply();
                            }
                            outPort_table2.apply();
                            // select_queue_table.apply();
                        }
                    // }
            }
    } else {
        // // BG traffic doesn't have data layer
        forward.apply();

#ifdef _ARP_
        if(hdr.arp.isValid()){
            arp_icmp.apply();
            if(hdr.arp_ipv4.dst_proto_addr == 0xffffffff) {
                req_arp.apply();
        }
        if(hdr.ipv4.isValid()){
            l3.apply();
        }
#endif
    }
    }

        // MIRROR : FIXME:
        // session_id_allocation_table.apply();
        // ig_intr_md_for_dprsr.mirror_type = 1; 
        // // ig_md.mdata.session_id = 1;
        // ig_md.mdata.header_type = 0xC;
        // ig_md.mdata.header_info = 1;
        // ig_md.mdata.ingress_port = ig_intr_md.ingress_port; //FIXME:
        // // ig_md.mdata.padding2 = 0;                           //FIXME:
        // ig_intr_md_for_tm.ucast_egress_port = 171;

    
}
}


control Egress(
    inout header_t hdr,
    inout egress_metadata_t eg_md,
    in egress_intrinsic_metadata_t eg_intr_md,
    in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
    inout egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md,
    inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) 
{

#include "egress_registers.p4"
#include "egress_actions.p4"
#include "egress_tables.p4"

    Wred<bit<19>, bit<14>>(size= 1 << 14,
                            drop_value = 1,
                            no_drop_value = 0) wred;


    action header_validation_action(){
        hdr.p4ml.aggregation_complete_flag = 1; // for 1-bit recirculation pkt
        hdr.p4ml_entries.setInvalid();
        hdr.p4ml_entries_global_1bit.setValid();
    }

    table header_validation_table{
        key={
        }
        actions = {
            header_validation_action();
        }
        default_action = header_validation_action();

    }


    apply{
        qdepth_table.apply();
        if (hdr.ipv4.isValid()) {

#ifdef _ENABLE_CONT3_
            ttl_table.apply();
            eg_intr_dprs_md.drop_ctl = (bit<3>)wred.execute(eg_intr_md.deq_qdepth,
                                        eg_intr_md.egress_port ++ eg_intr_md.egress_qid);
            action_drop_counter();            
#endif

            if (eg_md.mdata.qdepth != 0) {
                mark_ecn_ipv4_table.apply();
                modify_ecn_table.apply(); ///

            }


        }



        if(hdr.p4ml.isResend == 1 || hdr.p4ml.isACK == 1){

        }
        else{
            if(hdr.p4ml.quantization_level == 1 && eg_md.mdata.agtr_complete == 0xFF){ //TODO : change to other field (voting)

                // if(eg_intr_md.egress_port == 3){ // TODO
                    // hdr.p4ml_agtr_index_useless2.setInvalid();
                    // hdr.p4ml_entries_1bit.setValid();
                // }
                // hdr.p4ml_entries_global_1bit.setValid();

                voting_table1.apply();
                voting_table2.apply();
                voting_table3.apply();
                voting_table4.apply();
                voting_table5.apply();
                voting_table6.apply();
                voting_table7.apply();
                voting_table8.apply();
                voting_table9.apply();
                voting_table10.apply();
                voting_table11.apply();
                voting_table12.apply();
                voting_table13.apply();
                voting_table14.apply();
                voting_table15.apply();
                voting_table16.apply();
                voting_table17.apply();
                voting_table18.apply();
                voting_table19.apply();
                voting_table20.apply();
                voting_table21.apply();
                voting_table22.apply();
                voting_table23.apply();
                voting_table24.apply();
                voting_table25.apply();
                voting_table26.apply();
                voting_table27.apply();
                voting_table28.apply();
                voting_table29.apply();
                voting_table30.apply();
                voting_table31.apply();
                voting_table32.apply();

                header_validation_table.apply();
                
            }
        }


    }
}


Pipeline(IngressParser(),
         Ingress(),
         IngressDeparser(),
         EgressParser(),
         Egress(),
         EgressDeparser()) pipe;

Switch(pipe) main;
