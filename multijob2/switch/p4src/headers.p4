#ifndef _HEADERS_
#define _HEADERS_

#define MAX_ENTRIES_PER_PACKET 32

typedef bit<32> ipv4_addr_t;
typedef bit<48> mac_addr_t;
typedef bit<8> ip_protocol_t;

#ifdef _ARP_
header arp_h {
    bit<16>       hw_type;
    bit<16>       proto_type;
    bit<8>        hw_addr_len;
    bit<8>        proto_addr_len;
    bit<16>  opcode;
}

header arp_ipv4_h {
    bit<48>  src_hw_addr;
    bit<32>  src_proto_addr;
    bit<48>  dst_hw_addr;
    bit<32>  dst_proto_addr;
}

#endif

header ethernet_t {
    mac_addr_t dstAddr;
    mac_addr_t srcAddr;
    bit<16>    etherType;
}

header ipv4_t {
    bit<4>        version;
    bit<4>        ihl;
    bit<6>        dscp;
    bit<2>        ecn;
    bit<16>       totalLen;
    bit<16>       identification;
    bit<3>        flags;
    bit<13>       fragOffset;
    bit<8>        ttl;
    ip_protocol_t protocol;
    bit<16>       hdr_checksum;
    ipv4_addr_t   srcAddr;
    ipv4_addr_t   dstAddr;
}

header udp_t {
    bit<16>     srcPort;
    bit<16>     dstPort;
    bit<16>     length_;
    bit<16>     checksum;
}

header p4ml_t { // 10B or 11B (CEINA)
    bit<32>     bitmap;
    bit<8>      agtr_time;
    bit<1>      overflow;
    bit<2>      PSIndex;
    bit<1>      dataIndex;
    bit<1>      ECN;
    bit<1>      isResend;
    bit<1>      isWCollision;  
    bit<1>      isACK;
    int<32>     PAS; // priority_appid_seqnum appIDandSeqNum;
#ifdef _CEINA_    
    bit<3>      padding;
    bit<1>      evicted;
    bit<1>      clear;
    bit<1>      aggregation_complete_flag;
    bit<2>      quantization_level; // 1 if 1bit, 2 if 32bit
#endif

}



header p4ml_agtr_index_t {
    bit<16>     agtr;
}


// header bg_p4ml_t {
//     bit<64>     key;     
//     bit<32>     len_tensor;     
//     bit<32>     bitmap;   
//     bit<8>      agtr_time;   
//     bit<4>      reserved;      
//     bit<1>      ECN;  
//     bit<1>      isResend;  
//     bit<1>      isSWCollision;  
//     bit<1>      isACK;  
//     bit<16>     agtr; 
//     bit<32>     appIDandSeqNum; 
// }

header entry_t {
    bit<32>     data0 ;      
    bit<32>     data1 ;      
    bit<32>     data2 ;      
    bit<32>     data3 ;      
    bit<32>     data4 ;      
    bit<32>     data5 ;      
    bit<32>     data6 ;      
    bit<32>     data7 ;      
    bit<32>     data8 ;      
    bit<32>     data9 ;      
    bit<32>     data10;      
    bit<32>     data11;      
    bit<32>     data12;      
    bit<32>     data13;      
    bit<32>     data14;      
    bit<32>     data15;      
    bit<32>     data16;      
    bit<32>     data17;      
    bit<32>     data18;      
    bit<32>     data19;      
    bit<32>     data20;      
    bit<32>     data21;      
    bit<32>     data22;      
    bit<32>     data23;      
    bit<32>     data24;      
    bit<32>     data25;      
    bit<32>     data26;      
    bit<32>     data27;      
    bit<32>     data28;      
    bit<32>     data29;      
    bit<32>     data30;   
    bit<32>     data31;
}


header p4ml_meta_t { // 
    bit<32>     bitmap                   ; // 4B
    int<32>     isMyPAS                  ; // isMyAppIDandMyCurrentSeq ; // 2B
    bit<32>     isAggregate              ; // 4B // 10
    bit<8>      agtr_time                ; // 1B 
    bit<32>     integrated_bitmap        ; // 4B
    bit<8>      current_agtr_time        ; // 1B 
    bit<32>     agtr_index 	          	 ; // 4B // 20
    bit<32>     isDrop                   ; // 4B
    bit<1>      inside_appID_and_Seq     ; //   
    bit<1>      value_one                ; //  
    bit<1>      preemption               ; // 
    bit<5>      padding                  ; // 1B
    bit<8>      agtr_complete            ; // 1B
    bit<16>     qdepth                   ; // 2B  
    bit<8>      seen_bitmap0		     ; // 1B
    bit<8>      seen_isAggregate 	     ; // 1B // 30B
    bit<32>     is_ecn                   ; // 4B // 34B

}

