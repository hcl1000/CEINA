// Worker DPDK TX
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <memory.h>
#include <zlib.h>
#include <sys/queue.h>

#include <rte_memory.h>
#include <rte_mbuf.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_ethdev.h>
#include <rte_udp.h>
#include <rte_ip.h>
#include <rte_version.h>
#include <rte_log.h>

#define RTE_LOGTYPE_WORKER RTE_LOGTYPE_USER1
#define LOG_LEVEL 5
// 6 NOTICE 7 INFO 8 DEBUG
int log_level = LOG_LEVEL; ////FIXME

#define CRCPOLY_LE 0xedb88320
#define LOSS_RECOVERY_ENABLE 1
#define L2_ENABLE 1
#define CC_ENABLE 1
#define LOSS_CC_ENABLE 1
#define ATP 0
#define CHANGE_AGTR_ENABLE ATP
#define LAYER_CONTROL 2 % (ATP+1)

// #define APP_ID 0x9 // 1bit job1
// #define APP_ID 0xA // 1bit job2
// #define APP_ID 0xB // 1bit job3
#define APP_ID 0x1
#define TRAINING_TIME 10 // COMP
#define TRAINING_TIME 2.5 // COMM
#define TRAINING_TIME 0

#define IP_DEFTTL 0 // Job1_W1:0, Job1_W2 : 2   // Job2_W1:2, Job2_W2:0
#define PORT_ID 1 
#define NUM_WORKER 2
#define WORKER_ID 1

// #include <window_manager.h>
#define TX_PACKET_LENGTH 305
#define MAX_AGTR_SIZE 1000 // Actually 2*MAX_AGTR_SIZE used in switch because we uses two entries
#define INITIAL_WINDOW_SIZE 1
#define WINDOW_SIZE 128
#define BURST_SIZE 128
#define TX_BURST_SIZE 128

#define TOTAL_GRADIENTS 640000000

#define TEST_SECOND 2000000000 // 5s
#define TIMEOUT_NS 50000 // 50us

#define NUM_ROUND 1
#define ENTER 1

#if APP_ID > 0x8
#define TOTAL_PACKET TOTAL_GRADIENTS / 64 / 8
#define QUAN_LEVEL 1 << 6
#else
#define TOTAL_PACKET TOTAL_GRADIENTS / 64
#define QUAN_LEVEL 0
#endif

#define MODEL_SIZE 1 << 5 // 1bit job - num1
#define LAYER_INDEX 1 << 4 // 1bit job - num1


#define RX_RING_SIZE 2048
#define TX_RING_SIZE 1024

#define RX_NUM_MBUFS 8192
#define NUM_MBUFS 8192
#define MBUF_BUF_SIZE 1152 // RTE_MBUF_DEFAULT_BUF_SIZE : 2048
#define MBUF_CACHE_SIZE 250


#define MAX_BYTES TX_BURST_SIZE * TX_PACKET_LENGTH


#define UDP_SRC_PORT 6666
#define UDP_DST_PORT 6666
#define IP_VERSION 0x40
#define IP_HDRLEN 0x05 /* default IP header length == five 32-bits words. */
#define IP_VHL_DEF (IP_VERSION | IP_HDRLEN)

#if RTE_BYTE_ORDER == RTE_BIG_ENDIAN
#define RTE_BE_TO_CPU_16(be_16_v) (be_16_v)
#define RTE_CPU_TO_BE_16(cpu_16_v) (cpu_16_v)
#else
#define RTE_BE_TO_CPU_16(be_16_v) \
	(uint16_t)((((be_16_v)&0xFF) << 8) | ((be_16_v) >> 8))
#define RTE_CPU_TO_BE_16(cpu_16_v) \
	(uint16_t)((((cpu_16_v)&0xFF) << 8) | ((cpu_16_v) >> 8))
#endif

uint64_t DST_MAC;
uint32_t IP_SRC_ADDR, IP_DST_ADDR;

static const struct rte_eth_conf port_conf_default = {
	// .rxmode = {.max_rx_pkt_len = RTE_ETHER_MAX_LEN}};
	.rxmode = {.max_lro_pkt_size = RTE_ETHER_MAX_LEN}};

static struct rte_ceina_hdr
{
	rte_be32_t bitmap;
	uint8_t agtr_time;
	uint8_t flags;
	rte_be32_t PAS;
	// rte_be16_t appid;
	// rte_be16_t seq_num;
	uint8_t quantization_flags;
	rte_be16_t agtr_idx;
	rte_be16_t agtr_idx2;
} __attribute__((__packed__)) pkt_ceina_hdr;

static struct rte_ceina_entry1_hdr_1bit // one data has two 1-bit values
{
	uint8_t data[128];

} __attribute__((__packed__)) pkt_ceina_entry1_1bit;

static struct rte_ceina_entry2_hdr_1bit // one data has two 1-bit values
{
	uint8_t data[128];

} __attribute__((__packed__)) pkt_ceina_entry2_1bit;

static struct rte_ceina_entry1_hdr_1bit_global 
{
	uint8_t data[32];

} __attribute__((__packed__)) pkt_ceina_entry1_1bit_global;

static struct rte_ceina_entry2_hdr_1bit_global 
{
	uint8_t data[32];

} __attribute__((__packed__)) pkt_ceina_entry2_1bit_global;



static struct rte_ceina_entry1_hdr
{
	rte_be32_t data1;
	rte_be32_t data2;
	rte_be32_t data3;
	rte_be32_t data4;
	rte_be32_t data5;
	rte_be32_t data6;
	rte_be32_t data7;
	rte_be32_t data8;
	rte_be32_t data9;
	rte_be32_t data10;
	rte_be32_t data11;
	rte_be32_t data12;
	rte_be32_t data13;
	rte_be32_t data14;
	rte_be32_t data15;
	rte_be32_t data16;
	rte_be32_t data17;
	rte_be32_t data18;
	rte_be32_t data19;
	rte_be32_t data20;
	rte_be32_t data21;
	rte_be32_t data22;
	rte_be32_t data23;
	rte_be32_t data24;
	rte_be32_t data25;
	rte_be32_t data26;
	rte_be32_t data27;
	rte_be32_t data28;
	rte_be32_t data29;
	rte_be32_t data30;
	rte_be32_t data31;
	rte_be32_t data32;
} __attribute__((__packed__)) pkt_ceina_entry1;

static struct rte_ceina_entry2_hdr
{
	rte_be32_t data1;
	rte_be32_t data2;
	rte_be32_t data3;
	rte_be32_t data4;
	rte_be32_t data5;
	rte_be32_t data6;
	rte_be32_t data7;
	rte_be32_t data8;
	rte_be32_t data9;
	rte_be32_t data10;
	rte_be32_t data11;
	rte_be32_t data12;
	rte_be32_t data13;
	rte_be32_t data14;
	rte_be32_t data15;
	rte_be32_t data16;
	rte_be32_t data17;
	rte_be32_t data18;
	rte_be32_t data19;
	rte_be32_t data20;
	rte_be32_t data21;
	rte_be32_t data22;
	rte_be32_t data23;
	rte_be32_t data24;
	rte_be32_t data25;
	rte_be32_t data26;
	rte_be32_t data27;
	rte_be32_t data28;
	rte_be32_t data29;
	rte_be32_t data30;
	rte_be32_t data31;
	rte_be32_t data32;
} __attribute__((__packed__)) pkt_ceina_entry2;

static struct rte_ipv4_hdr pkt_ip_hdr; /**< IP header of transmitted packets. */
struct rte_ether_addr my_addr;		   // SRC MAC address of NIC

struct rte_mempool *mbuf_pool;
struct rte_mempool *tx_mbuf_pool;
struct rte_mempool *tx_mbuf_pool2;
struct rte_mempool *tx_mbuf_pool3;
struct rte_eth_dev_tx_buffer *tx_buffer;




static struct window_manager
{
	bool *isACKed;
	bool *isSent;
	uint32_t total_ACK;
	uint32_t last_ACK;
};


uint32_t string_to_ip(char *);
uint64_t string_to_mac(char *);
// static void send_packet(uint16_t);
static void exit_stats(int);



// convert a quad-dot IP string to uint32_t IP address
uint32_t string_to_ip(char *s)
{
	unsigned char a[4];
	int rc = sscanf(s, "%hhd.%hhd.%hhd.%hhd", a + 0, a + 1, a + 2, a + 3);
	if (rc != 4)
	{
		fprintf(stderr, "bad source IP address format. Use like: -s 198.19.111.179\n");
		exit(1);
	}

	return (uint32_t)(a[0]) << 24 |
		   (uint32_t)(a[1]) << 16 |
		   (uint32_t)(a[2]) << 8 |
		   (uint32_t)(a[3]);
}

// convert six colon separated hex bytes string to uint64_t Ethernet MAC address
uint64_t string_to_mac(char *s)
{
	unsigned char a[6];
	int rc = sscanf(s, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
					a + 0, a + 1, a + 2, a + 3, a + 4, a + 5);
	if (rc != 6)
	{
		fprintf(stderr, "bad MAC address format. Use like: -m 0a:38:ca:f6:f3:20\n");
		exit(1);
	}

	return (uint64_t)(a[0]) << 40 |
		   (uint64_t)(a[1]) << 32 |
		   (uint64_t)(a[2]) << 24 |
		   (uint64_t)(a[3]) << 16 |
		   (uint64_t)(a[4]) << 8 |
		   (uint64_t)(a[5]);
}

