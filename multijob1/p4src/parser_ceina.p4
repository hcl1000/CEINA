#ifndef _PARSERS_
#define _PARSERS_

#include "headers.p4"



parser IngressParser(
    packet_in pkt,
    out header_t hdr,
    out ingress_metadata_t ig_md,
    out ingress_intrinsic_metadata_t ig_intr_md) {

    Checksum() ipv4_checksum;

    state start {
        pkt.extract(ig_intr_md);
        transition select(ig_intr_md.resubmit_flag) {
            1 : check_if_resubmit;
            default : parse_port_metadata;
        }
    }

    state check_if_resubmit {
    	pkt.extract<resubmit_metadata_t>(hdr.resubmit_meta);
        transition parse_ethernet;
    }

    state parse_port_metadata {
        pkt.advance(PORT_METADATA_SIZE);

        // // Mirror
        // hdr.bridge.setValid();
        // hdr.bridge.header_type  = 0xB;
        // hdr.bridge.header_info  = 0;
        // // hdr.bridge.ingress_port = ig_intr_md.ingress_port; 
        // hdr.bridge.padding2     = 0;

        transition parse_ethernet;
    }

    state parse_ethernet {
        
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
#ifdef _ARP_
            16w0x0806 : parse_arp;
#endif
            0x0800 : parse_ipv4;
            default : accept;
        }
    }


#ifdef _ARP_
    state parse_arp {
        pkt.extract(hdr.arp);
        transition select(hdr.arp.hw_type, hdr.arp.proto_type) {
            (0x0001, 0x0800) : parse_arp_ipv4;
            default: accept;
        }
    }

    state parse_arp_ipv4 {
        pkt.extract(hdr.arp_ipv4);
        transition accept;
    }
#endif



    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            0 : parse_p4ml;
            default : accept;
        }
    }

#ifdef _ENABLE_CONT3_
    state parse_p4ml {
        pkt.extract(hdr.p4ml);
        ig_md.mdata.setValid();
        // ig_md.mdata.isMyPAS = hdr.p4ml.PAS;

        transition select(hdr.p4ml.dataIndex, hdr.p4ml.isACK, hdr.p4ml.quantization_level, hdr.p4ml.aggregation_complete_flag, hdr.ipv4.ttl) {
            (0, 1, 1, 1, 0)     : use_first_p4ml_agtr_index_parse_global_1bit;
            (1, 1, 1, 1, 0)     : use_second_p4ml_agtr_index_parse_global_1bit;
            (0, 0, 1, 0, 0)     : use_first_p4ml_agtr_index_recirculate_1bit;
            (1, 0, 1, 0, 0)     : use_second_p4ml_agtr_index_recirculate_1bit;
            (1, 0, 1, 1, 0)     : use_second_p4ml_agtr_index_recirculate_1bit_complete;
            (0, _, 3, _, 0)     : use_first_p4ml_agtr_index_recirculate;
            (1, _, 3, _, 0)     : use_second_p4ml_agtr_index_recirculate;
            default : accept;
        }
    }
#else
    state parse_p4ml {
        pkt.extract(hdr.p4ml);
        ig_md.mdata.setValid();
        // ig_md.mdata.isMyPAS = hdr.p4ml.PAS;

        transition select(hdr.p4ml.dataIndex, hdr.p4ml.isACK, hdr.p4ml.quantization_level, hdr.p4ml.aggregation_complete_flag) {
            (0, 1, 1, 1)     : use_first_p4ml_agtr_index_parse_global_1bit;
            (1, 1, 1, 1)     : use_second_p4ml_agtr_index_parse_global_1bit;
            (0, 0, 1, 0)     : use_first_p4ml_agtr_index_recirculate_1bit;
            (1, 0, 1, 0)     : use_second_p4ml_agtr_index_recirculate_1bit;
            (1, 0, 1, 1)     : use_second_p4ml_agtr_index_recirculate_1bit_complete;
            (0, _, 3, _)     : use_first_p4ml_agtr_index_recirculate;
            (1, _, 3, _)     : use_second_p4ml_agtr_index_recirculate;
            default : accept;
        }
    }
