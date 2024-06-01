#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <memory.h>
#include <zlib.h>
#include <sys/queue.h>
#include <sys/resource.h>

#include <rte_memory.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>

#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_ethdev.h>
#include <rte_udp.h>
#include <rte_ip.h>
#include <rte_version.h>
#include <rte_log.h>

#define RTE_LOGTYPE_PS RTE_LOGTYPE_USER1
#define LOG_LEVEL 5
// 5 WARNING 6 NOTICE 7 INFO 8 DEBUG
int log_level = LOG_LEVEL; ////FIXME

// #include "queue.h"
// #include <window_manager.h>
// #include "ps.h"

#define TX_PACKET_LENGTH 305
#define MAX_AGTR_SIZE 1000 // Actually 2*MAX_AGTR_SIZE used in switch because we uses two entries

#define MAX_WINDOW_SIZE 128
#define RX_BURST_SIZE 128
#define TX_BURST_SIZE 128

#define TOTAL_GRADIENTS 640000000 

#define MAX_SEQ_NUM TOTAL_GRADIENTS / 64
#define MAX_SEQ_NUM_1bit TOTAL_GRADIENTS / 64 / 8
#define MAX_WORKER 16
#define MAX_NUM_APPS 5
#define NUM_ROUND 100

#define TRAINING_TIME 2
#define MEASUREMENT_SECOND 500
#define TIME_DURATION 50 // ms


#define MEASUREMENT_TIME MEASUREMENT_SECOND * 1000 / TIME_DURATION

#define CRCPOLY_LE 0xedb88320
#define LOSS_RECOVERY_ENABLE 1
#define CHANGE_AGTR_ENABLE 0
#define LOSS_RECOVERY_LOG 0
#define EANBLE_LOG 0
#define NECESSARY_LOG 0


#define PORT_ID 1
#define RX_RING_SIZE 2048
#define TX_RING_SIZE 1024


#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 250

#define UDP_SRC_PORT 6666
#define UDP_DST_PORT 6666

#define IP_DEFTTL 64 /* from RFC 1340. */
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











#define MAX_TENSOR_SIZE TOTAL_GRADIENTS
#define MAX_APP_PER_THREAD 10
#define MAX_STORAGE_PER_APP_PER_THREAD 1
// #define MAX_WORKER 16
#define OVERFLOW_HANDLE false

#define MAX_ENTRIES_PER_PACKET 64

struct data_t {
    int32_t *data_int;
    float *data_float;
};

struct window_manager
{
	bool *isACKed;
	// bool *isSent;
	uint32_t total_ACK;
	uint32_t last_ACK;
};


struct tensor_context {
    bool* isOccupy;
    bool* isCollision;
    bool* isFloat;
    bool isCompleted;
    struct data_t data;
    uint32_t len;
    uint64_t key;
    uint8_t num_worker;
    struct window_manager* window_manager_arr;
    // struct timespec start_time;
};

void inline init_tensor(struct tensor_context* tensor, uint32_t len) {
    tensor->data.data_int = (int32_t*) malloc (sizeof(int32_t) * len);
    tensor->data.data_float = (float*) malloc (sizeof(float) * len);
    tensor->isCompleted = true;
    tensor->isOccupy = (bool*)malloc(sizeof(bool) * (MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1));
    tensor->isCollision = (bool*)malloc(sizeof(bool) * (MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1));
    tensor->isFloat = (bool*)malloc(sizeof(bool) * (MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1));
    tensor->len = 0;
    tensor->num_worker = 0;
    tensor->key = 0xffffffffffffffff;
    tensor->window_manager_arr = malloc(MAX_WORKER * sizeof(struct window_manager));
    for (int i = 0; i < MAX_WORKER; i++) {
        tensor->window_manager_arr[i].isACKed = calloc((MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1), sizeof(bool));
        tensor->window_manager_arr[i].total_ACK = MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1;
        tensor->window_manager_arr[i].last_ACK = 0;
    }
    
}

void reset_tensor(struct tensor_context* tensor, uint32_t len) {
    tensor->data.data_int = (int32_t*) malloc (sizeof(int32_t) * len);
    tensor->data.data_float = (float*) malloc (sizeof(float) * len);
    tensor->isCompleted = true;
    tensor->isOccupy = (bool*)malloc(sizeof(bool) * (MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1));
    tensor->isCollision = (bool*)malloc(sizeof(bool) * (MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1));
    tensor->isFloat = (bool*)malloc(sizeof(bool) * (MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1));
    tensor->len = 0;
    tensor->num_worker = 0;
    tensor->key = 0xffffffffffffffff;
    tensor->window_manager_arr = malloc(MAX_WORKER * sizeof(struct window_manager));
    for (int i = 0; i < MAX_WORKER; i++) {
        tensor->window_manager_arr[i].isACKed = calloc((MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1), sizeof(bool));
        tensor->window_manager_arr[i].total_ACK = MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1;
        tensor->window_manager_arr[i].last_ACK = 0;
    }
    
}

























static int core()
{

	struct rlimit core_limits;
	core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &core_limits);

	return 0;
}

uint64_t DST_MAC;
uint32_t IP_SRC_ADDR, IP_DST_ADDR;

static const struct rte_eth_conf port_conf_default = {
	.rxmode = {.max_rx_pkt_len = RTE_ETHER_MAX_LEN}};

static struct rte_ceina_hdr
{
	rte_be32_t bitmap;
	uint8_t agtr_time;
	uint8_t flags;
	rte_be32_t app_id_seq_num;
	// rte_be16_t appid;
	// rte_be16_t seq_num;
	uint8_t quantization_flags; // ceina
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
struct rte_mbuf *pkts_tx_burst[TX_BURST_SIZE];
int num_pkts_to_send = 0;



uint32_t string_to_ip(char *);
uint64_t string_to_mac(char *);
// static void send_packet(uint16_t);
static void exit_stats(int);
;
static uint64_t packet_count = 0;
static time_t t1;

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