static void setup_pkt_headers_1bit(struct rte_ipv4_hdr *ip_hdr,
							  		struct rte_ceina_hdr *ceina_hdr,
							  		struct rte_ceina_entry1_hdr_1bit *entry1_hdr_1bit,
							  		struct rte_ceina_entry2_hdr_1bit *entry2_hdr_1bit,
							  		uint16_t pkt_data_len)
{
	uint16_t *ptr16;
	uint32_t ip_cksum;
	uint16_t pkt_len;

	// Initialize IP header.
	pkt_len = (uint16_t)(pkt_data_len + sizeof(struct rte_ipv4_hdr));
	ip_hdr->version_ihl = IP_VHL_DEF;
	ip_hdr->type_of_service = 0;
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live = IP_DEFTTL;
	ip_hdr->next_proto_id = 0;
	ip_hdr->packet_id = 0;
	ip_hdr->total_length = RTE_CPU_TO_BE_16(pkt_len);
	ip_hdr->src_addr = rte_cpu_to_be_32(IP_SRC_ADDR);
	ip_hdr->dst_addr = rte_cpu_to_be_32(IP_DST_ADDR);

	// Compute IP header checksum.
	ptr16 = (unaligned_uint16_t *)ip_hdr;
	ip_cksum = 0;
	ip_cksum += ptr16[0];
	ip_cksum += ptr16[1];
	ip_cksum += ptr16[2];
	ip_cksum += ptr16[3];
	ip_cksum += ptr16[4];
	ip_cksum += ptr16[6];
	ip_cksum += ptr16[7];
	ip_cksum += ptr16[8];
	ip_cksum += ptr16[9];

	// Reduce 32 bit checksum to 16 bits and complement it.
	ip_cksum = ((ip_cksum & 0xFFFF0000) >> 16) +
			   (ip_cksum & 0x0000FFFF);
	if (ip_cksum > 65535)
		ip_cksum -= 65535;
	ip_cksum = (~ip_cksum) & 0x0000FFFF;
	if (ip_cksum == 0)
		ip_cksum = 0xFFFF;
	ip_hdr->hdr_checksum = (uint16_t)ip_cksum;

	for (int i=0; i < 128; i++){
		entry1_hdr_1bit->data[i]  = 0x11;	
		entry2_hdr_1bit->data[i]  = 0x11;
	}
}


// setup packet IP headers
static void setup_pkt_headers(struct rte_ipv4_hdr *ip_hdr,
							  struct rte_ceina_hdr *ceina_hdr,
							  struct rte_ceina_entry1_hdr *entry1_hdr,
							  struct rte_ceina_entry2_hdr *entry2_hdr,
							  uint16_t pkt_data_len)
{
	uint16_t *ptr16;
	uint32_t ip_cksum;
	uint16_t pkt_len;

	// Initialize IP header.
	pkt_len = (uint16_t)(pkt_data_len + sizeof(struct rte_ipv4_hdr));
	ip_hdr->version_ihl = IP_VHL_DEF;
	ip_hdr->type_of_service = 0;
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live = IP_DEFTTL;
	ip_hdr->next_proto_id = 0;
	ip_hdr->packet_id = 0;
	ip_hdr->total_length = RTE_CPU_TO_BE_16(pkt_len);
	ip_hdr->src_addr = rte_cpu_to_be_32(IP_SRC_ADDR);
	ip_hdr->dst_addr = rte_cpu_to_be_32(IP_DST_ADDR);

	// Compute IP header checksum.
	ptr16 = (unaligned_uint16_t *)ip_hdr;
	ip_cksum = 0;
	ip_cksum += ptr16[0];
	ip_cksum += ptr16[1];
	ip_cksum += ptr16[2];
	ip_cksum += ptr16[3];
	ip_cksum += ptr16[4];
	ip_cksum += ptr16[6];
	ip_cksum += ptr16[7];
	ip_cksum += ptr16[8];
	ip_cksum += ptr16[9];

	// Reduce 32 bit checksum to 16 bits and complement it.
	ip_cksum = ((ip_cksum & 0xFFFF0000) >> 16) +
			   (ip_cksum & 0x0000FFFF);
	if (ip_cksum > 65535)
		ip_cksum -= 65535;
	ip_cksum = (~ip_cksum) & 0x0000FFFF;
	if (ip_cksum == 0)
		ip_cksum = 0xFFFF;
	ip_hdr->hdr_checksum = (uint16_t)ip_cksum;

	entry1_hdr->data1 = rte_cpu_to_be_32(1);
	entry1_hdr->data2 = rte_cpu_to_be_32(2);
	entry1_hdr->data3 = rte_cpu_to_be_32(3);
	entry1_hdr->data4 = rte_cpu_to_be_32(4);
	entry1_hdr->data5 = rte_cpu_to_be_32(5);
	entry1_hdr->data6 = rte_cpu_to_be_32(6);
	entry1_hdr->data7 = rte_cpu_to_be_32(7);
	entry1_hdr->data8 = rte_cpu_to_be_32(8);
	entry1_hdr->data9 = rte_cpu_to_be_32(9);
	entry1_hdr->data10 = rte_cpu_to_be_32(10);
	entry1_hdr->data11 = rte_cpu_to_be_32(11);
	entry1_hdr->data12 = rte_cpu_to_be_32(12);
	entry1_hdr->data13 = rte_cpu_to_be_32(13);
	entry1_hdr->data14 = rte_cpu_to_be_32(14);
	entry1_hdr->data15 = rte_cpu_to_be_32(15);
	entry1_hdr->data16 = rte_cpu_to_be_32(16);
	entry1_hdr->data17 = rte_cpu_to_be_32(17);
	entry1_hdr->data18 = rte_cpu_to_be_32(18);
	entry1_hdr->data19 = rte_cpu_to_be_32(19);
	entry1_hdr->data20 = rte_cpu_to_be_32(20);
	entry1_hdr->data21 = rte_cpu_to_be_32(21);
	entry1_hdr->data22 = rte_cpu_to_be_32(22);
	entry1_hdr->data23 = rte_cpu_to_be_32(23);
	entry1_hdr->data24 = rte_cpu_to_be_32(24);
	entry1_hdr->data25 = rte_cpu_to_be_32(25);
	entry1_hdr->data26 = rte_cpu_to_be_32(26);
	entry1_hdr->data27 = rte_cpu_to_be_32(27);
	entry1_hdr->data28 = rte_cpu_to_be_32(28);
	entry1_hdr->data29 = rte_cpu_to_be_32(29);
	entry1_hdr->data30 = rte_cpu_to_be_32(30);
	entry1_hdr->data31 = rte_cpu_to_be_32(31);
	entry1_hdr->data32 = rte_cpu_to_be_32(32);

	entry2_hdr->data1 = rte_cpu_to_be_32(-1);
	entry2_hdr->data2 = rte_cpu_to_be_32(-2);
	entry2_hdr->data3 = rte_cpu_to_be_32(-3);
	entry2_hdr->data4 = rte_cpu_to_be_32(-4);
	entry2_hdr->data5 = rte_cpu_to_be_32(-5);
	entry2_hdr->data6 = rte_cpu_to_be_32(-6);
	entry2_hdr->data7 = rte_cpu_to_be_32(-7);
	entry2_hdr->data8 = rte_cpu_to_be_32(-8);
	entry2_hdr->data9 = rte_cpu_to_be_32(-9);
	entry2_hdr->data10 = rte_cpu_to_be_32(-10);
	entry2_hdr->data11 = rte_cpu_to_be_32(-11);
	entry2_hdr->data12 = rte_cpu_to_be_32(-12);
	entry2_hdr->data13 = rte_cpu_to_be_32(-13);
	entry2_hdr->data14 = rte_cpu_to_be_32(-14);
	entry2_hdr->data15 = rte_cpu_to_be_32(-15);
	entry2_hdr->data16 = rte_cpu_to_be_32(-16);
	entry2_hdr->data17 = rte_cpu_to_be_32(-17);
	entry2_hdr->data18 = rte_cpu_to_be_32(-18);
	entry2_hdr->data19 = rte_cpu_to_be_32(-19);
	entry2_hdr->data20 = rte_cpu_to_be_32(-20);
	entry2_hdr->data21 = rte_cpu_to_be_32(-21);
	entry2_hdr->data22 = rte_cpu_to_be_32(-22);
	entry2_hdr->data23 = rte_cpu_to_be_32(-23);
	entry2_hdr->data24 = rte_cpu_to_be_32(-24);
	entry2_hdr->data25 = rte_cpu_to_be_32(-25);
	entry2_hdr->data26 = rte_cpu_to_be_32(-26);
	entry2_hdr->data27 = rte_cpu_to_be_32(-27);
	entry2_hdr->data28 = rte_cpu_to_be_32(-28);
	entry2_hdr->data29 = rte_cpu_to_be_32(-29);
	entry2_hdr->data30 = rte_cpu_to_be_32(-30);
	entry2_hdr->data31 = rte_cpu_to_be_32(-31);
	entry2_hdr->data32 = rte_cpu_to_be_32(-32);
}

struct rte_mbuf *pkts_tx_burst[TX_BURST_SIZE];
int num_pkts_to_send = 0;
uint64_t total_sent = 0;
// uint64_t total_resent = 0;
// uint64_t total_normal_sent = 0;


void send_packet_in_burst(struct rte_mbuf** pkts_tx_burst){

	uint16_t num_sent_pkts = 0;
	uint16_t nb_tx = 0;
	
	do{
		nb_tx = rte_eth_tx_burst(PORT_ID, 0, &pkts_tx_burst[num_sent_pkts], num_pkts_to_send - num_sent_pkts);
		num_sent_pkts += nb_tx;
	} while (num_sent_pkts < num_pkts_to_send);
	
	if (num_sent_pkts != num_pkts_to_send) {
		RTE_LOG(WARNING, WORKER, "Different (num_sent_pkts:%d/num_pkts_to_send:%do )\n", num_sent_pkts, num_pkts_to_send);
	}

	// 배열 초기화
	for(int i=0; i < num_sent_pkts; i++){
		pkts_tx_burst[i] = NULL; 
	}
	num_pkts_to_send = num_pkts_to_send - num_sent_pkts; // 전송한 패킷 수 초기화

}