#endif

    state use_first_p4ml_agtr_index_parse_global_1bit {
        pkt.extract(hdr.p4ml_agtr_index);
        pkt.extract(hdr.p4ml_agtr_index_useless);
        pkt.extract(hdr.p4ml_entries_global_1bit_useless);
        pkt.extract(hdr.p4ml_entries_global_1bit);
        
        transition accept;
    }

    state use_second_p4ml_agtr_index_parse_global_1bit {
        pkt.extract(hdr.p4ml_agtr_index_useless2);
        pkt.extract(hdr.p4ml_agtr_index);
        pkt.extract(hdr.p4ml_entries_global_1bit_useless);
        pkt.extract(hdr.p4ml_entries_global_1bit);
        transition accept;
    }

    state use_first_p4ml_agtr_index_recirculate_1bit {
        pkt.extract(hdr.p4ml_agtr_index);
        pkt.extract(hdr.p4ml_agtr_index_useless);
        pkt.extract(hdr.p4ml_entries);
        pkt.extract(hdr.p4ml_entries_useless1);
        transition accept;
    }

    state use_second_p4ml_agtr_index_recirculate_1bit {
        pkt.extract(hdr.p4ml_agtr_index_useless2);
        pkt.extract(hdr.p4ml_agtr_index);
        pkt.extract(hdr.p4ml_entries_useless);
        pkt.extract(hdr.p4ml_entries);
        transition accept;
    }

    state use_second_p4ml_agtr_index_recirculate_1bit_complete {
        pkt.extract(hdr.p4ml_agtr_index_useless2);
        pkt.extract(hdr.p4ml_agtr_index);
        pkt.extract(hdr.p4ml_entries_global_1bit_useless);
        pkt.extract(hdr.p4ml_entries);
        transition accept;
    }

// Resubmit_Flag == 0
    // state parse_p4ml_agtr_index {
    //     pkt.extract(hdr.p4ml_agtr_index);
    //     transition skip_second_p4ml_agtr_index;
    // }

    // state skip_second_p4ml_agtr_index {
    //     pkt.advance(16);
    //     transition parse_entry;
    // }

    state parse_entry {
        pkt.extract(hdr.p4ml_entries);
        transition accept;
    }

// Resubmit_Flag == 1
    // state skip_first_p4ml_agtr_index {
    //     pkt.advance(16);
    //     transition parse_p4ml_agtr_index2;
    // }

    // state parse_p4ml_agtr_index2{
    //     pkt.extract(hdr.p4ml_agtr_index);
    //     transition skip_first_p4ml_entries;
    // }

    // state skip_first_p4ml_entries{
    //     pkt.advance(1024);
    //     transition parse_entry;
    // }

// Recirculation 1
    state use_first_p4ml_agtr_index_recirculate {
        pkt.extract(hdr.p4ml_agtr_index);
        // [I]
        transition useless_second_p4ml_agtr_index_recirculate;
    } 


    state useless_second_p4ml_agtr_index_recirculate {
        pkt.extract(hdr.p4ml_agtr_index_useless); 
        transition parse_entry; 
    }   // [I][I_useless][E]

// Recirculation 2
    state use_second_p4ml_agtr_index_recirculate {
        pkt.extract(hdr.p4ml_agtr_index_useless2); 
        transition parse_p4ml_agtr_index_recirculate;
    }   // [I_useless2]

    state parse_p4ml_agtr_index_recirculate {
        pkt.extract(hdr.p4ml_agtr_index); 
        transition select(hdr.p4ml.quantization_level){
          0x3   : parse_entry2;       // [I_useless2][I][(128B)E_useless][(128B)E]
          0x1   : parse_entry2_1bit;  // [I_useless2][I][(32B)E_useless ][(128B)E]
          default : accept;
        }
    }   // [I_useless2][I]

    state parse_entry2 {
        pkt.extract(hdr.p4ml_entries_useless); 
        transition parse_entry; 
    }   // [I_useless2][I][(128B)E_useless][(128B)E]

    state parse_entry2_1bit {
        pkt.extract(hdr.p4ml_entries_1bit_useless2); 
        transition parse_entry; 
    }   // [I_useless2][I][(32B)E_useless ][(128B)E]



}