	entry1_hdr->data1 = rte_cpu_to_be_32(10);
	entry1_hdr->data2 = rte_cpu_to_be_32(10);
	entry1_hdr->data3 = rte_cpu_to_be_32(10);
	entry1_hdr->data4 = rte_cpu_to_be_32(10);
	entry1_hdr->data5 = rte_cpu_to_be_32(10);
	entry1_hdr->data6 = rte_cpu_to_be_32(10);
	entry1_hdr->data7 = rte_cpu_to_be_32(10);
	entry1_hdr->data8 = rte_cpu_to_be_32(10);
	entry1_hdr->data9 = rte_cpu_to_be_32(10);
	entry1_hdr->data10 = rte_cpu_to_be_32(10);
	entry1_hdr->data11 = rte_cpu_to_be_32(10);
	entry1_hdr->data12 = rte_cpu_to_be_32(10);
	entry1_hdr->data13 = rte_cpu_to_be_32(10);
	entry1_hdr->data14 = rte_cpu_to_be_32(10);
	entry1_hdr->data15 = rte_cpu_to_be_32(10);
	entry1_hdr->data16 = rte_cpu_to_be_32(10);
	entry1_hdr->data17 = rte_cpu_to_be_32(10);
	entry1_hdr->data18 = rte_cpu_to_be_32(10);
	entry1_hdr->data19 = rte_cpu_to_be_32(10);
	entry1_hdr->data20 = rte_cpu_to_be_32(10);
	entry1_hdr->data21 = rte_cpu_to_be_32(10);
	entry1_hdr->data22 = rte_cpu_to_be_32(10);
	entry1_hdr->data23 = rte_cpu_to_be_32(10);
	entry1_hdr->data24 = rte_cpu_to_be_32(10);
	entry1_hdr->data25 = rte_cpu_to_be_32(10);
	entry1_hdr->data26 = rte_cpu_to_be_32(10);
	entry1_hdr->data27 = rte_cpu_to_be_32(10);
	entry1_hdr->data28 = rte_cpu_to_be_32(10);
	entry1_hdr->data29 = rte_cpu_to_be_32(10);
	entry1_hdr->data30 = rte_cpu_to_be_32(10);
	entry1_hdr->data31 = rte_cpu_to_be_32(10);
	entry1_hdr->data32 = rte_cpu_to_be_32(10);

	entry2_hdr->data1 = rte_cpu_to_be_32(10);
	entry2_hdr->data2 = rte_cpu_to_be_32(10);
	entry2_hdr->data3 = rte_cpu_to_be_32(10);
	entry2_hdr->data4 = rte_cpu_to_be_32(10);
	entry2_hdr->data5 = rte_cpu_to_be_32(10);
	entry2_hdr->data6 = rte_cpu_to_be_32(10);
	entry2_hdr->data7 = rte_cpu_to_be_32(10);
	entry2_hdr->data8 = rte_cpu_to_be_32(10);
	entry2_hdr->data9 = rte_cpu_to_be_32(10);
	entry2_hdr->data10 = rte_cpu_to_be_32(10);
	entry2_hdr->data11 = rte_cpu_to_be_32(10);
	entry2_hdr->data12 = rte_cpu_to_be_32(10);
	entry2_hdr->data13 = rte_cpu_to_be_32(10);
	entry2_hdr->data14 = rte_cpu_to_be_32(10);
	entry2_hdr->data15 = rte_cpu_to_be_32(10);
	entry2_hdr->data16 = rte_cpu_to_be_32(10);
	entry2_hdr->data17 = rte_cpu_to_be_32(10);
	entry2_hdr->data18 = rte_cpu_to_be_32(10);
	entry2_hdr->data19 = rte_cpu_to_be_32(10);
	entry2_hdr->data20 = rte_cpu_to_be_32(10);
	entry2_hdr->data21 = rte_cpu_to_be_32(10);
	entry2_hdr->data22 = rte_cpu_to_be_32(10);
	entry2_hdr->data23 = rte_cpu_to_be_32(10);
	entry2_hdr->data24 = rte_cpu_to_be_32(10);
	entry2_hdr->data25 = rte_cpu_to_be_32(10);
	entry2_hdr->data26 = rte_cpu_to_be_32(10);
	entry2_hdr->data27 = rte_cpu_to_be_32(10);
	entry2_hdr->data28 = rte_cpu_to_be_32(10);
	entry2_hdr->data29 = rte_cpu_to_be_32(10);
	entry2_hdr->data30 = rte_cpu_to_be_32(10);
	entry2_hdr->data31 = rte_cpu_to_be_32(10);
	entry2_hdr->data32 = rte_cpu_to_be_32(10);
}

void send_packet_in_burst(){


	uint16_t num_sent_pkts = 0;
	uint16_t nb_tx = 0;
	
	do{
		nb_tx = rte_eth_tx_burst(PORT_ID, 0, &pkts_tx_burst[num_sent_pkts], num_pkts_to_send - num_sent_pkts);
		num_sent_pkts += nb_tx;
	} while (num_sent_pkts < num_pkts_to_send);

	if (num_sent_pkts != num_pkts_to_send) {
		RTE_LOG(WARNING, PS, "Different (num_sent_pkts:%d/num_pkts_to_send:%do )\n", num_sent_pkts, num_pkts_to_send);
	}

	// 배열 초기화
	for(int i=0; i < num_sent_pkts; i++){
		pkts_tx_burst[i] = NULL; 
	}
	num_pkts_to_send = num_pkts_to_send - num_sent_pkts; // 전송한 패킷 수 초기화

}


