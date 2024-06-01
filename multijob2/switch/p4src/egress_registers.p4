#ifndef _EGRESS_REGISTERS_
#define _EGRESS_REGISTERS_

Register<bit<32>, _>(1) dqueue_alert_threshold;

RegisterAction<bit<32>, bit<32>, bit<32>>(dqueue_alert_threshold) do_comp_qdepth  = {
    void apply(inout bit<32> value, out bit<32> read_value){
        if (eg_intr_md.deq_qdepth >= 1000){
            read_value = (bit<32>)eg_intr_md.deq_qdepth; // eg_md.mdata.qdepth;
        }
    }
};

Register<bit<32>, _>(32) drop_counter;

RegisterAction<bit<32>, bit<32>, bit<32>>(drop_counter) increase_drop_counter  = {
    void apply(inout bit<32> value){
        if (eg_intr_dprs_md.drop_ctl != 0){
            value = value + 1;
            // read_value = value;
        }
    }
};

action action_drop_counter(){
    increase_drop_counter.execute((bit<32>)eg_intr_md.egress_qid);
}

#endif