control IngressDeparser(
    packet_out pkt,
    inout header_t hdr,
    in ingress_metadata_t ig_md,
    in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr) {

    Checksum() ipv4_checksum;
    Resubmit() resubmit;
    // Mirror() mirror;

    apply {

        // if(ig_intr_md_for_dprsr.mirror_type == 1 ){
        //     mirror.emit<ing_port_mirror_h>(
        //         ig_md.mdata.session_id,
        //         {
        //             ig_md.mdata.header_type,
        //             ig_md.mdata.header_info,
        //             ig_md.mdata.ingress_port,   //FIXME:
        //             ig_md.mdata.padding2        //FIXME:
        //         }
        //         );
        // }

        pkt.emit(ig_md.mdata);
        pkt.emit(hdr);


    }
}

parser EgressParser(
    packet_in pkt,
    out header_t hdr,
    out egress_metadata_t eg_md,
    out egress_intrinsic_metadata_t eg_intr_md) {



        // // MIRROR
        // inthdr_h inthdr;

        // state start {
        //     pkt.extract(eg_intr_md);
        //     transition accept;
            
        // }





    // state start {
    //     pkt.extract(eg_intr_md);
    //     // pkt.extract(eg_md.mdata);

    //     inthdr = pkt.lookahead<inthdr_h>();

    //     transition select(inthdr.header_type, inthdr.header_info){
    //         (0xC, 1) : parse_ing_port_mirror;
    //         default : parse_bridge;
    //     }
    //     // transition parse_ethernet;  
    // }

    // state parse_ing_port_mirror {
    //     pkt.extract(eg_md.ing_port_mirror);
    //     pkt.extract(hdr.ethernet);
    //     transition accept;
    // }

    // state parse_mdata {
    //     pkt.extract(eg_md.mdata);
    //     transition parse_ethernet;
    // }


    // state parse_bridge {
    //     pkt.extract(eg_md.bridge);
    //     transition parse_mdata;
    // }



    // state parse_ing_port_mirror {
    //     pkt.extract(eg_md.ing_port_mirror);
    //     transition accept;
    // }

    // state parse_mdata {
    //     pkt.extract(eg_md.mdata);
    //     transition parse_ethernet;
    // }



// ORIGINAL
    state start {
        pkt.extract(eg_intr_md);
        pkt.extract(eg_md.mdata);

        transition parse_ethernet;  
    }

    state parse_ethernet {
        
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
#ifdef _ARP_
            16w0x0806 : parse_arp;
#endif
            0x0800 : parse_ipv4;
            default : accept;
        }
    }


#ifdef _ARP_
    state parse_arp {
        pkt.extract(hdr.arp);
        transition select(hdr.arp.hw_type, hdr.arp.proto_type) {
            (0x0001, 0x0800) : parse_arp_ipv4;
            default: accept;
        }
    }

    state parse_arp_ipv4 {
        pkt.extract(hdr.arp_ipv4);
        transition accept;
    }
#endif

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            0 : parse_p4ml;
            default : accept;
        }
    }


    state parse_p4ml {
        pkt.extract(hdr.p4ml);
        transition select(eg_intr_md.egress_port, hdr.p4ml.quantization_level, hdr.p4ml.aggregation_complete_flag, hdr.p4ml.isACK) {
          (196, 3, 0, _)     : use_first_p4ml_agtr_index_recirculate;
          (_,   3, 0, _)     : use_second_p4ml_agtr_index_recirculate;
          (196, 1, 0, 0)     : use_first_p4ml_agtr_index_recirculate_1bit;
          (_,   1, 0, 0)     : use_second_p4ml_agtr_index_recirculate_1bit;
          (_,   1, 1, 0)     : use_second_p4ml_agtr_index_recirculate_1bit_complete;
          (196, 1, 1, 1)     : use_first_p4ml_agtr_index_parse_global_1bit;
          (_,   1, 1, 1)     : use_second_p4ml_agtr_index_parse_global_1bit;
          default  : accept;
        }
    }

    state use_first_p4ml_agtr_index_parse_global_1bit {
        pkt.extract(hdr.p4ml_agtr_index);
        pkt.extract(hdr.p4ml_agtr_index_useless);
        pkt.extract(hdr.p4ml_entries_global_1bit);
        pkt.extract(hdr.p4ml_entries_global_1bit_useless1);
        
        transition accept;
    }

    state use_second_p4ml_agtr_index_parse_global_1bit {
        pkt.extract(hdr.p4ml_agtr_index_useless2);
        pkt.extract(hdr.p4ml_agtr_index);
        pkt.extract(hdr.p4ml_entries_global_1bit_useless);
        pkt.extract(hdr.p4ml_entries_global_1bit);
        
        transition accept;
    }

    state use_first_p4ml_agtr_index_recirculate_1bit {
        pkt.extract(hdr.p4ml_agtr_index);
        pkt.extract(hdr.p4ml_agtr_index_useless);
        pkt.extract(hdr.p4ml_entries);
        pkt.extract(hdr.p4ml_entries_useless1);
        transition accept;
    }

    state use_second_p4ml_agtr_index_recirculate_1bit {
        pkt.extract(hdr.p4ml_agtr_index_useless2);
        pkt.extract(hdr.p4ml_agtr_index);
        pkt.extract(hdr.p4ml_entries_useless);
        pkt.extract(hdr.p4ml_entries);
        transition accept;
    }

    state use_second_p4ml_agtr_index_recirculate_1bit_complete {
        pkt.extract(hdr.p4ml_agtr_index_useless2);
        pkt.extract(hdr.p4ml_agtr_index);
        pkt.extract(hdr.p4ml_entries_global_1bit_useless);
        pkt.extract(hdr.p4ml_entries);
        transition accept;
    }






    state parse_entry {
        pkt.extract(hdr.p4ml_entries);
        transition accept;
    }