// actually send the packet
static void send_packet(uint32_t seq_num, uint16_t switch_agtr_pos, uint32_t bitmap, bool resend_flag, uint8_t flags, uint16_t port_id, uint8_t app_id, uint8_t quan_model_layer_jobid, struct rte_mempool *tx_mbuf_pool)
{
	// if (num_pkts_to_send <= TX_BURST_SIZE){

		union
		{
			uint64_t as_int;
			struct rte_ether_addr as_addr;
		} dst_eth_addr;
		struct rte_ether_hdr eth_hdr;



		struct rte_mbuf *pkt = rte_pktmbuf_alloc(tx_mbuf_pool);
		// pkt = rte_mbuf_raw_alloc(mbuf_pool);
		if (pkt == NULL)
		{
			printf("trouble at rte_mbuf_raw_alloc\n");
			return;
		}

		// rte_pktmbuf_reset_headroom(pkt);
		rte_pktmbuf_reset(pkt);
		// pkt->data_len = TX_PACKET_LENGTH;

		if(app_id > 0x8){
			pkt->data_len = sizeof(struct rte_ether_hdr) +
						sizeof(struct rte_ipv4_hdr) +
						sizeof(struct rte_ceina_hdr) +
						sizeof(struct rte_ceina_entry1_hdr_1bit_global) +
						sizeof(struct rte_ceina_entry2_hdr_1bit_global);
			pkt_ceina_hdr.quantization_flags = 4 + 1;
		}
		else{
			pkt->data_len = sizeof(struct rte_ether_hdr) +
							sizeof(struct rte_ipv4_hdr) +
							sizeof(struct rte_ceina_hdr) +
							sizeof(struct rte_ceina_entry1_hdr) +
							sizeof(struct rte_ceina_entry2_hdr);
			pkt_ceina_hdr.quantization_flags = 4 + 3;
		}


		// set up addresses
		dst_eth_addr.as_int = rte_cpu_to_be_64(DST_MAC);
		rte_ether_addr_copy(&dst_eth_addr.as_addr, &eth_hdr.d_addr);
		rte_ether_addr_copy(&my_addr, &eth_hdr.s_addr);
		eth_hdr.ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

		pkt_ceina_hdr.bitmap = rte_cpu_to_be_32(bitmap);
		pkt_ceina_hdr.app_id_seq_num = rte_cpu_to_be_32( (quan_model_layer_jobid << 24) | seq_num );
		pkt_ceina_hdr.agtr_idx = rte_cpu_to_be_16(switch_agtr_pos);
		pkt_ceina_hdr.agtr_idx2 = rte_cpu_to_be_16(switch_agtr_pos + MAX_AGTR_SIZE);
		pkt_ceina_hdr.flags = flags;
		if (resend_flag)
		{
			pkt_ceina_hdr.flags |= 0x4; // resend
			RTE_LOG(INFO, PS, "**resent pkt : %d, appid %d, flags %d, quantization_flags %d, agtr_idx %d agtr_idx2 %d, PAS:%d, packet_length: %d \n",
				seq_num,
				app_id,
				pkt_ceina_hdr.flags,
				pkt_ceina_hdr.quantization_flags,
				switch_agtr_pos,
				switch_agtr_pos + MAX_AGTR_SIZE,
				(quan_model_layer_jobid << 24) | seq_num ,
				pkt->data_len);

		}
		else
		{
			RTE_LOG(INFO, PS, "sent pkt : %d, appid %d, flags %d, quantization_flags %d, agtr_idx %d agtr_idx2 %d, PAS:%d, packet_length: %d \n",
				seq_num,
				app_id,
				pkt_ceina_hdr.flags,
				pkt_ceina_hdr.quantization_flags,
				switch_agtr_pos,
				switch_agtr_pos + MAX_AGTR_SIZE,
				(quan_model_layer_jobid << 24) | seq_num ,
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
		if(app_id > 0x8){
			rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *,
											sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ceina_hdr)),
					&pkt_ceina_entry1_1bit_global, (size_t)sizeof(pkt_ceina_entry1_1bit_global));
			rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *,
											sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ceina_hdr) + sizeof(struct rte_ceina_entry1_hdr_1bit_global)),
					&pkt_ceina_entry2_1bit_global, (size_t)sizeof(pkt_ceina_entry2_1bit_global));
		}
		else{
			rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *,
											sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ceina_hdr)),
					&pkt_ceina_entry1, (size_t)sizeof(pkt_ceina_entry1));
			rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *,
											sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_ceina_hdr) + sizeof(struct rte_ceina_entry1_hdr)),
					&pkt_ceina_entry2, (size_t)sizeof(pkt_ceina_entry2));
		}


		// Add some pkt fields
		pkt->nb_segs = 1;
		pkt->pkt_len = pkt->data_len;
		pkt->ol_flags = 0;

		// Actually send the packet
		// pkts_tx_burst[num_pkts_to_send++] = pkt;

		pkts_tx_burst[num_pkts_to_send] = pkt;
		num_pkts_to_send++;
		if(num_pkts_to_send >= MAX_WINDOW_SIZE/2){ ///todo;
			send_packet_in_burst(pkts_tx_burst);
		}

		if(num_pkts_to_send > TX_BURST_SIZE){
			RTE_LOG(WARNING, PS, "ERROR:overflow: seq_num:%d |num_pkts_to_send: %d\n", seq_num, num_pkts_to_send);
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
	total_time = time(NULL) - t1;
	printf("Caught signal %d\n", sig);
	printf("\n=============== Stats =================\n");
	printf("Total packets: %lu\n", packet_count);
	printf("Total transmission time: %ld seconds\n", total_time);
	printf("Average transmission rate: %lu pps\n", packet_count / total_time);
	printf("                           %lu Mbps\n", ((packet_count * TX_PACKET_LENGTH * 8) / total_time) / 1000000);
	printf("=======================================\n");
	exit(0);
}

// static struct window_manager
// {
// 	bool *isACKed;
// 	bool *isSent;
// 	uint32_t total_ACK;
// 	uint32_t last_ACK;
// };

bool inline UpdateWindow(uint32_t seq_num, struct window_manager *window_manager)
{
	bool isLastAckUpdated = false;
	if (window_manager->isACKed[seq_num] == 0)
	{
		window_manager->isACKed[seq_num] = true; // todo

		if (EANBLE_LOG)
			printf("seq_num %d is ACKed \n", seq_num);
	}
	
	while (window_manager->isACKed[window_manager->last_ACK + 1])
	{
		window_manager->last_ACK++;
		isLastAckUpdated = true;
		// printf("window_manager->last_ACK : %d \n", window_manager->last_ACK);
	}


	return isLastAckUpdated;
}

