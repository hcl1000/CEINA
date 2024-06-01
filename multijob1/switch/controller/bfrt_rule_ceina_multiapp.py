# $SDE/run_bfshell.sh -b bfrt_rule_atp.py

from ipaddress import ip_address
import time

############################### SETTING ###############################
############################### SETTING ###############################
_MODIFY_CONT2_ = 1

###############################         ###############################

p4 = bfrt.ceina.pipe.Ingress


# port_of_worker = [0, 1, 2, 3] #65x tofino : mncgpu2, mncgpu3 / last worker is PS

port_of_worker = []
PSs = []
PAS = []
len_workers = []
len_PS = []
mc_group = []
# PAS_mask=0xFF000000
PAS_mask=0x0F000000

# Job1
port_of_worker.append([0, 168, 169, 170, 171]) # 0 is padding / #tofino_32x : host5~host7
PSs.append ([0,4]) 
# PAS.append(0x02000000)
PAS.append(0x0A000000)

# Job2
port_of_worker.append([0, 184, 185, 186, 171]) # 0 is padding / #tofino_32x : mncgpu123
PSs.append ([0,4]) 
# PAS.append(0x03000000)
PAS.append(0x0B000000)

# Job3
port_of_worker.append([0, 176, 177, 171]) # 0 is padding / #tofino_32x : host9, host10, host8
PSs.append ([0,3])
# PAS.append(0x01000000) 
PAS.append(0x09000000)


num_app = len(PAS)
print(f"PSs : {PSs}")
print(f"PAS : {PAS}")
print(f"port_of_worker : {port_of_worker}")

for k in range(num_app):
    len_workers.append(len(port_of_worker[k]))
    len_PS.append(len(PSs[k]))
    mc_group.append(999-k)
    print(f"len_PS[k] : {len_PS[k]}")


# single_loopback_port = 68
single_loopback_port = 196


p4.forward.add_with_set_egr(
        dstAddr=0x000000000001,
        egress_spec=171
)


for k in range(num_app):

    # outPort Table
    for i in range(1, len_workers[k]):
        for j in range(1, len_PS[k]):
            print("outPort")
            p4.outPort_table.add_with_set_egr_and_set_index(
                PAS=PAS[k], PAS_mask=PAS_mask,
                ingress_port=port_of_worker[k][i],
                dataIndex=0, # 
                PSIndex=j-1,
                isAggregate=0, isAggregate_mask=0x0,
                agtr_complete=0x0, agtr_complete_mask=0x0,
                
                egress_spec=single_loopback_port
            ),
        print("outPort Completed")
    
    
    for j in range(1, len_PS[k]):
        print("outPort1-2")
        p4.outPort_table.add_with_set_egr_and_set_index_queue(
            PAS=PAS[k], PAS_mask=PAS_mask,
            ingress_port= single_loopback_port,
            dataIndex=1, # after recirculation
            PSIndex=j-1,
            isAggregate=0, isAggregate_mask=0x0,
            agtr_complete=0xFF, agtr_complete_mask=0xFF,
            MATCH_PRIORITY=2,
            
            egress_spec=port_of_worker[k][PSs[k][j]],
            qid=7
        ),
    print("outPort1-2 Completed")

    

    #todo outport2
    for i in range(1, len_workers[k]):
        for j in range(1, len_PS[k]):
            p4.outPort_table2.add_with_set_egr_and_set_index(
                PAS=PAS[k], PAS_mask=PAS_mask,
                ingress_port=port_of_worker[k][i],
                dataIndex=0, # 
                PSIndex=j-1,
                isAggregate=0, isAggregate_mask=0x0,
                agtr_complete=0x0, agtr_complete_mask=0x0,
                
                egress_spec=single_loopback_port
                # egress_spec=port_of_worker[PSs[j]] ###test
            ),
    print("outPort2 Completed")
    
    for j in range(1, len_PS[k]):
        p4.outPort_table2.add_with_set_egr_and_set_index(
            PAS=PAS[k], PAS_mask=PAS_mask,
            ingress_port=single_loopback_port,
            dataIndex=1, # 
            PSIndex=j-1,
            isAggregate=0, isAggregate_mask=0x0,
            agtr_complete=0x0, agtr_complete_mask=0x0,
            
            egress_spec=port_of_worker[k][PSs[k][j]]
            # egress_spec=port_of_worker[PSs[j]] ###test
        ),
    print("outPort2-2 Completed")



    # INGRESSPORT, Index
    # Worker1 to Worker8
    for i in range(1, len_workers[k] - 1):
        print("drop_table")
        p4.drop_table.add_with_drop_pkt(
            ingress_port=port_of_worker[k][i],
            dataIndex=1,
            isAggregate=0, isAggregate_mask=0x0, # anything
            agtr_complete=0x0, agtr_complete_mask=0x0,
            resubmit_flag=1
        )

    ####### Server ########
    for j in range(1, len_PS[k]):
        print("multicast_table")
        p4.multicast_table.add_with_multicast(
            isACK=1,
            PAS=PAS[k], PAS_mask=PAS_mask,
            ingress_port=single_loopback_port, ###
            dataIndex=1, #TODO: Why? ->-> recircul 후로 수정
            
            group=mc_group[k]
        )
        print("multicast_table completed")