// Recirculation 1
    state use_first_p4ml_agtr_index_recirculate {
        pkt.extract(hdr.p4ml_agtr_index);
        transition useless_second_p4ml_agtr_index_recirculate;
    }   // [I]

    state useless_second_p4ml_agtr_index_recirculate {
        pkt.extract(hdr.p4ml_agtr_index_useless);
        transition parse_entry1; 
    }   // [I][I_useless]

    state parse_entry1 {
        pkt.extract(hdr.p4ml_entries);
        transition parse_entry_useless1;
    }   // [I][I_useless][E]


    state parse_entry_useless1 {
        pkt.extract(hdr.p4ml_entries_useless1);
        transition accept;
    }   // [I][I_useless][E(128B)][E_useless(128B)]

// Recirculation 2
    state use_second_p4ml_agtr_index_recirculate {
        pkt.extract(hdr.p4ml_agtr_index_useless2); 
        transition parse_p4ml_agtr_index_recirculate;
    }   // [I_useless2]

    state parse_p4ml_agtr_index_recirculate {
        pkt.extract(hdr.p4ml_agtr_index); 
        transition select(hdr.p4ml.quantization_level){
          0x3   : parse_entry2;       // [I_useless2][I][(128B)E_useless][(128B)E]
          0x1   : parse_entry2_1bit;  // [I_useless2][I][(32B)E_useless ][(128B)E]
          default : accept;
        }
    }   // [I_useless2][I]

    state parse_entry2 {
        pkt.extract(hdr.p4ml_entries_useless); 
        transition parse_entry; 
    }   // [I_useless2][I][(128B)E_useless][(128B)E]

    state parse_entry2_1bit {
        pkt.extract(hdr.p4ml_entries_1bit_useless2); 
        transition parse_entry; 
    }   // [I_useless2][I][(32B)E_useless ][(128B)E]



}

control EgressDeparser(
    packet_out pkt,
    inout header_t hdr,
    in egress_metadata_t eg_md,
    in egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md) {

    Checksum() ipv4_checksum;

    apply {
        pkt.emit(hdr);
        // pkt.emit(hdr.ethernet);
        // pkt.emit(hdr.ipv4);
        // pkt.emit(hdr.p4ml);
        // pkt.emit(hdr.p4ml_agtr_index_useless2); // Second-1
        // pkt.emit(hdr.p4ml_agtr_index); // First-1 / Second-2
        // pkt.emit(hdr.p4ml_agtr_index_useless); //First-2

        // pkt.emit(hdr.p4ml_entries_1bit_useless2); // First-1
        // pkt.emit(hdr.p4ml_entries_1bit); // First-1
        
        // pkt.emit(hdr.p4ml_entries_useless); // Second-1
        // pkt.emit(hdr.p4ml_entries); // First-1, Second-2
        // pkt.emit(hdr.p4ml_entries_useless1); // First-2


    }
}

#endif 