int inline Reset(int packet_total, struct window_manager *window_manager)
{
	window_manager->last_ACK = 0;
	window_manager->isACKed = calloc((MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1), sizeof(bool));
	window_manager->total_ACK = MAX_TENSOR_SIZE / MAX_ENTRIES_PER_PACKET + 1;


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

uint32_t crc32_le(uint32_t crc, unsigned char const *p, size_t len)
{
	while (len--)
	{
		crc ^= *p++;
		for (int i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
	}
	return ~crc;
}

void hash_table_new_crc(struct hash_table *hash_table, uint8_t appID, uint16_t index)
{
	uint8_t crc_input[] = {(uint8_t)(appID & 0xff), (uint8_t)(index & 0xff), (uint8_t)(index >> 8), (uint8_t)(index >> 16), 0, 0};
	int used_size = MAX_AGTR_SIZE;
	uint16_t new_value;
	uint8_t salt = 0;

	do
	{
		new_value = crc32_le(0xffffffff, crc_input, 6); 
		new_value = new_value % used_size;				
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

int HashNew_predefine(struct hash_table *hash_table)
{
	if (hash_table->hash_pos < MAX_AGTR_SIZE)
	{
		hash_table->hash_pos++;
		return hash_table->predefine_agtr_list[hash_table->hash_pos];
	}
	else
	{
		return -1;
	}

}




struct node
{
	struct rte_ceina_hdr *ceina_hdr;
	TAILQ_ENTRY(node)
	entries;
};

TAILQ_HEAD(pending_queue, node)
head; // HEAD node

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

void pending_queue_pop(struct rte_ceina_hdr *dest)
{
	if (!TAILQ_EMPTY(&head))
	{
		struct node *temp_node = TAILQ_FIRST(&head);
		dest = temp_node->ceina_hdr;
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

struct rte_ceina_hdr *pending_queue_front()
{
	if (!TAILQ_EMPTY(&head))
	{
		struct node *temp_node = TAILQ_FIRST(&head);
		return temp_node->ceina_hdr;
	}
	return NULL;
}

int main(int argc, char **argv) 
{
	rte_log_set_global_level(LOG_LEVEL);


	int ret, c;
	uint16_t pkt_data_len;
	int mac_flag = 0, ip_src_flag = 0, ip_dst_flag = 0;
	int counter = 0;

	// DPDK
	uint16_t port_id = PORT_ID;
	uint16_t queue_id = 0;
	uint8_t	quan_model_layer_jobid = 0;

	// Parameters
	int max_agtr_size = MAX_AGTR_SIZE;
	int agtr_start_pos = 0;
	int p4ml_loss_packet = 0;
	int resent_to_be_sent = 0;
	int msgs_completed = 0;

	int total_normal_sent = 0;
	uint64_t full_packet_cnt = 0;
	int full_packet_cnt_normal = 0;
	int partial_packet_cnt = 0;
	int total_resent = 0;
	int total_received_resent_pkts = 0;
	int total_received_normal_pkts = 0;
	int total_loss = 0;
	int total_normal_received = 0;
	uint64_t total_sent_pkts = 0;
	int total_collision_pkts = 0;
	int total_ecn_pkts = 0;
	int total_collision_by_preemption_pkt[20] = {};

	int total_received_pkt[20] = {};
	int total_received_normal_pkt[20] = {};
	int total_full_pkt[20] = {};
	int total_partial_pkt[20] = {};
	int total_collision_pkt[20] = {};
	int total_received_resent_pkt[20] = {};
	int total_sent_pkt[20] = {};
	int total_resent_pkt[20] = {};
	float col_percent;
	int last_full_pkt[20] = {};

	int num_received_pkt[20] = {};
	int num_collision_pkt[20] = {};
	int num_received_resent_pkt[20] = {};
	int num_resent_pkt[20] = {};

	uint64_t num_received_pkts;

	int max_worker = MAX_WORKER;

	int count = 0;
	int total_dup_packet = 0;
	int total_last_tensor_packet = 0;
	int this_pos_to_send = max_agtr_size;
	int rand_index = 0;
	int resend_waiting = 0;

	// TEST
	int test_print_once[20] = {};
	int count_test[20] = {};
	int previous_full_pkt[20] = {};
	bool check[20] = {};




	// int window = WINDOW_SIZE;
	int send_pointer = max_agtr_size;
	int finish_window_seq = max_agtr_size;

	struct timespec t1, t2, current_time, current_time2, current_time3, last_time_receive;
	struct timespec last_receive_time[10];
	struct timespec reset_timer;
	int sec = 0;
	long time_span_sec, time_span_end_file;
	double time[MEASUREMENT_TIME] = {0};
	uint64_t received_bytes_array[MAX_NUM_APPS][MEASUREMENT_TIME] = {0};

	// 0. app 시작 및 기본 설정
	bool app_init[MAX_APP_PER_THREAD] = {0};
	long long int receive_in_sec[20] = {0};
	bool receive_byte_reset_flag[20] = {0};
	int next_agtr[MAX_AGTR_SIZE] = {-1};
	int loss = 0;
	int num_thread = 1;
	int thread_id = 0;

	int check_seq_num = 0;
	int my_tensors_pos = 0;
	uint8_t num_worker = 0;
	uint8_t app_id = 0;

	bool end_signal[10] = {0};





	struct tensor_context *tensors;
	tensors = (struct tensor_context *)malloc(sizeof(struct tensor_context) * MAX_APP_PER_THREAD * MAX_STORAGE_PER_APP_PER_THREAD * num_thread);
	printf("\nTensors memory pre-allocate...\n");
	for (int i = 0; i < MAX_APP_PER_THREAD * MAX_STORAGE_PER_APP_PER_THREAD * num_thread; i++)
	{ ///
		init_tensor(&tensors[i], MAX_TENSOR_SIZE);
	}

	// app start from 1
	int *tensors_pos_of_app = (int *)malloc(sizeof(int) * (MAX_APP_PER_THREAD + 1));

	for (int i = 1; i <= MAX_APP_PER_THREAD; i++)
	{
		tensors_pos_of_app[i] = thread_id * MAX_STORAGE_PER_APP_PER_THREAD * MAX_APP_PER_THREAD + (i - 1) * MAX_STORAGE_PER_APP_PER_THREAD;
		printf("tensors_pos_of_app[%d]: %d \n", i, tensors_pos_of_app[i]);
	}


	// 1. Initialize EAL
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	argc -= ret;
	argv += ret;

	signal(SIGINT, exit_stats);

    char FILE_NAME[80] = "time_throughput";
    char FILE_NAME_SUFFIX[60] = {};
    while ((c = getopt(argc, argv, "m:s:d:hf:")) != -1)
        switch (c)
        {
        case 'm':
            // note, not quite sure why last two bytes are zero, but that is how DPDK likes it
            DST_MAC = 0ULL;
            DST_MAC = string_to_mac(optarg) << 16;
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
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
										MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	tx_mbuf_pool = rte_pktmbuf_pool_create("TX_MBUF_POOL", NUM_MBUFS,
										MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
										

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	if (tx_mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	// Initialize port 0
	if (port_init(port_id, mbuf_pool) != 0)
		rte_exit(EXIT_FAILURE, "Cannot init port 0\n");

	/* Creates new hash table [L612] */
	struct hash_table hash_table;
	hash_table.isAlreadyDeclare = (bool *)malloc(sizeof(bool) * max_agtr_size);
	hash_table.predefine_agtr_list = (int *)malloc(sizeof(int) * max_agtr_size);

	// hash_table.used_size = 0;
	hash_table_init(max_agtr_size, &hash_table);
	for (int i = 0; i < max_agtr_size; i++)
	{
		uint8_t app_id = 1;
		hash_table_new_crc(&hash_table, app_id, i);
	}

	/* 4. Make packet headers*/
	pkt_data_len = (uint16_t)(sizeof(struct rte_ceina_hdr) +
							  sizeof(struct rte_ceina_entry1_hdr) +
							  sizeof(struct rte_ceina_entry2_hdr));

	setup_pkt_headers(&pkt_ip_hdr, &pkt_ceina_hdr, &pkt_ceina_entry1, &pkt_ceina_entry2, pkt_data_len);


	const uint16_t nb_pkts = RX_BURST_SIZE;

    unsigned int max_lcores = rte_lcore_count();
    printf("Maximum number of lcores: %u\n", max_lcores);




	/* Main packet send and receive loop*/
	/* Main packet send and receive loop*/
	/* Main packet send and receive loop*/
	int pkt_count = 0;
	int count2 = 0;

	for (int app=0; app<10; app++){
		clock_gettime(CLOCK_MONOTONIC ,&last_receive_time[app]);
	}

	struct rte_mbuf *pkts_tx_burst[TX_BURST_SIZE] = {NULL}; ///FIXME:
	clock_gettime(CLOCK_MONOTONIC, &reset_timer);
	clock_gettime(CLOCK_MONOTONIC, &last_time_receive);
	time[0] = (double)(reset_timer.tv_sec * 1000000000L + reset_timer.tv_nsec) / 1000000000.0;


	while (1)
	{
		rte_log_set_global_level(log_level); ////FIXME: tobedel

		struct rte_mbuf *pkts_rx_burst[RX_BURST_SIZE] = {NULL}; ///FIXME:
		// struct rte_mbuf *pkts_tx_burst[TX_BURST_SIZE] = {NULL}; ///FIXME:


		msgs_completed = 0;
		clock_gettime(CLOCK_MONOTONIC, &t1);

		/* 1-1 */
		// printf("Line %d 1-1 \n", 759);
		while (1)
		{
			clock_gettime(CLOCK_MONOTONIC, &t2);
			long time_span = (t2.tv_sec - t1.tv_sec) * 1000000000L + (t2.tv_nsec - t1.tv_nsec);

			msgs_completed = rte_eth_rx_burst(port_id, queue_id, pkts_rx_burst, nb_pkts);
			num_received_pkts += msgs_completed;
			if (msgs_completed)
			{
				break;
			}
			
			count2++;
			if ( unlikely(count2 % 100000000 == 0) ){
				RTE_LOG(NOTICE, PS, "num_received: %llu |full: %llu |full_normal:%d |partial: %d |total_received_resent: %d |total_collision: %d\n", 
					num_received_pkts, full_packet_cnt, full_packet_cnt_normal, partial_packet_cnt, total_received_resent_pkts, total_collision_pkts);
				count2 = 0;
			}

			///todo de
			if(time_span > TRAINING_TIME * 1000000000L){
				RTE_LOG(NOTICE, PS, "time span : %d \n", TRAINING_TIME);
				break;
			}

			// time_span > 20.0 //todo
			// partial aggregation
		}

		int to_be_sent = 0;

		/* 1-2 */
		for (int msg = 0; msg < msgs_completed; msg++)
		{
			/* 1-2-1 Header Parsing */
			// struct rte_mbuf *mbuf = pkts_rx_burst[msg]; ///
			pkt_count++;

			struct rte_mbuf *mbuf = pkts_rx_burst[msg];
			rte_prefetch0(rte_pktmbuf_mtod(mbuf, void *)); // 성능 향상을 위함
			pkts_rx_burst[msg] = NULL;
			

			struct rte_ether_hdr *ether = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *); // Ethernet
			struct rte_ipv4_hdr *ipv4 = (struct rte_ipv4_hdr *)((uint8_t *)ether + sizeof(struct rte_ether_hdr));

			if ((ipv4->next_proto_id != 0))
			{
				num_received_pkts--;
				RTE_LOG(INFO, PS, "Not ceina packet \n");
				rte_pktmbuf_free(mbuf);
				break;
			}


			struct rte_ceina_hdr *ceina_hdr = (struct rte_ceina_hdr *)((uint8_t *)ipv4 + sizeof(struct rte_ipv4_hdr));

			uint32_t src_addr = rte_be_to_cpu_32(ipv4->src_addr);
			uint32_t dst_addr = rte_be_to_cpu_32(ipv4->dst_addr);
			
			uint32_t bitmap = rte_be_to_cpu_32(ceina_hdr->bitmap);
			uint8_t agtr_time = ceina_hdr->agtr_time;
			uint8_t flags = ceina_hdr->flags;
			bool is_collision_packet = ceina_hdr->flags & 0x02;
			bool isResend_packet = ceina_hdr->flags & 0x04;
			bool isECN_packet = ceina_hdr->flags & 0x08;
			bool isOverflow_packet = ceina_hdr->flags & 0x80;

			uint8_t	quan_model_layer_jobid = (uint8_t)(rte_be_to_cpu_32(ceina_hdr->app_id_seq_num) >> 24);
			uint8_t app_id = (uint8_t)(rte_be_to_cpu_32(ceina_hdr->app_id_seq_num) >> 24) & 0xF;
			uint8_t appID = app_id & 0x7; // appid for array access
			uint32_t seq_num = rte_be_to_cpu_32(ceina_hdr->app_id_seq_num) & 0x00FFFFFF;
			uint8_t quantization_flags = ceina_hdr->quantization_flags;

			num_received_pkt[appID]++;
			total_received_pkt[appID]++;
									
			check_seq_num = seq_num;

			uint16_t agtr_idx = rte_be_to_cpu_16(ceina_hdr->agtr_idx);
			uint16_t agtr_idx2 = rte_be_to_cpu_16(ceina_hdr->agtr_idx2);
			
			uint8_t num_worker = agtr_time;
			max_worker = agtr_time;

			uint16_t packet_length = mbuf->data_len;
			received_bytes_array[appID][sec] = received_bytes_array[appID][sec] + packet_length;

			// RTE_LOG(INFO, PS, "---------------------------mbuf: %p ---------------------------------------msgs: (%d / %d) / %d \n", mbuf, msg+1, msgs_completed, num_received_pkts);
			RTE_LOG(INFO, PS, "Received: seq %d, app_id %d, bmap %d, agtr_time %d, agtr_idx %d, agtr_idx2 %d, f %d, level: %d, length: %d \n", 
					seq_num, app_id, bitmap, agtr_time, agtr_idx, agtr_idx2, flags, quantization_flags, packet_length);
			


			if(app_id <= 0x8){
				struct rte_ceina_entry1_hdr *ceina_entry1 = (struct rte_ceina_entry1_hdr *)((uint8_t *)ceina_hdr + sizeof(struct rte_ceina_hdr));
				struct rte_ceina_entry2_hdr *ceina_entry2 = (struct rte_ceina_entry2_hdr *)((uint8_t *)ceina_entry1 + sizeof(struct rte_ceina_entry1_hdr));
			}
			else if(app_id > 0x8){
				if(isResend_packet){
					struct rte_ceina_entry1_hdr_1bit *ceina_entry1 = (struct rte_ceina_entry1_hdr_1bit *)((uint8_t *)ceina_hdr + sizeof(struct rte_ceina_hdr));
					struct rte_ceina_entry2_hdr_1bit *ceina_entry2 = (struct rte_ceina_entry2_hdr_1bit *)((uint8_t *)ceina_entry1 + sizeof(struct rte_ceina_entry1_hdr_1bit));
				}
				else{
					struct rte_ceina_entry1_hdr_1bit_global *ceina_entry1 = (struct rte_ceina_entry1_hdr_1bit_global *)((uint8_t *)ceina_hdr + sizeof(struct rte_ceina_hdr));
					struct rte_ceina_entry2_hdr_1bit_global *ceina_entry2 = (struct rte_ceina_entry2_hdr_1bit_global *)((uint8_t *)ceina_entry1 + sizeof(struct rte_ceina_entry1_hdr_1bit_global));
				}
				
			}
			


			if ( isResend_packet ){
				total_received_resent_pkt[appID]++;
				num_received_resent_pkt[appID]++;
				total_received_resent_pkts++;
			}
			else{
				total_received_normal_pkt[appID]++;
			}



			// 1-2-3 App Init

			if ( unlikely(!app_init[appID]) )
			{
				app_init[appID] = true;
			}
			else
			{
			}

			if ( !hash_table.isAlreadyDeclare[agtr_idx] ) 
				hash_table.isAlreadyDeclare[agtr_idx] = true;

			/* 1-2-5 Check if Collision packet */
			
			if ( is_collision_packet )
			{
				tensors[tensors_pos_of_app[appID]].isCollision[seq_num] = true;
				total_collision_pkts++;
				total_collision_pkt[appID]++;
				num_collision_pkt[appID]++;
				
			}

			if ( isECN_packet )
			{
				// RTE_LOG(WARNING, PS, "pkt %d isECN_packet \n", seq_num);
				total_ecn_pkts++;
			}


			my_tensors_pos = tensors_pos_of_app[appID];
			// int my_tensors_pos = 0; // todo;
			// todo check_tensor_available(&tensors[my_tensors_pos], p4ml_header, thread_id);


			/* 1-2-6 Check Full Packet */
			bool isFullPacket = (1 << ceina_hdr->agtr_time) - 1 == rte_cpu_to_be_32(ceina_hdr->bitmap) ? 1 : 0;
			if (receive_byte_reset_flag)
			{
				receive_in_sec[appID] = 0;
				receive_byte_reset_flag[appID] = false;
			}
			/* 1-2-7 Full packet인 경우 */
			if (isFullPacket)
			{
				full_packet_cnt++;
				total_full_pkt[appID]++;

				if(!isResend_packet){
					full_packet_cnt_normal++;
				}

				if (EANBLE_LOG)
					printf("pkt %d is full packet \n", seq_num);

				// todo updateModel_force(ceina_hdr, &tensors[my_tensors_pos]);

				for (int i = 0; i < ceina_hdr->agtr_time; i++)
				{
					UpdateWindow(seq_num, &tensors[my_tensors_pos].window_manager_arr[i]); // TODO:
				}

			}

			// 1-2-8 partial packet인 경우
			else
			{
				partial_packet_cnt++;
				total_partial_pkt[appID]++;
				
				
				bool type_consistent = true; //FIXME;
				if (type_consistent)
				{
					int valid_bit = 1;
					bool need_to_update = true;
					// check if update is needed
					for (int i = 0; i < num_worker; i++)
					{
						if (valid_bit & bitmap)
						{
							if (tensors[my_tensors_pos].window_manager_arr[i].isACKed[seq_num])
							{
								need_to_update = false;
								break;
							}
						}
						valid_bit <<= 1;
					}
					if (need_to_update)
					{
						int valid_bit = 1;
						for (int i = 0; i < num_worker; i++)
						{
							if (valid_bit & bitmap)
							{
								UpdateWindow(seq_num, &tensors[my_tensors_pos].window_manager_arr[i]);
							}
							valid_bit <<= 1;
						}
						
					}
				}
			} // if partial packet

			bool is_slot_completed = true;
			// 1-2-9 미완성한 워커가 있는지 슬롯 체크
			for (int i = 0; i < ceina_hdr->agtr_time; i++){
				if (!tensors[my_tensors_pos].window_manager_arr[i].isACKed[seq_num]) {
					is_slot_completed = false;
					RTE_LOG(INFO, PS, "slot %d is not completed, (worker %d is not received) \n", seq_num, i);
				}
			}

			// 1-2-10 슬롯 완료되었으면
			uint32_t new_bitmap = 7;
			if (is_slot_completed)
			{
				RTE_LOG(DEBUG, PS, "pkt %d is_slot_completed, it will be sent \n", seq_num);
				uint16_t new_agtr;

				// 1-2-10-1 if collsion is happened //todo
				if(CHANGE_AGTR_ENABLE){
					if (tensors[my_tensors_pos].isCollision[seq_num] == true) {
						if (next_agtr[agtr_idx] == -1) {
							int new_hash_agtr = HashNew_predefine(&hash_table);
							// if get any of AGTR from hash
							if (new_hash_agtr != -1) {
									new_agtr = new_hash_agtr;
									next_agtr[agtr_idx] = new_agtr;
									hash_table.hash_map[agtr_idx] = new_agtr;
							}
							else{
								new_agtr = agtr_idx;
							}
						}
						else{
							new_agtr = next_agtr[agtr_idx];
						}
						flags = flags | 0x02; // collision
						new_bitmap = (uint32_t)new_agtr;
					}
					else{
						flags = flags & ~(1 << 1);
					}
				}


				// 1-2-10-3 재전송 패킷이면 바로 재전송
				if (isResend_packet)
				{

					bool resend_flag = 1;
					uint8_t flags = 0; 
					flags = flags | 0x01; // ACK
					flags = flags | (tensors[my_tensors_pos].isCollision[seq_num] << 1); // Collision
					flags = flags | 0x04; // Resend
					flags = flags | (isECN_packet << 3) ; // ECN
					send_packet(seq_num, agtr_idx, new_bitmap, resend_flag, flags, port_id, app_id, quan_model_layer_jobid, tx_mbuf_pool);
					pkt_count = 0;

					total_resent++;
					total_resent_pkt[appID]++;
					num_resent_pkt[appID]++;
					total_sent_pkts++;
					total_sent_pkt[appID]++;
				}
				// 1-2-10-4 재전송 패킷 아니면
				else
				{

					bool resend_flag = 0;
					uint8_t flags = 0; 
					flags = flags | 0x01; // ACK
					flags = flags | (tensors[my_tensors_pos].isCollision[seq_num] << 1); // Collision
					flags = flags | (isECN_packet << 3) ; // ECN
					send_packet(seq_num, agtr_idx, new_bitmap, resend_flag, flags, port_id, app_id, quan_model_layer_jobid, tx_mbuf_pool); //TODO:
					total_sent_pkts++;
					total_normal_sent++;
					total_sent_pkt[appID]++;

				}

			}
			
			if(app_id <= 0x8 && seq_num == MAX_SEQ_NUM && num_received_pkt[appID] > (MAX_SEQ_NUM-1000)){
				end_signal[appID] = true;
				clock_gettime(CLOCK_MONOTONIC, &last_receive_time[appID]); 
			}

			if(app_id > 0x8 && seq_num == MAX_SEQ_NUM_1bit && num_received_pkt[appID] > (MAX_SEQ_NUM_1bit-1000) ){
				end_signal[appID] = true;
				clock_gettime(CLOCK_MONOTONIC, &last_receive_time[appID]); 
			}


			rte_pktmbuf_free(mbuf);

		}
		

		if (msgs_completed < 0)
		{
			printf("Polling error\n");
			exit(1);
		}


		if (msgs_completed > 0)
		{
			send_packet_in_burst(); ///FIXED
			clock_gettime(CLOCK_MONOTONIC, &last_time_receive);

		}





		clock_gettime(CLOCK_MONOTONIC, &current_time3);
		time_span_end_file = (current_time3.tv_sec - last_time_receive.tv_sec)*1000000000L + (current_time3.tv_nsec - last_time_receive.tv_nsec);		
		if(time_span_end_file >= 10000000000L){ // 10s
			char LOG_FILE_NAME[80] = "pkts_count";
			if (strcmp(FILE_NAME_SUFFIX, "") != 0 ){
				strcat(LOG_FILE_NAME, FILE_NAME_SUFFIX);
				strcat(LOG_FILE_NAME, ".txt");
			}
			else{
				strcat(LOG_FILE_NAME, ".txt");
			}			
			FILE *file_ptr;
			file_ptr = fopen(LOG_FILE_NAME, "w");

			fprintf(file_ptr, "seq: %d| recv_pkts: %llu |full_pkt: %llu |partial_packet: %d |col: %d |ecn: %d |sent_pkts: %llu |normal_sent: %d |resent : %d \n",
				check_seq_num,
				num_received_pkts,
				full_packet_cnt,
				partial_packet_cnt,
				total_collision_pkts,
				total_ecn_pkts,
				total_sent_pkts,
				total_normal_sent,
				total_resent
				);

			for(int i=1; i<MAX_NUM_APPS; i++){
				fprintf(file_ptr, "job %d PER_ROUND recv_pkts:%d |col:%d |recv_resent_pkt:%d |resent_pkt:%d \n", 
					i, num_received_pkt[i], num_collision_pkt[i], num_received_resent_pkt[i], num_resent_pkt[i]);
			}


			for(int i=1; i<MAX_NUM_APPS; i++){
			
				float col_percent = 0;
				if(num_received_pkts != 0 ){
					col_percent = (double)total_collision_pkt[i] / (double)num_received_pkts * 100;
				}
				fprintf(file_ptr, "job %2d TOTAL recv:%10d normal:%10d full:%10d partial:%9d col:%9d (%.2f%%) recv_resent:%9d sent:%10d resent:%9d \n", 
					i, 
					total_received_pkt[i], 
					total_received_normal_pkt[i],
					total_full_pkt[i],
					total_partial_pkt[i],
					total_collision_pkt[i],
					col_percent,
					total_received_resent_pkt[i], 
					total_sent_pkt[i],
					total_resent_pkt[i]);

				float sent_traffic;
				float recv_full_traffic;
				if (total_received_pkt[i] < MAX_SEQ_NUM * NUM_ROUND){ // 1bit
					recv_full_traffic = (float)total_full_pkt[i] / 1024 / 1024 / 1024 * 113;
					sent_traffic = (float)total_sent_pkt[i] / 1024 / 1024 / 1024 * 113;
				}
				else{ // 32bit 
					recv_full_traffic = (float)total_full_pkt[i] / 1024 / 1024 / 1024 * 305;
					sent_traffic = (float)total_sent_pkt[i] / 1024 / 1024 / 1024 * 305;
				}
				float recv_partial_traffic = (float)total_partial_pkt[i] / 1024 / 1024 / 1024 * 305;
				float recv_traffic = recv_full_traffic + recv_partial_traffic;
				float collision_traffic = (float)total_collision_pkt[i] / 1024 / 1024 / 1024 * 305;

				fprintf(file_ptr, "sent_traffic_job%d : %.4f GB \n", i, sent_traffic);
				fprintf(file_ptr, "recv_traffic_job%d : %.4f GB \n", i, recv_traffic);
				fprintf(file_ptr, "collision_traffic_job%d : %.6f GB \n", i, collision_traffic);


			}

			fclose(file_ptr);
			RTE_LOG(WARNING, PS, "========== %s file is saved \n", LOG_FILE_NAME);
			clock_gettime(CLOCK_MONOTONIC, &last_time_receive);

		}






		count++;

		if(LOG_LEVEL == 7){
			RTE_LOG(INFO, PS, "received_pkts: %llu full_packet: %d  partial_packet: %d total_collision_pkts: %d total_normal_sent: %d  total_resent : %d \n",
				num_received_pkts,
				full_packet_cnt,
				partial_packet_cnt,
				total_collision_pkts,
				total_normal_sent,
				total_resent);
		}

		if(LOG_LEVEL == 6){
			if(count % 100000 == 0 || check_seq_num == MAX_SEQ_NUM || check_seq_num == MAX_SEQ_NUM_1bit){
				RTE_LOG(NOTICE, PS, "----------------------------------------------------------------- \n");
				RTE_LOG(NOTICE, PS, "seq: %d| recv_pkts: %llu |full_pkt: %d |partial_packet: %d |col: %d |ecn: %d |sent_pkts: %d |normal_sent: %d |resent : %d \n",
					check_seq_num,
					num_received_pkts,
					full_packet_cnt,
					partial_packet_cnt,
					total_collision_pkts,
					total_ecn_pkts,
					total_sent_pkts,
					total_normal_sent,
					total_resent);

				for(int i=1; i<MAX_NUM_APPS; i++){
					RTE_LOG(NOTICE, PS, "job %d recv_pkts:%d |collision:%d |recv_resent_pkt:%d |resent_pkt:%d \n", 
						i, num_received_pkt[i], total_collision_pkt[i], total_received_resent_pkt[i], total_resent_pkt[i]);
				}

			}
		}

		if(LOG_LEVEL == 5){
			if(count % 200000 == 0 || check_seq_num == MAX_SEQ_NUM || check_seq_num == MAX_SEQ_NUM_1bit){

				RTE_LOG(WARNING, PS, "-----------------------------------------------------------------sec:%d \n", sec);
				RTE_LOG(WARNING, PS, "seq: %d| recv_pkts: %llu |full_pkt: %llu |partial_packet: %d |col: %d |ecn: %d |sent_pkts: %llu |normal_sent: %d |resent : %d \n",
					check_seq_num,
					num_received_pkts,
					full_packet_cnt,
					partial_packet_cnt,
					total_collision_pkts,
					total_ecn_pkts,
					total_sent_pkts,
					total_normal_sent,
					total_resent
					);

				for(int i=1; i<MAX_NUM_APPS; i++){
					RTE_LOG(WARNING, PS, "job %d PER_ROUND recv_pkts:%d |col:%d |recv_resent_pkt:%d |resent_pkt:%d \n", 
						i, num_received_pkt[i], num_collision_pkt[i], num_received_resent_pkt[i], num_resent_pkt[i]);
				}
				for(int i=1; i<MAX_NUM_APPS; i++){
					RTE_LOG(WARNING, PS, "job %2d TOTAL recv:%10d normal:%10d full:%10d partial:%9d col:%9d recv_resent:%9d sent:%10d resent:%9d \n", 
						i, 
						total_received_pkt[i], 
						total_received_normal_pkt[i],
						total_full_pkt[i],
						total_partial_pkt[i],
						total_collision_pkt[i], 
						total_received_resent_pkt[i], 
						total_sent_pkt[i],
						total_resent_pkt[i]);
				}
			}
		}

		
		for (int app=1; app< MAX_NUM_APPS; app++){
			if(end_signal[app]){
				clock_gettime(CLOCK_MONOTONIC ,&current_time);
				long elapsed_time_end = (current_time.tv_sec-last_receive_time[app].tv_sec) * 1000000000L 
					+ (current_time.tv_nsec - last_receive_time[app].tv_nsec);
				float time_span_end = (float)elapsed_time_end / 1000000000.0;
					
				if(time_span_end > TRAINING_TIME){
					

					for (int i = 0; i < MAX_WORKER; i++){
						free(tensors[tensors_pos_of_app[app]].window_manager_arr[i].isACKed);
						Reset(MAX_SEQ_NUM, &tensors[tensors_pos_of_app[app]].window_manager_arr[i]);
					}
					
					reset_tensor(&tensors[tensors_pos_of_app[app]], MAX_TENSOR_SIZE);

					RTE_LOG(WARNING, PS, ">>>>> app_id:%d resets window / tensors_pos_of_app[app]:%d \n", 
						app, tensors_pos_of_app[app]);

					num_received_pkt[app] = 0;
					num_collision_pkt[app] = 0;
					num_received_resent_pkt[app] = 0;
					num_resent_pkt[app] = 0;
					test_print_once[app] = 0;
					end_signal[app] = 0;
					clock_gettime(CLOCK_MONOTONIC ,&last_receive_time[app]);
					// log_level = 7; ////FIXME
				}
				
			}
		}


	}

	return (0);
}