p4.modify_packet_bitmap_table.add_with_nop(
        dataIndex=1,
        isAggregate=0, isAggregate_mask=0xFFFFFFFF,
        agtr_complete=0x0, agtr_complete_mask=0x0
)
p4.modify_packet_bitmap_table.add_with_nop(
        dataIndex=1,
        isAggregate=0, isAggregate_mask=0xFFFFFFFF,
        agtr_complete=0xFF, agtr_complete_mask=0xFF
)
p4.modify_packet_bitmap_table.add_with_nop(
        dataIndex=0,
        isAggregate=0, isAggregate_mask=0x0,
        agtr_complete=0, agtr_complete_mask=0x0,
        MATCH_PRIORITY=1
)








if _MODIFY_CONT2_ == 0:
    for i in range(1,33):
        try:
            number = i
            func1 = "p4.NewprocessEntry{0}.add_with_entry{0}WriteToPacket(".format(number) + \
                    "isAggregate=0, isAggregate_mask=0xFFFFFFFF," + \
                    "agtr_complete=0xFF, agtr_complete_mask=0xFF," + \
                    "bitmap=0, bitmap_mask=0x0," + \
                    "MATCH_PRIORITY=1)"
                    
            func2 ="p4.NewprocessEntry{0}.add_with_processentry{0}(".format(number) + \
                    "isAggregate=0, isAggregate_mask=0x0," + \
                    "agtr_complete=0, agtr_complete_mask=0x0," + \
                    "bitmap=0, bitmap_mask=0xFFFFFFFF," + \
                    "MATCH_PRIORITY=3)"

            func3 ="p4.NewprocessEntry{0}.add_with_noequ0_processentry{0}(".format(number) + \
                    "isAggregate=0, isAggregate_mask=0x0," + \
                    "agtr_complete=0, agtr_complete_mask=0x0," + \
                    "bitmap=0, bitmap_mask=0x0," + \
                    "MATCH_PRIORITY=4)"

            func4 ="p4.NewprocessEntry{0}.add_with_noequ0_processentry{0}andWriteToPacket(".format(number) + \
                    "isAggregate=0, isAggregate_mask=0x0," + \
                    "agtr_complete=0xFF, agtr_complete_mask=0xFF," + \
                    "bitmap=0, bitmap_mask=0x0," + \
                    "MATCH_PRIORITY=2)"
            exec(func1)
            exec(func2)
            exec(func3)
            exec(func4)
            print("function started : {0}".format(i))
        except:
            print("can't insert rules into NewpprocessEntry tables")
            # quit()
else:

    p4.bitmap_table.add_with_process_bitmap_preemption(
        isMyPAS=0x0,
        isMyPAS_mask=0x0,
        MATCH_PRIORITY=2
    )
    p4.bitmap_table.add_with_process_bitmap(
        isMyPAS=0x0,
        isMyPAS_mask=0xFFFFFFFF,
        MATCH_PRIORITY=1
    )

    p4.agtr_time_table.add_with_check_agtr_time_preemption(
        isMyPAS=0x0,
        isMyPAS_mask=0x0,
        MATCH_PRIORITY=2
    )    
    p4.agtr_time_table.add_with_check_agtr_time(
        isMyPAS=0x0,
        isMyPAS_mask=0xFFFFFFFF,
        MATCH_PRIORITY=1
    )        
    
    for i in range(1,33):
        try:
            number = i
            func1 = "p4.NewprocessEntry{0}.add_with_entry{0}WriteToPacket(".format(number) + \
                    "isAggregate=0, isAggregate_mask=0xFFFFFFFF," + \
                    "agtr_complete=0xFF, agtr_complete_mask=0xFF," + \
                    "bitmap=0, bitmap_mask=0x0," + \
                    "isMyPAS=0, isMyPAS_mask=0xFFFFFFFF," + \
                    "MATCH_PRIORITY=1)"
                    
            func2 ="p4.NewprocessEntry{0}.add_with_processentry{0}(".format(number) + \
                    "isAggregate=0, isAggregate_mask=0x0," + \
                    "agtr_complete=0, agtr_complete_mask=0x0," + \
                    "bitmap=0, bitmap_mask=0xFFFFFFFF," + \
                    "isMyPAS=0, isMyPAS_mask=0xFFFFFFFF," + \
                    "MATCH_PRIORITY=4)"

            func3 ="p4.NewprocessEntry{0}.add_with_noequ0_processentry{0}(".format(number) + \
                    "isAggregate=0, isAggregate_mask=0x0," + \
                    "agtr_complete=0, agtr_complete_mask=0x0," + \
                    "bitmap=0, bitmap_mask=0x0," + \
                    "isMyPAS=0, isMyPAS_mask=0xFFFFFFFF," + \
                    "MATCH_PRIORITY=5)"

            func4 ="p4.NewprocessEntry{0}.add_with_noequ0_processentry{0}andWriteToPacket(".format(number) + \
                    "isAggregate=0, isAggregate_mask=0x0," + \
                    "agtr_complete=0xFF, agtr_complete_mask=0xFF," + \
                    "bitmap=0, bitmap_mask=0x0," + \
                    "isMyPAS=0, isMyPAS_mask=0xFFFFFFFF," + \
                    "MATCH_PRIORITY=2)"
            exec(func1)
            exec(func2)
            exec(func3)
            exec(func4)
                    
            func5 ="p4.NewprocessEntry{0}.add_with_processentry{0}(".format(number) + \
                    "isAggregate=0, isAggregate_mask=0x0," + \
                    "agtr_complete=0, agtr_complete_mask=0x0," + \
                    "bitmap=0, bitmap_mask=0xFFFFFFFF," + \
                    "isMyPAS=0x80000000, isMyPAS_mask=0x80000000," + \
                    "MATCH_PRIORITY=3)"
                    
            exec(func5)
            
            print("function started : {0}".format(i))
        except:
            print("can't insert rules into NewpprocessEntry tables")
            # quit()
            