// actually send the packet
static void send_packet(uint32_t seq_num, uint16_t switch_agtr_pos, bool resend_flag, uint8_t flags, uint16_t port_id, uint8_t quan_model_layer_jobid,
						struct window_manager *window_manager, struct rte_mempool *tx_mbuf_pool)
{
	// if (num_pkts_to_send <= TX_BURST_SIZE){
		
		union
		{
			uint64_t as_int;
			struct rte_ether_addr as_addr;
		} dst_eth_addr;
		struct rte_ether_hdr eth_hdr;

		struct rte_mbuf *pkt = rte_pktmbuf_alloc(tx_mbuf_pool);
		if (pkt == NULL)
		{
			printf("trouble at rte_mbuf_raw_alloc\n");
			return;
		}

		// rte_pktmbuf_reset_headroom(pkt);
		rte_pktmbuf_reset(pkt);

		// pkt->data_len = TX_PACKET_LENGTH;
		pkt->data_len = sizeof(struct rte_ether_hdr) +
						sizeof(struct rte_ipv4_hdr) +
						sizeof(struct rte_ceina_hdr) +
						sizeof(struct rte_ceina_entry1_hdr) +
						sizeof(struct rte_ceina_entry2_hdr);

		// set up addresses
		dst_eth_addr.as_int = rte_cpu_to_be_64(DST_MAC);
		rte_ether_addr_copy(&dst_eth_addr.as_addr, &eth_hdr.d_addr);
		rte_ether_addr_copy(&my_addr, &eth_hdr.s_addr);
		eth_hdr.ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

		if(LAYER_CONTROL){
			if(seq_num <= TOTAL_PACKET/2){
				quan_model_layer_jobid = (uint8_t)QUAN_LEVEL | (uint8_t)MODEL_SIZE | (APP_ID & 0xF);
			}
			else{
				quan_model_layer_jobid = (uint8_t)QUAN_LEVEL | (uint8_t)MODEL_SIZE | (1 << 4) | (APP_ID & 0xF);
			}
		}

		pkt_ceina_hdr.bitmap = rte_cpu_to_be_32(1 << (WORKER_ID - 1));
		pkt_ceina_hdr.PAS = rte_cpu_to_be_32( (quan_model_layer_jobid << 24) | seq_num);
		// pkt_ceina_hdr.app_id = rte_cpu_to_be_16(APP_ID);
		// pkt_ceina_hdr.seq_num = rte_cpu_to_be_16(seq_num);
		pkt_ceina_hdr.agtr_time = NUM_WORKER;
		pkt_ceina_hdr.agtr_idx = rte_cpu_to_be_16(switch_agtr_pos);
		pkt_ceina_hdr.agtr_idx2 = rte_cpu_to_be_16(switch_agtr_pos + MAX_AGTR_SIZE);
		pkt_ceina_hdr.flags = flags;
		if(APP_ID & 0x8){
			pkt_ceina_hdr.quantization_flags = 1; ///
		}
		else{
			pkt_ceina_hdr.quantization_flags = 3; ///
		}	
		if (resend_flag)
		{
			pkt_ceina_hdr.flags |= 0x4; // resend
			RTE_LOG(INFO, WORKER, ">**Resent %d, bitmap %d, appid %d, PAS %u agtr_time %d, agtr_idx %d agtr_idx2 %d, flags %d, quant_level %d, length: %d \n",
					seq_num,
					(1 << (WORKER_ID - 1)),
					APP_ID,
					pkt_ceina_hdr.PAS,
					NUM_WORKER,
					switch_agtr_pos,
					switch_agtr_pos + MAX_AGTR_SIZE,
					pkt_ceina_hdr.flags,
					pkt_ceina_hdr.quantization_flags,
					pkt->data_len);
		}
		else
		{
			RTE_LOG(INFO, WORKER, ">Sent %d, bitmap %d, appid %d, PAS %u agtr_time %d, agtr_idx %d agtr_idx2 %d, flags %d, quant_level %d, length: %d \n",
					seq_num,
					(1 << (WORKER_ID - 1)),
					APP_ID,
					pkt_ceina_hdr.PAS,
					NUM_WORKER,
					switch_agtr_pos,
					switch_agtr_pos + MAX_AGTR_SIZE,
					pkt_ceina_hdr.flags,
					pkt_ceina_hdr.quantization_flags,
					pkt->data_len);
		}

		// copy header to packet in mbuf
		rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *, 0),
				&eth_hdr, (size_t)sizeof(eth_hdr));
		rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *, sizeof(struct rte_ether_hdr)),
				&pkt_ip_hdr, (size_t)sizeof(pkt_ip_hdr));
		rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *,
										sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr)),
				&pkt_ceina_hdr, (size_t)sizeof(pkt_ceina_hdr));
		if(APP_ID & 0x8){
			rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *,
											sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ceina_hdr)),
					&pkt_ceina_entry1_1bit, (size_t)sizeof(pkt_ceina_entry1_1bit));
			rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *,
											sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ceina_hdr) + sizeof(struct rte_ceina_entry1_hdr)),
					&pkt_ceina_entry2_1bit, (size_t)sizeof(pkt_ceina_entry2_1bit));
		}
		else{
			rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *,
											sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ceina_hdr)),
					&pkt_ceina_entry1, (size_t)sizeof(pkt_ceina_entry1));
			rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *,
											sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ceina_hdr) + sizeof(struct rte_ceina_entry1_hdr)),
					&pkt_ceina_entry2, (size_t)sizeof(pkt_ceina_entry2));
		}

		/// DEBUG
		if(window_manager->isSent[seq_num] != 0 && resend_flag == 0){
			RTE_LOG(NOTICE, WORKER, "ERROR:DUP SENT %d pkt (f: %d) \n", seq_num, pkt_ceina_hdr.flags);
			// RTE_LOG(NOTICE, WORKER, "isSent[%d] is %d \n", seq_num+1, window_manager->isSent[seq_num+1]);
			// RTE_LOG(NOTICE, WORKER, "isSent[%d] is %d \n", seq_num+10, window_manager->isSent[seq_num+10]);
		}
		else{
			window_manager->isSent[seq_num] = true;
		}

		if(seq_num <=128){
			if (window_manager->isSent[seq_num] == 0)
			RTE_LOG(NOTICE, WORKER, "> isSent[%d] is %d \n", seq_num+128, window_manager->isSent[seq_num+128]);
		}




		// Add some pkt fields
		pkt->nb_segs = 1;
		pkt->pkt_len = pkt->data_len;
		pkt->ol_flags = 1;

		// Enqueue send the packet
		pkts_tx_burst[num_pkts_to_send] = pkt;
		num_pkts_to_send++;
		if(num_pkts_to_send >= WINDOW_SIZE/2){ ///todo;
			send_packet_in_burst(pkts_tx_burst);
		}

		if(num_pkts_to_send > TX_BURST_SIZE){
			RTE_LOG(WARNING, WORKER, "ERROR:overflow: seq_num:%d |num_pkts_to_send: %d\n", seq_num, num_pkts_to_send);
		}

}

// Initialize Port
static inline int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf txconf;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	rte_eth_dev_info_get(port, &dev_info);
	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |=
			DEV_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;

	// Allocate and set up 1 TX queue
	for (q = 0; q < tx_rings; q++)
	{
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
										rte_eth_dev_socket_id(port), &txconf);
		if (retval < 0)
			return retval;
	}

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++)
	{
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
										rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	/* get the port MAC address. */
	rte_eth_macaddr_get(port, &my_addr);
	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
		   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
		   port,
		   my_addr.addr_bytes[0], my_addr.addr_bytes[1],
		   my_addr.addr_bytes[2], my_addr.addr_bytes[3],
		   my_addr.addr_bytes[4], my_addr.addr_bytes[5]);

	rte_eth_promiscuous_enable(port);

	return 0;
}

static void exit_stats(int sig)
{
	time_t total_time;
	// total_time = time(NULL) - t1; //todo
	printf("Caught signal %d\n", sig);
	printf("\n=============== Stats =================\n");
	printf("Total packets: %lu\n", total_sent);
	printf("Total transmission time: %ld seconds\n", total_time);
	printf("Average transmission rate: %lu pps\n", total_sent / total_time);
	printf("                           %lu Mbps\n", ((total_sent * TX_PACKET_LENGTH * 8) / total_time) / 1000000);
	printf("=======================================\n");
	exit(0);
}



bool inline UpdateWindow(uint32_t seq_num, struct window_manager *window_manager)
{
	bool isLastAckUpdated = false;
	window_manager->isACKed[seq_num] = true;
	while (window_manager->isACKed[window_manager->last_ACK + 1])
	{
		window_manager->last_ACK++;
		// printf("window_manager->last_ACK : %d \n", window_manager->last_ACK);
		isLastAckUpdated = true;
	}
	return isLastAckUpdated;
}

bool inline UpdateSendWindow(uint32_t seq_num, struct window_manager *window_manager) ///
{
	window_manager->isSent[seq_num] = true;
	return true;
}

int Reset(uint32_t packet_total, struct window_manager *window_manager)
{
	window_manager->last_ACK = 0;
	window_manager->total_ACK = packet_total;
	memset(window_manager->isACKed, 0, sizeof(bool) * (packet_total+1));
	memset(window_manager->isSent, 0, sizeof(bool) * (packet_total+1));

}

static struct cc_manager
{
	int cwnd_bytes;
};

void init_cc_manager(struct cc_manager *cc_manager, int init_window)
{
	cc_manager->cwnd_bytes = init_window * TX_PACKET_LENGTH;
}