header p4ml_constant_t {
    bit<32>     bitmap;
    bit<8>      agtr_time;
}

header entry_1bit_t {
    bit<8>      data0_1bit;
    bit<8>      data1_1bit;
    bit<8>      data2_1bit;
    bit<8>      data3_1bit;
    bit<8>      data4_1bit;
    bit<8>      data5_1bit;
    bit<8>      data6_1bit;
    bit<8>      data7_1bit;
    bit<8>      data8_1bit;
    bit<8>      data9_1bit;
    bit<8>      data10_1bit;
    bit<8>      data11_1bit;
    bit<8>      data12_1bit;
    bit<8>      data13_1bit;
    bit<8>      data14_1bit;
    bit<8>      data15_1bit;
    bit<8>      data16_1bit;
    bit<8>      data17_1bit;
    bit<8>      data18_1bit;
    bit<8>      data19_1bit;
    bit<8>      data20_1bit;
    bit<8>      data21_1bit;
    bit<8>      data22_1bit;
    bit<8>      data23_1bit;
    bit<8>      data24_1bit;
    bit<8>      data25_1bit;
    bit<8>      data26_1bit;
    bit<8>      data27_1bit;
    bit<8>      data28_1bit;
    bit<8>      data29_1bit;
    bit<8>      data30_1bit;
    bit<8>      data31_1bit;
}

header entry_global_1bit_t {
    bit<8>      data0_1bit;
    bit<8>      data1_1bit;
    bit<8>      data2_1bit;
    bit<8>      data3_1bit;
    bit<8>      data4_1bit;
    bit<8>      data5_1bit;
    bit<8>      data6_1bit;
    bit<8>      data7_1bit;
    bit<8>      data8_1bit;
    bit<8>      data9_1bit;
    bit<8>      data10_1bit;
    bit<8>      data11_1bit;
    bit<8>      data12_1bit;
    bit<8>      data13_1bit;
    bit<8>      data14_1bit;
    bit<8>      data15_1bit;
    bit<8>      data16_1bit;
    bit<8>      data17_1bit;
    bit<8>      data18_1bit;
    bit<8>      data19_1bit;
    bit<8>      data20_1bit;
    bit<8>      data21_1bit;
    bit<8>      data22_1bit;
    bit<8>      data23_1bit;
    bit<8>      data24_1bit;
    bit<8>      data25_1bit;
    bit<8>      data26_1bit;
    bit<8>      data27_1bit;
    bit<8>      data28_1bit;
    bit<8>      data29_1bit;
    bit<8>      data30_1bit;
    bit<8>      data31_1bit;
}

// header entry_global_1bit_t {
//     bit<32>      data0_global_1bit;
//     bit<32>      data1_global_1bit;
// }

header resubmit_metadata_t {
    bit<8>      agtr_time;
}
struct header_t {
    resubmit_metadata_t resubmit_meta;
    ethernet_t          ethernet;

#ifdef _ARP_
    arp_h        arp;
    arp_ipv4_h   arp_ipv4;
#endif

    ipv4_t              ipv4;
    udp_t               udp;
    p4ml_t              p4ml;                       // 10B                       
    p4ml_agtr_index_t   p4ml_agtr_index_useless2;   // 2B
    p4ml_agtr_index_t   p4ml_agtr_index;            // 2B
    p4ml_agtr_index_t   p4ml_agtr_index_useless;    // 2B
#ifdef _CEINA_    
    entry_global_1bit_t p4ml_entries_global_1bit_useless;          /// 32B  
    entry_global_1bit_t p4ml_entries_global_1bit;          /// 32B  
    entry_global_1bit_t p4ml_entries_global_1bit_useless1;          /// 32B  
    entry_1bit_t        p4ml_entries_1bit_useless2; /// 128B
    entry_1bit_t        p4ml_entries_1bit;          /// 128B  
#endif
    entry_t             p4ml_entries_useless;       // 128B
    entry_t             p4ml_entries;               // 128B
    entry_t             p4ml_entries_useless1;      // 128B
    // bg_p4ml_t           p4ml_bg;     
    
}


// struct metadata_t {
//     p4ml_meta_t         mdata;
//     p4ml_constant_t     p4ml_constant;
// }

struct ingress_metadata_t{
    p4ml_meta_t  mdata;
    // p4ml_constant_t     p4ml_constant;
}

struct egress_metadata_t{
    p4ml_meta_t  mdata;
}

#endif