for k in range(num_app):
    mgid = mc_group[k]
    node_id = k+1
    rid = k+1 # which means multicast node id
    port_list = port_of_worker[k][1:-1]
    print(port_list)

    # Create multicast group 
    bfrt.pre.mgid.add(MGID=mgid)
    # Create multicast node
    bfrt.pre.node.add(
        MULTICAST_NODE_ID=node_id,
        MULTICAST_RID=rid,
        DEV_PORT=port_list
    )
    # Extend multicast group (Associate multicast node with multicast group)
    bfrt.pre.mgid.mod_inc(
        MGID=mgid,
        MULTICAST_NODE_ID=[node_id],
        MULTICAST_NODE_L1_XID_VALID=[True],
        MULTICAST_NODE_L1_XID=[rid]        
    )



p4_egress = bfrt.ceina.pipe.Egress

def generate_hex_bin (index):
    num_worker = 2
    half_worker = int(num_worker/2) + 1
    bin_bitmap = [0,0,0,0,0,0,0,0]
    hex_bitmap = [0,0,0,0,0,0,0,0]
    if(index & 128):
        hex_bitmap[0] = half_worker
        bin_bitmap[0] = 1
    if(index & 64):
        hex_bitmap[1] = half_worker
        bin_bitmap[1] = 1
    if(index & 32):
        hex_bitmap[2] = half_worker
        bin_bitmap[2] = 1
    if(index & 16):
        hex_bitmap[3] = half_worker
        bin_bitmap[3] = 1
    if(index & 8):
        hex_bitmap[4] = half_worker
        bin_bitmap[4] = 1
    if(index & 4):
        hex_bitmap[5] = half_worker
        bin_bitmap[5] = 1
    if(index & 2):
        hex_bitmap[6] = half_worker
        bin_bitmap[6] = 1
    if(index & 1):
        hex_bitmap[7] = half_worker
        bin_bitmap[7] = 1
    hex = '0x'
    bin = '0b'
    hex_mask = '0x'
    hex_mask_ = [half_worker for i in range(8)]
    for i in range(8):
        hex = hex + str(hex_bitmap[i])
        bin = bin + str(bin_bitmap[i])
        hex_mask = hex_mask + str(hex_mask_[i])
    
    return hex, bin, hex_mask
    
for i in range(1,33):
    try:
        for j in range(256):
            hex, bin, hex_mask = generate_hex_bin(j)
            func1 = f"p4_egress.voting_table{i}.add_with_voting_action{i}(" + \
                    "isAggregate=0, isAggregate_mask=0x0," + \
                    "agtr_complete=0xFF, agtr_complete_mask=0xFF," + \
                    "bitmap=0, bitmap_mask=0x0," + \
                    f"data{i-1}={hex}, data{i-1}_mask={hex_mask}," + \
                    f"voting_result={bin}," +  \
                    f"MATCH_PRIORITY={j+1})"
            exec(func1)
            # print(f"voting_table{i} rule inserted : {hex} -> {bin}")
        print(f"voting_table{i} rule inserted")
    except:
        print("can't insert rules into voting_tables")
        # quit()
        