int adjustWindow(struct cc_manager *cc_manager, bool isECN, bool pkt_loss)
{

	if (isECN | pkt_loss)
	{
		cc_manager->cwnd_bytes /= 2;
		RTE_LOG(INFO, WORKER, "window is halved to %d \n", cc_manager->cwnd_bytes / TX_PACKET_LENGTH);

	}
	else
	{
		cc_manager->cwnd_bytes += 1500;
	}

	if (cc_manager->cwnd_bytes < TX_PACKET_LENGTH)
	{
		cc_manager->cwnd_bytes = TX_PACKET_LENGTH;
	}
	if (cc_manager->cwnd_bytes > MAX_BYTES)
	{
		cc_manager->cwnd_bytes = MAX_BYTES;
	}
	if (cc_manager->cwnd_bytes > TX_PACKET_LENGTH)
	{
		cc_manager->cwnd_bytes = (cc_manager->cwnd_bytes / TX_PACKET_LENGTH) * TX_PACKET_LENGTH;
	}
	return cc_manager->cwnd_bytes / TX_PACKET_LENGTH;
}

static struct hash_table
{
	bool *isAlreadyDeclare;
	int *predefine_agtr_list;
	uint16_t hash_pos;
	uint16_t *hash_map;
};

void hash_table_init(int size, struct hash_table *hash_table)
{
	hash_table->hash_map = (uint16_t *)malloc(size * sizeof(uint16_t));
	memset(hash_table->isAlreadyDeclare, 0, sizeof(bool) * size);
	memset(hash_table->predefine_agtr_list, 0, sizeof(bool) * size);
	for (int i = 0; i < size; i++)
	{
		hash_table->predefine_agtr_list[i] = i;
	}
	int random_seed = rand();
	// std::shuffle(hash_table->predefine_agtr_list, hash_table->predefine_agtr_list+size, std::default_random_engine(random_seed)); //todo
	hash_table->hash_pos = 0;
	printf("hash_table_init \n");
}

