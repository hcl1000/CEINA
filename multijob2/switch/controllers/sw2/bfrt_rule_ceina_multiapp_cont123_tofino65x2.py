# $SDE/run_bfshell.sh -b bfrt_rule_atp.py

from ipaddress import ip_address
import time

p4 = bfrt.ceina.pipe.Ingress

## job1
p4.forward.add_with_set_egr( # global
        dstAddr=0x000000000001,
        protocol = 0,
        protocol_mask = 0xFF,
        isACK = 1,
        isACK_mask = 1,
        
        egress_spec=180, # ->tofino65x1
        qid=2
)
p4.forward.add_with_set_egr( # local
        dstAddr=0x000000000001,
        protocol = 0,
        protocol_mask = 0xFF,
        isACK = 0,
        isACK_mask = 1,
        
        egress_spec=184, # ->tofino65x1
        qid=1
)

## job2
p4.forward.add_with_set_egr( # global
        dstAddr=0x000000000002,
        protocol = 0,
        protocol_mask = 0xFF,
        isACK = 1,
        isACK_mask = 1,
        
        egress_spec=184, # ->tofino65x1
        qid=2
)
p4.forward.add_with_set_egr( # local
        dstAddr=0x000000000002,
        protocol = 0,
        protocol_mask = 0xFF,
        isACK = 0,
        isACK_mask = 1,
        
        egress_spec=180, # ->tofino65x1
        qid=1
)

# non-INA
p4.forward.add_with_set_egr( # non-INA
        dstAddr=0x000000000000,
        protocol = 0,
        protocol_mask = 0x0,
        isACK = 0,
        isACK_mask = 0,
        
        egress_spec=180, # ->tofino65x1
        qid=0
)



p4_egress = bfrt.ceina.pipe.Egress


## Left -> Right side
port = 180
queue_high = 2
queue_middle = 1
queue_low = 0

p4_egress.wred.mod( # GLOBAL
    WRED_INDEX=(port << 5) | queue_high,
    WRED_SPEC_MIN_THRESH_CELLS = 3000,
    WRED_SPEC_MAX_THRESH_CELLS = 4000,
    WRED_SPEC_MAX_PROBABILITY = 0.1,
    WRED_SPEC_TIME_CONSTANT_NS = 1000000.0)
print(f"wred port: {port} queue:{queue_high} rule is inserted")

p4_egress.wred.mod( # GLOBAL
    WRED_INDEX=(port << 5) | queue_middle,
    WRED_SPEC_MIN_THRESH_CELLS = 2000,
    WRED_SPEC_MAX_THRESH_CELLS = 3000,
    WRED_SPEC_MAX_PROBABILITY = 0.3,
    WRED_SPEC_TIME_CONSTANT_NS = 1000000.0)
print(f"wred port: {port} queue:{queue_middle} rule is inserted")

p4_egress.wred.mod( # normal
    WRED_INDEX=(port << 5) | queue_low,
    WRED_SPEC_MIN_THRESH_CELLS = 1000,
    WRED_SPEC_MAX_THRESH_CELLS = 2000,
    WRED_SPEC_MAX_PROBABILITY = 0.7,
    WRED_SPEC_TIME_CONSTANT_NS = 1000000.0)
print(f"wred port: {port} queue:{queue_low} rule is inserted")




## Right -> Left side
port_left = 184
queue_high = 2
queue_middle = 1
queue_low = 0

p4_egress.wred.mod( # GLOBAL
    WRED_INDEX=(port_left << 5) | queue_high,
    WRED_SPEC_MIN_THRESH_CELLS = 3000,
    WRED_SPEC_MAX_THRESH_CELLS = 4000,
    WRED_SPEC_MAX_PROBABILITY = 0.1,
    WRED_SPEC_TIME_CONSTANT_NS = 1000000.0)
print(f"wred port: {port_left} queue:{queue_high} rule is inserted")

p4_egress.wred.mod( # GLOBAL
    WRED_INDEX=(port_left << 5) | queue_middle,
    WRED_SPEC_MIN_THRESH_CELLS = 2000,
    WRED_SPEC_MAX_THRESH_CELLS = 3000,
    WRED_SPEC_MAX_PROBABILITY = 0.3,
    WRED_SPEC_TIME_CONSTANT_NS = 1000000.0)
print(f"wred port: {port_left} queue:{queue_middle} rule is inserted")

p4_egress.wred.mod( # normal
    WRED_INDEX=(port << 5) | queue_low,
    WRED_SPEC_MIN_THRESH_CELLS = 1000,
    WRED_SPEC_MAX_THRESH_CELLS = 2000,
    WRED_SPEC_MAX_PROBABILITY = 0.7,
    WRED_SPEC_TIME_CONSTANT_NS = 1000000.0)
print(f"wred port: {port_left} queue:{queue_low} rule is inserted")