uint32_t crc32_le(uint32_t crc, unsigned char const* p, size_t len)
{
	while (len--)
	{
		crc ^= *p++;
		for (int i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
	}
	return ~crc;
}

void hash_table_new_crc(struct hash_table *hash_table, uint8_t appID, uint32_t index)
{
	uint8_t crc_input[] = {(uint8_t)(appID & 0xff), (uint8_t)(index & 0xff), (uint8_t)( (index >> 8) & 0xff), (uint8_t)( (index >> 16) & 0xff), 0, 0};
	int used_size = MAX_AGTR_SIZE;
	uint16_t new_value; // agtr_idx

	do
	{
		new_value = crc32_le(0xffffffff, crc_input, 6); // todo:hash
		new_value = new_value % used_size;				// todo
		crc_input[4]++;
		if (crc_input[4] == 255)
		{
			crc_input[4] = 0;
			crc_input[5]++;
		}
	} while (hash_table->isAlreadyDeclare[new_value]);
	hash_table->hash_map[index] = new_value;
	hash_table->isAlreadyDeclare[new_value] = true;
	// printf("hash_table_new_crc \n");
}

struct node
{
	struct rte_ceina_hdr *ceina_hdr;
	TAILQ_ENTRY(node)
	entries;
};

TAILQ_HEAD(pending_queue, node) head; // HEAD node

void init_queue()
{
	TAILQ_INIT(&head);
}

void pending_queue_push(struct rte_ceina_hdr *ceina_hdr)
{
	struct node *new_node = (struct node *)malloc(sizeof(struct node));
	new_node->ceina_hdr = ceina_hdr;
	TAILQ_INSERT_TAIL(&head, new_node, entries);
}

bool is_pending_queue_empty()
{
	return TAILQ_EMPTY(&head);
}

void pending_queue_pop(struct rte_ceina_hdr **dest)
{
	if (!TAILQ_EMPTY(&head))
	{
		struct node *temp_node = TAILQ_FIRST(&head);
		*dest = temp_node->ceina_hdr;
		TAILQ_REMOVE(&head, temp_node, entries);
		free(temp_node);
	}
}

void pending_queue_delete()
{
	if (!TAILQ_EMPTY(&head))
	{
		struct node *temp_node = TAILQ_FIRST(&head);
		TAILQ_REMOVE(&head, temp_node, entries);
		free(temp_node);
	}
}

int get_pending_queue_length()
{
	int length = 0;
	struct node *temp_node;
	TAILQ_FOREACH(temp_node, &head, entries)
	{
		length++;
	}
	return length;
}

struct rte_ceina_hdr *pending_queue_front()
{
	if (!TAILQ_EMPTY(&head))
	{
		struct node *temp_node = TAILQ_FIRST(&head);
		return temp_node->ceina_hdr;
	}
	return NULL;
}


void clear_pending_queue()
{
    struct node *current_node;

    // Iterate through the queue and free each node
    while (!TAILQ_EMPTY(&head))
    {
        current_node = TAILQ_FIRST(&head);
        TAILQ_REMOVE(&head, current_node, entries); // Remove the current node from the queue
        free(current_node->ceina_hdr); // Free data associated with the node
        free(current_node); // Free the node itself
    }

    // Reinitialize the queue after clearing it
    TAILQ_INIT(&head);
}




int main(int argc, char **argv) //FIXME:
{
	rte_log_set_global_level(LOG_LEVEL);

	int ret, c;
	uint16_t pkt_data_len;
	int mac_flag = 0, ip_src_flag = 0, ip_dst_flag = 0;
	int counter = 0;

	// DPDK
	uint16_t port_id = PORT_ID;
	uint8_t app_id = APP_ID;
	uint16_t queue_id = 0;
	

	// Priority
    uint8_t quan_level = QUAN_LEVEL;
    uint8_t model_size = MODEL_SIZE;
    uint8_t layer_index = LAYER_INDEX;	
	uint8_t	quan_model_layer_jobid = quan_level + model_size + layer_index + (APP_ID & 0xF); // 0b 0 0 model 0 jobid
	RTE_LOG(WARNING, WORKER, "quan_model_layer_jobid %d \n", quan_model_layer_jobid);

	// Parameters
	int max_agtr_size = MAX_AGTR_SIZE;
	int agtr_start_pos = 0;
	int p4ml_loss_packet = 0;
	int resent_to_be_sent = 0;
	int msgs_completed = 0;
	int this_pos_to_send = max_agtr_size;
	int total_packet = TOTAL_PACKET;
	int resend_waiting = 0;
	int finish_window_seq = WINDOW_SIZE;
	bool resend_flag = 0;

	uint32_t window = WINDOW_SIZE;
	uint32_t send_pointer = WINDOW_SIZE;



	// Count variables
	int num_received_pkt_round = 0;
	int num_received_normal_pkts = 0;
	int num_received_dup_pkts = 0;
	int num_normal_sent = 0;
	int num_resent_pkts = 0;
	
	uint64_t total_received_pkts = 0;
	uint64_t total_normal_received = 0;
	uint64_t total_dup_packet = 0;
	int total_ecn_pkts = 0;
	int total_collision_pkts = 0;
	uint64_t total_sent_pkts = 0;
	uint64_t total_normal_sent = 0;
	uint64_t total_resent = 0;
	
	// TEST count
	int total_error_pkts = 0;
	int window_decrease_cnt = 0;
	int window_case1 = 0;
	int window_case2 = 0;
	int window_case3 = 0;
	int window_case4 = 0;
	bool test_flag = 0;
	int pending_queue_cnt = 0;
	int pending_queue_sum = 0;
	int pending_queue_case1 = 0;
	int pending_queue_case2 = 0;
	int pending_queue_case3 = 0;
	int pending_queue_case4 = 0;
	int l1_loss_cnt = 0;
	int l2_loss_cnt = 0;
	int l1_cc_cnt = 0;

	// SUM/AVG
	float total_time_sum = 0;
 	float total_time_avg = 0;
	int total_resent_sum = 0;
 	int total_resent_avg = 0;
 	float throughput_sum = 0;
	float throughput_avg = 0;

	// Time variables
	struct timespec t1, t2, first_receive_time, finish_time;
	struct timespec job_start_time, job_finish_time, training_end_time, training_start_time;
	struct timespec start_test_time, test_time;
	uint64_t timeout_value = TIMEOUT_NS;

	// 1. Initialize EAL
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	argc -= ret;
	argv += ret;

	signal(SIGINT, exit_stats);
	
	
	char FILE_NAME[80] = "app";
	char APPID[10] = {};
	char WORKERID[10] = {};
	sprintf(APPID, "%d", APP_ID);
	sprintf(WORKERID, "%d", WORKER_ID);

	strcat(FILE_NAME, APPID);
	strcat(FILE_NAME, "_worker");
	strcat(FILE_NAME, WORKERID);
	char FILE_NAME_SUFFIX[60] = {};

	while ((c = getopt(argc, argv, "m:s:d:hf:")) != -1)
		switch (c)
		{
		case 'm':
			// note, not quite sure why last two bytes are zero, but that is how DPDK likes it
			DST_MAC = 0ULL;
			DST_MAC = string_to_mac(optarg) << 16;
			// DST_MAC = string_to_mac(DST_MAC_ADDR); // job1
			mac_flag = 1;
			break;
		case 's':
			IP_SRC_ADDR = string_to_ip(optarg);
			ip_src_flag = 1;
			break;
		case 'd':
			IP_DST_ADDR = string_to_ip(optarg);
			ip_dst_flag = 1;
			break;
		case 'h':
			printf("usage -- -m [dst MAC] -s [src IP] -d [dst IP]\n");
			exit(0);
			break;
		case 'p':
			port_id = optarg;
			break;
		case 'f':
            strcpy(FILE_NAME_SUFFIX, optarg);
            printf("FILE_NAME_SUFFIX: %s", FILE_NAME_SUFFIX);
            break;
		}


    if (strcmp(FILE_NAME_SUFFIX, "") != 0 ){
        strcat(FILE_NAME, FILE_NAME_SUFFIX);
        strcat(FILE_NAME, ".txt");
    }
    else{
        strcat(FILE_NAME, ".txt");
    }



	if (mac_flag == 0)
	{
		fprintf(stderr, "missing -m for destination MAC adress\n");
		exit(1);
	}
	if (ip_src_flag == 0)
	{
		fprintf(stderr, "missing -s for IP source adress\n");
		exit(1);
	}
	if (ip_dst_flag == 0)
	{
		fprintf(stderr, "missing -d for IP destination adress\n");
		exit(1);
	}

	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", RX_NUM_MBUFS,
										MBUF_CACHE_SIZE, 0, MBUF_BUF_SIZE, rte_socket_id());
	tx_mbuf_pool = rte_pktmbuf_pool_create("TX_MBUF_POOL", NUM_MBUFS,
										MBUF_CACHE_SIZE, 0, MBUF_BUF_SIZE, rte_socket_id());
	tx_mbuf_pool2 = rte_pktmbuf_pool_create("TX_MBUF_POOL2", NUM_MBUFS,
										MBUF_CACHE_SIZE, 0, MBUF_BUF_SIZE, rte_socket_id());
	tx_mbuf_pool3 = rte_pktmbuf_pool_create("TX_MBUF_POOL3", NUM_MBUFS,
										MBUF_CACHE_SIZE, 0, MBUF_BUF_SIZE, rte_socket_id());


	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	if (tx_mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool 1\n");
	if (tx_mbuf_pool2 == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool 2\n");
	if (tx_mbuf_pool3 == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool 3\n");

	// Initialize port 0
	if (port_init(port_id, mbuf_pool) != 0)
		rte_exit(EXIT_FAILURE, "Cannot init port 0\n");


	///ADDED
    struct rte_ring *rx_ring = rte_ring_create("RX_RING", RX_RING_SIZE, rte_socket_id(), 0); 
    if (rx_ring == NULL) {
        RTE_LOG(INFO, WORKER, "Failed to create RX ring buffer\n");
    }



	/* Creates new hash table [L612] */
	struct hash_table hash_table;
	hash_table.isAlreadyDeclare = (bool *)malloc(sizeof(bool) * max_agtr_size);
	hash_table.predefine_agtr_list = (int *)malloc(sizeof(int) * max_agtr_size);

	// hash_table.used_size = 0;
	hash_table_init(max_agtr_size, &hash_table);
	for (int i = 0; i < max_agtr_size; i++)
	{
		hash_table_new_crc(&hash_table, app_id, i);
	}

	/* 4. Make packet headers*/
	// pkt_data_len = (uint16_t)(TX_PACKET_LENGTH - (sizeof(struct rte_ether_hdr) +
	// 																							sizeof(struct rte_ipv4_hdr) +
	// 																							sizeof(struct rte_ceina_hdr) +
	// 																							sizeof(struct rte_ceina_entry1_hdr) +
	// 																							sizeof(struct rte_ceina_entry2_hdr)));
	pkt_data_len = (uint16_t)(sizeof(struct rte_ceina_hdr) +
							  sizeof(struct rte_ceina_entry1_hdr) +
							  sizeof(struct rte_ceina_entry2_hdr));

	if(app_id & 0x8){
		setup_pkt_headers_1bit(&pkt_ip_hdr, &pkt_ceina_hdr, &pkt_ceina_entry1_1bit, &pkt_ceina_entry2_1bit, pkt_data_len);
		
	}
	else{
		setup_pkt_headers(&pkt_ip_hdr, &pkt_ceina_hdr, &pkt_ceina_entry1, &pkt_ceina_entry2, pkt_data_len);
	}

	/* 5. Setup window manager */

	struct window_manager window_manager;
	window_manager.total_ACK = TOTAL_PACKET;
	window_manager.isACKed = (bool *)calloc( (TOTAL_PACKET+1), sizeof(bool));
	window_manager.isSent = (bool *)calloc( (TOTAL_PACKET+1), sizeof(bool));


	uint16_t nb_rx;
	const uint16_t nb_pkts = BURST_SIZE;
	long total_received_bytes = 0;


	/* Setup cc_manager*/
	struct cc_manager cc_manager;
	init_cc_manager(&cc_manager, WINDOW_SIZE);

	if(ENTER){
		RTE_LOG(WARNING, WORKER, "Press Enter to continue... \n");
		while (getchar() != '\n');
	}

	for(int round=1; round <= NUM_ROUND; round++){ //FIXME:


		clock_gettime(CLOCK_MONOTONIC ,&training_start_time);
		if(round <= NUM_ROUND){
			while(1){
				clock_gettime(CLOCK_MONOTONIC ,&training_end_time);
				long elapsed_training_time = (training_end_time.tv_sec-training_start_time.tv_sec) * 1000000000L
					+ (training_end_time.tv_nsec - training_start_time.tv_nsec);
				float time_span_training = (float)elapsed_training_time / 1000000000.0;
				
				if (time_span_training > TRAINING_TIME){
					break;
				}
			}
		}
		
		// Window reset
		Reset(TOTAL_PACKET, &window_manager);

		/* reset parameters */
		bool resend_flag = 0;
		uint8_t flags = 0;
		num_pkts_to_send = 0;
		uint32_t timeout_end_seq = 0;
		
		num_received_pkt_round = 0;
		num_received_dup_pkts = 0;
		num_received_normal_pkts = 0;
		num_received_dup_pkts = 0;
		
		num_normal_sent = 0;
		num_resent_pkts = 0;
		total_received_bytes = 0;
		
		/* Setup Pending queue */ // todo
		init_queue();		



		// struct rte_mbuf *pkts_tx_burst[TX_BURST_SIZE] = {NULL}; ///FIXED
		for (int i=0; i < TX_BURST_SIZE; i++){
			pkts_tx_burst[i] = NULL;
		}

		window = INITIAL_WINDOW_SIZE;
		finish_window_seq = window; /// MAX_AGTR_SIZE;
		send_pointer = 0;

		/* 6. Send first packets */
		uint32_t seq_num = 0; // SEQ number start from 1
		int num_first_time_sending = window;


		// 초기화가 제대로 되었는지 체크
		if(num_pkts_to_send != 0)
			RTE_LOG(WARNING, WORKER, "ERROR: num_pkts_to_send is not zero : %d) \n", num_pkts_to_send);


		// The first round sending
		// for (int i = 1; i <= WINDOW_SIZE; i++)
		for (int i = 1; i <= window; i++)
		{
			seq_num = i;
			uint16_t switch_agtr_pos = hash_table.hash_map[agtr_start_pos + i];
			if (seq_num <= total_packet)
			{
				/// ForceForward : not implemented

				// Send Packet
				bool resend_flag = 0;
				uint8_t flags = 0;

				send_packet(seq_num, switch_agtr_pos, resend_flag, flags, port_id, quan_model_layer_jobid, &window_manager, tx_mbuf_pool);
				num_normal_sent++;
				total_sent_pkts++;
			}
			else
			{
			}
		}
		send_pointer = seq_num;
		RTE_LOG(WARNING, WORKER, "Sent 1~%d \n", seq_num);

		if(num_pkts_to_send > 0){
			// RTE_LOG(WARNING, WORKER, "ERROR: Can't send first batch / num_pkts_to_send : %d \n", num_pkts_to_send);
			send_packet_in_burst(pkts_tx_burst);
		}

		/* Main packet send and receive loop*/
		/* Main packet send and receive loop*/
		/* Main packet send and receive loop*/
		int count_ = 0;

		while (window_manager.last_ACK < window_manager.total_ACK)
		{
			if( unlikely(num_received_pkt_round % 200000 == 0) ){
				// RTE_LOG(NOTICE, WORKER, "total_sent: %d | normal_sent: %d | resent: %d \n", total_sent, total_normal_sent, total_resent);
				uint32_t mbuf_count = rte_mempool_avail_count(mbuf_pool);
				uint32_t in_use_count = rte_mempool_in_use_count(mbuf_pool);


				RTE_LOG(NOTICE, WORKER, "received: %d |dup: %d |err: %d |total_sent: %d |normal_sent: %d |resent: %d: |ecn: %d   \n", 
					total_received_pkts, total_dup_packet, total_error_pkts, total_sent, total_normal_sent, total_resent, total_ecn_pkts);
				RTE_LOG(NOTICE, WORKER, "(seq_num:%d) window: %d  / mbuf_count:%d, in_use_count:%d \n", 
					seq_num, window, mbuf_count, in_use_count);
			}
			
			struct rte_mbuf *pkts_rx_burst[BURST_SIZE] = {NULL}; ///FIXED
			// struct rte_mbuf *pkts_tx_burst[TX_BURST_SIZE] = {NULL}; ///FIXED
			for (int i=0; i < TX_BURST_SIZE; i++){
				pkts_tx_burst[i] = NULL;
			}

			/* Timeout and loss recovery */
			clock_gettime(CLOCK_MONOTONIC, &t1);
			while (1)
			{
				clock_gettime(CLOCK_MONOTONIC, &t2);
				long time_span = (t2.tv_sec - t1.tv_sec) * 1000000000L + (t2.tv_nsec - t1.tv_nsec);

				/* Check if there are received packets  */
				nb_rx = rte_eth_rx_burst(port_id, queue_id, pkts_rx_burst, nb_pkts);
				total_received_pkts += nb_rx;
				num_received_pkt_round += nb_rx;


				// If there are packets, go to 'process received packets'
				if ( likely(nb_rx) )
				{
					break;
				}

				// Loss case 1 (L1)
				if (LOSS_RECOVERY_ENABLE)
				{
					// todo loss recovery logic
					if (time_span > timeout_value && nb_rx == 0)
					{
						l1_loss_cnt++;
						if (CC_ENABLE && LOSS_CC_ENABLE) ///
						{ // todo
							if (true)
							{
								l1_cc_cnt++;
								int new_window = adjustWindow(&cc_manager, 0, 1);
								if (send_pointer + new_window > window_manager.total_ACK)
								{
									window = window_manager.total_ACK - send_pointer;
								}
								else
								{
									window = new_window;
								}
								finish_window_seq += window;
							}
						}
						
						
						int loss_case;

						RTE_LOG(INFO, WORKER, "------------------------------------------- \n");

						if (!is_pending_queue_empty())
						{
							struct rte_ceina_hdr *pending_ceina_hdr = pending_queue_front();
							if(pending_ceina_hdr == NULL){
								RTE_LOG(WARNING, WORKER, "Error: pending_ceina_hdr indicates NULL \n" );
							}

							timeout_end_seq = rte_be_to_cpu_32(pending_ceina_hdr->PAS) & 0x00FFFFFF;

							if(timeout_end_seq <= window_manager.last_ACK){ ///FIXED
								RTE_LOG(WARNING, WORKER, "E: Pending pkt %d < last_ACK %d",
									timeout_end_seq, window_manager.last_ACK);
								timeout_end_seq = window_manager.last_ACK + 1;
								timeout_end_seq = timeout_end_seq + 1;
							}
							if(window_manager.last_ACK + 2*TX_BURST_SIZE < timeout_end_seq){ ///FIXED
								RTE_LOG(WARNING, WORKER, "E: Pending pkt %d >> last_ACK %d -> should be cleared (win:%d) \n",
									timeout_end_seq, window_manager.last_ACK, window);
								pending_queue_delete();
								timeout_end_seq = window_manager.last_ACK + 1;
								timeout_end_seq = timeout_end_seq + 1;
								
							}

							loss_case = 1;

						}
						else
						{
							timeout_end_seq = window_manager.last_ACK + 1;
							timeout_end_seq = timeout_end_seq + 1;

							loss_case = 2;
						}

						int resent_to_be_sent = 0;

						if(total_resent % 2000 == 0){
							RTE_LOG(NOTICE, WORKER, "*Timeout (no received), %d~%d should be resent! (num_pending_queue : %d), LASTACK: %d / num_total_recv/tx/re_tx: %d/%d/%d \n",
									window_manager.last_ACK + 1, timeout_end_seq - 1,
									get_pending_queue_length(),
									window_manager.last_ACK,
									total_received_pkts,
									total_sent_pkts,
									total_resent);
						}

						for (uint32_t timeout_seq = window_manager.last_ACK + 1; timeout_seq < timeout_end_seq; timeout_seq++) 
						{
							uint16_t switch_agtr_pos = hash_table.hash_map[agtr_start_pos + ((timeout_seq - 1) % max_agtr_size)];
							if ( timeout_seq <= total_packet)
							{
								if( window_manager.isACKed[timeout_seq] == 0) 
								{
									// send packet
									bool resend_flag = true;
									uint8_t flags = 0;
									flags = flags | (resend_flag << 2); 
									send_packet(timeout_seq, switch_agtr_pos, resend_flag, flags, port_id, quan_model_layer_jobid, &window_manager, tx_mbuf_pool);
									// if(timeout_seq <= 128 && total_resent % 100 == 0){
									// 	RTE_LOG(WARNING, WORKER, "**Resent %d (Timeout), agtr_idx:%d \n", timeout_seq, switch_agtr_pos);
									// }

									total_resent++;
									num_resent_pkts++;
									p4ml_loss_packet++;
									total_sent_pkts++;

									// if(send_pointer < timeout_seq){
									// 	RTE_LOG(WARNING, WORKER, "ERROR: send_pointer:%d < timeout_seq:%d / lastACK:%d / loss_case:%d\n", send_pointer, timeout_seq, window_manager.last_ACK, loss_case);
									// 	send_pointer = timeout_seq;
									// }

								}

							}
						}

						

						if(num_pkts_to_send > 0){
							send_packet_in_burst(pkts_tx_burst);
						}
						clock_gettime(CLOCK_MONOTONIC, &t1);
					}
				}
			}

			


			/* Process received packets */
			for (uint16_t j = 0; j < nb_rx; ++j)
			{
				bool pkt_loss_three_cont = 0;


				/* Header Parsing */
				struct rte_mbuf *mbuf = pkts_rx_burst[j];
				rte_prefetch0(rte_pktmbuf_mtod(mbuf, void *)); // 성능 향상을 위함
				pkts_rx_burst[j] = NULL;


				struct rte_ether_hdr *ether = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *); // Ethernet
				struct rte_ipv4_hdr *ipv4 = (struct rte_ipv4_hdr *)((uint8_t *)ether + sizeof(struct rte_ether_hdr));

				// If not ceina packet
				if (ipv4->next_proto_id != 0)
				{
					total_received_pkts--;
					num_received_pkt_round--;
					total_error_pkts++;
					RTE_LOG(WARNING, WORKER, " ERROR: not ceina packet \n");
					rte_pktmbuf_free(mbuf); ///TODOed
					continue;
				}

				struct rte_ceina_hdr *ceina_hdr = (struct rte_ceina_hdr *)((uint8_t *)ipv4 + sizeof(struct rte_ipv4_hdr));

				uint16_t packet_length = mbuf->data_len;
				total_received_bytes += packet_length;

				rte_be32_t src_addr = ipv4->src_addr;
				rte_be32_t dst_addr = rte_cpu_to_be_32(ipv4->dst_addr);

				uint32_t bitmap = rte_cpu_to_be_32(ceina_hdr->bitmap);
				uint8_t agtr_time = ceina_hdr->agtr_time;
				uint16_t agtr_idx = rte_cpu_to_be_16(ceina_hdr->agtr_idx);
				uint16_t agtr_idx2 = rte_cpu_to_be_16(ceina_hdr->agtr_idx2);
				uint8_t flags = ceina_hdr->flags;
				uint8_t quantization_flags = ceina_hdr->quantization_flags;
				bool is_ACK_packet = ceina_hdr->flags & 0x01;
				bool is_collision_packet = ceina_hdr->flags & 0x02;
				bool is_resend_packet = ceina_hdr->flags & 0x04;
				bool is_ecn_mark_packet = ceina_hdr->flags & 0x08;
				bool is_overflow_packet = ceina_hdr->flags & 0x80;
				uint8_t app_id = (uint8_t)((rte_cpu_to_be_32(ceina_hdr->PAS)) >> 24) & 0xF;
				rte_be32_t seq_num_ = rte_cpu_to_be_32(ceina_hdr->PAS) & 0x00FFFFFF;
				seq_num = seq_num_;

				RTE_LOG(INFO, WORKER, "---------------------------------------received mbuf: %d / num: %d \n", mbuf, total_received_pkts);

				// RTE_LOG(INFO, WORKER, "------mbuf-1: %d--------mbuf: %d ---------mbuf+1: %d----------/num: %d \n", mbuf_prev, mbuf, mbuf_next, num_received_pkts);
				if (seq_num % 50000 == 0){
					RTE_LOG(NOTICE, WORKER, "<Received: seq_num:%d |app_id:%d |bitmap:%d |flags:%d |window:%d |quan_level:%d |len:%d |agtr_idx:%d |LastACK:%d, PAS: %u \n",
						seq_num, app_id, bitmap, flags, window, quantization_flags, packet_length, agtr_idx, window_manager.last_ACK, dst_addr);
				}

				if(app_id <= 8){
					struct rte_ceina_entry1_hdr *ceina_entry1 = (struct rte_ceina_entry1_hdr *)((uint8_t *)ceina_hdr + sizeof(struct rte_ceina_hdr));
					struct rte_ceina_entry2_hdr *ceina_entry2 = (struct rte_ceina_entry2_hdr *)((uint8_t *)ceina_entry1 + sizeof(struct rte_ceina_entry1_hdr));
				}
				else if(app_id > 8){
					struct rte_ceina_entry1_hdr_1bit_global *ceina_entry1 = (struct rte_ceina_entry1_hdr_1bit_global *)((uint8_t *)ceina_hdr + sizeof(struct rte_ceina_hdr));
					struct rte_ceina_entry2_hdr_1bit_global *ceina_entry2 = (struct rte_ceina_entry2_hdr_1bit_global *)((uint8_t *)ceina_entry1 + sizeof(struct rte_ceina_entry1_hdr_1bit_global));
				}

				if ( (flags == 0 || flags == 4) ){
					RTE_LOG(WARNING, WORKER, "ERROR: sent packet, flags: %d, bitmap:%d \n", flags, bitmap);
					total_error_pkts++;
					rte_pktmbuf_free(mbuf); ///TODOed
					// pkts_rx_burst[j] = NULL;
					continue;
				}

				if (window_manager.isACKed[seq_num] == 1)
				{
					if (is_resend_packet == true){
						total_dup_packet++;
						num_received_dup_pkts++;
					}
					else{
					}
				}

				/* A. If Normal packet */
				RTE_LOG(INFO, WORKER, "window_manager.isACKed[%d] : %d, send_pointer:%d) \n", seq_num, window_manager.isACKed[seq_num], send_pointer); ////
				if ( !window_manager.isACKed[seq_num] )
				{ ///

					total_normal_received++;
					num_received_normal_pkts++;
					if (num_received_normal_pkts == 1)
					{
						clock_gettime(CLOCK_MONOTONIC, &first_receive_time);
						if(round == 1){
							clock_gettime(CLOCK_MONOTONIC, &job_start_time);
							clock_gettime(CLOCK_MONOTONIC, &start_test_time); ///
						}

					}
					clock_gettime(CLOCK_MONOTONIC, &test_time);
					if (   (test_time.tv_sec - start_test_time.tv_sec) * 1000000000L + (test_time.tv_nsec - first_receive_time.tv_nsec) > TEST_SECOND    ){
						RTE_LOG(WARNING, WORKER, "Window:%d \n", window);
						RTE_LOG(WARNING, WORKER, "isECN:%d \n", is_ecn_mark_packet);
						RTE_LOG(WARNING, WORKER, "seqnum:%d / finish_num: %d\n", seq_num, finish_window_seq);
						clock_gettime(CLOCK_MONOTONIC, &start_test_time);
						test_flag = 1;
					}



					
					RTE_LOG(INFO, WORKER, "N.1 pkt %d is normal and not ACKed (lastACK : %d) \n", seq_num, window_manager.last_ACK);

					if(is_collision_packet){
						total_collision_pkts++;
						if (CHANGE_AGTR_ENABLE){
							uint16_t new_agtr = (uint16_t)(bitmap & 0x0000FFFF);
							if(!hash_table.isAlreadyDeclare[new_agtr]){
								hash_table.hash_map[agtr_start_pos + ((seq_num - 1) % max_agtr_size)] = new_agtr;
								hash_table.isAlreadyDeclare[new_agtr] = true;
							}
							else{
							}
						}
					}


					// Update Window
					UpdateWindow(seq_num, &window_manager);
					// printf("lastACK : %d \n", window_manager.last_ACK);

					// next packet 추적
					uint32_t next_seq_num = seq_num + window;
					int next_offset = (next_seq_num - 1) * 64;

					// N.1.A ACK이 아직 다 도착하지 않아서 보낼 수 없는 경우, 보내지 않고 큐에 저장함
					if (next_seq_num > window_manager.last_ACK + window)
					{
						// ceina_hdr를 pending_queue에 넣어놓음
						struct rte_ceina_hdr *enqueue_hdr = (struct rte_ceina_hdr *)malloc(sizeof(struct rte_ceina_hdr));
						memcpy(enqueue_hdr, ceina_hdr, sizeof(struct rte_ceina_hdr));
						pending_queue_push(enqueue_hdr);
						RTE_LOG(INFO, WORKER, "N.1.A pkt %d is pushed to pending queue (lastACK : %d) \n", seq_num, window_manager.last_ACK);
					}

					int n1b_prev_send_pointer = send_pointer;
					uint32_t n1b_process_next_seq_num;


					// N.1.B Send Next Packet
					if (next_seq_num <= total_packet &&
						next_seq_num <= window_manager.last_ACK + window &&
						next_seq_num > send_pointer)
					{


						int packet_to_process = next_seq_num - send_pointer;
						// N.1.B 보내야할 패킷마다 받은 패킷을 이용해서 헤더 생성 및 전송
						for (int i = packet_to_process - 1; i >= 0; i--) //FIXME:
						{
							uint32_t process_next_seq_num = next_seq_num - i;

							// agtr_index 할당
							uint16_t switch_agtr_pos = hash_table.hash_map[agtr_start_pos + ((process_next_seq_num - 1) % max_agtr_size)];


							bool resend_flag = 0;
							uint8_t flags = 0;
							if( (window_manager.isSent[process_next_seq_num] != 0 && resend_flag == 0)){
								RTE_LOG(WARNING, WORKER, "ERROR: RECV:%d / DUP_TO_BE_SENT %d (isSent=%d) pkt N.1.B / send_pointer:%d \n", 
									seq_num, process_next_seq_num, window_manager.isSent[process_next_seq_num], send_pointer);
								RTE_LOG(NOTICE, WORKER, ">N.1.B sent %d (recv_seq:%d, lastACK:%d, send_pointer:%d, pkt_to_pro:%d, ceina_hdr->flags:%d ) \n", 
									process_next_seq_num, seq_num, window_manager.last_ACK, send_pointer, packet_to_process, ceina_hdr->flags);
							}									
							send_packet(process_next_seq_num, switch_agtr_pos, resend_flag, flags, PORT_ID, quan_model_layer_jobid, &window_manager, tx_mbuf_pool2);
							n1b_process_next_seq_num = process_next_seq_num;
							RTE_LOG(INFO, WORKER, "N.1.B sent %d (recv_seq:%d, lastACK:%d, send_pointer:%d, pkt_to_pro:%d, ) \n", 
								process_next_seq_num, seq_num, window_manager.last_ACK, send_pointer, packet_to_process);

							num_normal_sent++;
							total_sent_pkts++;

							// to_be_sent++;
							// rte_pktmbuf_free(mbuf);
						}
						send_pointer = next_seq_num;
					}

					bool check_flag = 0;
					// N.2 Check If packet in Pending Queue is Ready to send (<- Send Next Packet)
					while (is_pending_queue_empty() == 0)
					{
						if (check_flag == 0){
							pending_queue_cnt++;
							pending_queue_sum += get_pending_queue_length();
							check_flag = 1;

							///TEST
							if( 1<= get_pending_queue_length() && get_pending_queue_length() <32){
								pending_queue_case1++;
							}
							else if( 32<= get_pending_queue_length() && get_pending_queue_length() <64){
								pending_queue_case2++;
							}
							else if( 64<= get_pending_queue_length() && get_pending_queue_length() <=96){
								pending_queue_case3++;
							}
							else if( 96<= get_pending_queue_length()){
								pending_queue_case4++;
							}

							if (test_flag == 1){
								RTE_LOG(WARNING, WORKER, "pending_queue_cnt %d \n", pending_queue_cnt);
								test_flag = 0;
							}

						}

						struct rte_ceina_hdr *pending_ceina_hdr = pending_queue_front();
						if(pending_ceina_hdr == NULL){
							RTE_LOG(WARNING, WORKER, "Error: pending_ceina_hdr indicates NULL \n" );
						}

						uint32_t pending_seq_num = rte_cpu_to_be_32(pending_ceina_hdr->PAS) & 0x00FFFFFF;

						// N.2.A 보낼 패킷의 seq_num이 도착한 패킷의 seq보다 크다 -> 더 기다려야함
						if (window_manager.last_ACK < pending_seq_num)
						{
							RTE_LOG(INFO, WORKER, "N.2.A Hold, pending_seq_num:%d (recv_seq:%d, last_ACK:%d) \n", 
								pending_seq_num, seq_num, window_manager.last_ACK);
							break;
						}

						// Check next_seq_num
						uint32_t next_seq_num = pending_seq_num + window;
						int next_offset = (next_seq_num - 1) * 64;

						// N.2.B 보낼 pkt의 seq_num이 보낼 수 있는 범위에 있다. //FIXME:
						if (send_pointer < next_seq_num && next_seq_num <= window_manager.last_ACK + window)
						{
							// ForeForward // TODO of TODO



							// send first packet in pending queue
							if ( next_seq_num <= total_packet )
							{
								uint32_t packet_to_process_ = abs(next_seq_num - send_pointer);
								// printf("num of packet to process : %d \n", packet_to_process);
								for (int i = packet_to_process_ - 1; i >= 0; i--)
								{
									uint32_t process_next_seq_num_ = next_seq_num - i;
									uint16_t switch_agtr_pos_ = hash_table.hash_map[agtr_start_pos + ((process_next_seq_num_ - 1) % max_agtr_size)];
									bool resend_flag = 0;
									uint8_t flags = 0;
									if( (window_manager.isSent[process_next_seq_num_] != 0 && resend_flag == 0) ){
										RTE_LOG(WARNING, WORKER, "ERROR:DUP SENT %d pkt N.2.B \n", seq_num);
									}			
									send_packet(process_next_seq_num_, switch_agtr_pos_, resend_flag, flags, PORT_ID, quan_model_layer_jobid, &window_manager, tx_mbuf_pool3);
									RTE_LOG(INFO, WORKER, "N.2.B sent %d (lastACK : %d, recv_seq: %d, n1b_prev_send_pointer:%d, n1b_sent_seq_num:%d ) \n", 
										process_next_seq_num_, window_manager.last_ACK, seq_num, n1b_prev_send_pointer, n1b_process_next_seq_num);
									num_normal_sent++;
									total_sent_pkts++;
								}
								send_pointer = next_seq_num;
							}
							free(pending_ceina_hdr);
							pending_queue_delete();
						}
						
						//
						else
						{ // 보낼 필요가 없음
							free(pending_ceina_hdr);
							pending_queue_delete();
							RTE_LOG(INFO, WORKER, "discard %d from pending queue (num_pen : %d) (lastACK : %d) \n", pending_seq_num, get_pending_queue_length() ,window_manager.last_ACK);
						}
					}

					// if (!isForceForward && LOSS_RECOVERY_ENABLE) {
					if (L2_ENABLE){ ///
					if (LOSS_RECOVERY_ENABLE) //TODO:
					{
						if (!is_pending_queue_empty() && get_pending_queue_length() >= 3)
						// if (!is_pending_queue_empty())
						{
							struct rte_ceina_hdr *pending_ceina_hdr = pending_queue_front();

							if (window_manager.last_ACK + 1 > resend_waiting)
							{
								l2_loss_cnt++;
								pkt_loss_three_cont = 1;
								// if (resend_pos_to_send > dma_context->my_send_queue_length - 2 * max_agtr_size_per_thread)
								// 		resend_pos_to_send = dma_context->my_send_queue_length / 2 + 1;
								uint32_t pending_seq_num = rte_cpu_to_be_32(pending_ceina_hdr->PAS) & 0x00FFFFFF;

								int resent_to_be_sent = 0;
								RTE_LOG(NOTICE, WORKER, "L2 pkt %d trigger %d~%d Resend! (num_pending_queue: %d) \n", seq_num,
										window_manager.last_ACK + 1,
										pending_seq_num - 1,
										get_pending_queue_length());

								for (uint32_t resend_seq = window_manager.last_ACK + 1; resend_seq < pending_seq_num; resend_seq++)
								{
									// int offset = (resend_seq - 1) * 64;
									uint16_t switch_agtr_pos = hash_table.hash_map[agtr_start_pos + ((resend_seq - 1) % max_agtr_size)];
									if (likely(resend_seq <= total_packet))
									{
										if( likely(window_manager.isACKed[resend_seq] == 0)) /// FIXME:
										{
											// send packet
											bool resend_flag = 1;
											uint8_t flags = 0;
											flags = flags | (resend_flag << 2); 
											send_packet(resend_seq, switch_agtr_pos, resend_flag, flags, port_id, quan_model_layer_jobid, &window_manager, tx_mbuf_pool);
											RTE_LOG(NOTICE, WORKER, "**Resent %d from pending queue (pending_seq_num: %d) L2 \n ", 
												resend_seq, pending_seq_num);

											total_resent++;
											num_resent_pkts++;
											p4ml_loss_packet++;
											total_sent_pkts++;
										}

									}
									resend_waiting = resend_seq;
									clock_gettime(CLOCK_MONOTONIC, &t1);
								}
								free(pending_ceina_hdr); ///FIXME:
								pending_queue_delete(); ///FIXME:
							}
						}
					}
					} ///

					if(is_ecn_mark_packet){
						total_ecn_pkts++;
					}
					if (CC_ENABLE)
					{ // todo
						if (seq_num >= finish_window_seq)
						{

							int new_window = adjustWindow(&cc_manager, is_ecn_mark_packet, pkt_loss_three_cont); ///
							if (send_pointer + new_window > window_manager.total_ACK)
							{
								window = window_manager.total_ACK - send_pointer;
							}
							else
							{
								window = new_window;
							}
							finish_window_seq += window;
						}
					}
				}
				rte_pktmbuf_free(mbuf);

				if (window < 32){
					window_case1++;
				}
				if (32 <= window && window < 64){
					window_case2++;
				}
				if (64 <= window && window < 96){
					window_case3++;
				}
				if (96 <= window && window <= 128){
					window_case4++;
				}

				if(num_pkts_to_send > 0){
					send_packet_in_burst(pkts_tx_burst);
				}
			}


		}
		clock_gettime(CLOCK_MONOTONIC, &finish_time);
		long elapsed_time = (finish_time.tv_sec - first_receive_time.tv_sec) * 1000000000L + (finish_time.tv_nsec - first_receive_time.tv_nsec);
		float total_time = (float)elapsed_time / 1000000000.0;
		

		total_time_sum += total_time;
		total_time_avg = total_time_sum / (round);
		total_resent_sum += total_resent;
		total_resent_avg = total_resent_sum / (round);
		total_normal_sent += num_normal_sent;

		float throughput = (float)( (double)total_received_bytes * 8 / 1024 / 1024 / 1024 / total_time);
		throughput_sum += throughput;
		throughput_avg = throughput_sum / (round);

		RTE_LOG(WARNING, WORKER, "=======================Round: %d/%d =======================  \n", round, NUM_ROUND);
		// RTE_LOG(WARNING, WORKER, "first_receive_time: %ld \n", first_receive_time.tv_sec);
		// RTE_LOG(WARNING, WORKER, "finish_time: %ld \n", finish_time.tv_sec);
		// RTE_LOG(WARNING, WORKER, "-------------------------Received---------------------  \n");
		// RTE_LOG(WARNING, WORKER, "-------------------------Sent-------------------------  \n");
		RTE_LOG(WARNING, WORKER, "num_received_pkts (round)        : %d | max_seq_num:%d \n", num_received_pkt_round, TOTAL_PACKET);
		RTE_LOG(WARNING, WORKER, "num_received_normal_pkts (round) : %d \n", num_received_normal_pkts);
		RTE_LOG(WARNING, WORKER, "num_received_dup_pkts (round)    : %d \n", num_received_dup_pkts);
		RTE_LOG(WARNING, WORKER, "received_bytes (round)           : %ld \n", total_received_bytes);
		RTE_LOG(WARNING, WORKER, "num_normal_sent                  : %d \n", num_normal_sent);
		RTE_LOG(WARNING, WORKER, "num_resent_pkts (round)          : %d \n", num_resent_pkts);
		RTE_LOG(WARNING, WORKER, "*total_time                      : %4.4f \n", total_time);
		RTE_LOG(WARNING, WORKER, "*throughput                      : %4.4f Gbps \n", throughput);
		RTE_LOG(WARNING, WORKER, "-------------------------Total-------------------------  \n");
		RTE_LOG(WARNING, WORKER, "total_received            : %llu \n", total_received_pkts);
		RTE_LOG(WARNING, WORKER, "total_normal_received     : %llu \n", total_normal_received);
		RTE_LOG(WARNING, WORKER, "total_received_dup_packet : %llu \n", total_dup_packet);
		RTE_LOG(WARNING, WORKER, "total_ecn_pkts            : %d \n", total_ecn_pkts);
		RTE_LOG(WARNING, WORKER, "total_collision_pkts      : %d \n", total_collision_pkts);
		RTE_LOG(WARNING, WORKER, "total_sent                : %llu \n", total_sent_pkts);
		RTE_LOG(WARNING, WORKER, "total_normal_sent         : %llu \n", total_normal_sent);
		RTE_LOG(WARNING, WORKER, "total_resent              : %llu \n", total_resent);
		RTE_LOG(WARNING, WORKER, "window %d %d %d %d\n", window_case1, window_case2, window_case3, window_case4);
		RTE_LOG(WARNING, WORKER, "l1_loss_cnt %d / l2_loss_cnt %d \n", l1_loss_cnt, l2_loss_cnt);	
		RTE_LOG(WARNING, WORKER, "l1_cc_cnt %d \n", l1_cc_cnt);	
		RTE_LOG(WARNING, WORKER, "pending_queue_avg %4.4f \n", (float)(pending_queue_sum) / (float)pending_queue_cnt);	
		RTE_LOG(WARNING, WORKER, "pending_queue_case :  %d / %d / %d / %d \n", pending_queue_case1, 
			pending_queue_case2, pending_queue_case3, pending_queue_case4);		
		RTE_LOG(WARNING, WORKER, "-------------------------Avg-------------------------  \n");
		RTE_LOG(WARNING, WORKER, "total_resent_avg    : %d \n", total_resent_avg);
		RTE_LOG(WARNING, WORKER, "*avg_iteration_time : %4.4f s\n", total_time_avg);
		RTE_LOG(WARNING, WORKER, "*avg_throughput     : %4.4f Gbps\n", throughput_avg);
		RTE_LOG(WARNING, WORKER, "====================================================\n");
		RTE_LOG(WARNING, WORKER, "\n");

		// Free window
		// free(window_manager.isACKed);
		// free(window_manager.isSent);



		// log_level = 7;


	}

	clock_gettime(CLOCK_MONOTONIC, &job_finish_time);
	long job_completion_time = (job_finish_time.tv_sec - job_start_time.tv_sec) * 1000000000L + (job_finish_time.tv_nsec - job_start_time.tv_nsec);
	float job_completion_time_sec = (float)job_completion_time / 1000000000.0;
	
	RTE_LOG(WARNING, WORKER, "Job_completion_time: %4.4f s\n", job_completion_time_sec);

	int RX_PACKET_LENGTH;
	if (APP_ID > 0x8){
		RX_PACKET_LENGTH = 113;
	}
	else{
		RX_PACKET_LENGTH = 305;
	}

	FILE *file_ptr;
	file_ptr = fopen(FILE_NAME, "w");
	if (file_ptr == NULL){
		printf("파일을 열 수 없습니다. \n");
	}

    fprintf(file_ptr, "total_received            : %llu \n", total_received_pkts);
    fprintf(file_ptr, "total_normal_received     : %llu \n", total_normal_received);
    fprintf(file_ptr, "total_received_dup_packet : %llu \n", total_dup_packet);
    fprintf(file_ptr, "total_ecn_pkts            : %d \n", total_ecn_pkts);
    fprintf(file_ptr, "total_collision_pkts      : %d \n", total_collision_pkts);
    fprintf(file_ptr, "total_sent                : %llu \n", total_sent_pkts);
    fprintf(file_ptr, "total_normal_sent         : %llu \n", total_normal_sent);
    fprintf(file_ptr, "total_resent              : %llu \n", total_resent);
    fprintf(file_ptr, "total_resent_avg    		 : %d \n", total_resent_avg);
    fprintf(file_ptr, "avg_iteration_time 		 : %4.4f s\n", total_time_avg);
    fprintf(file_ptr, "avg_throughput     		 : %4.4f Gbps\n", throughput_avg);
    fprintf(file_ptr, "job_completion_time		 : %4.4f s\n", job_completion_time_sec);
	fprintf(file_ptr, "========================================\n");
	float sent_amount_traffic = (double)(total_sent_pkts) / 1024  / 1024 / 1024 * TX_PACKET_LENGTH;
	float recv_amount_traffic = (double)(total_received_pkts) / 1024  / 1024 / 1024 * RX_PACKET_LENGTH;
    fprintf(file_ptr, "sent_amount_traffic		: %.3f GB \n", sent_amount_traffic); // all 305B
	fprintf(file_ptr, "recv_amount_traffic		: %.3f GB \n", recv_amount_traffic); //


    // 파일 닫기
    fclose(file_ptr);

	free(window_manager.isACKed);
	free(window_manager.isSent);
	
	// rte_free(pkts_rx_burst);
	return (0);
}
