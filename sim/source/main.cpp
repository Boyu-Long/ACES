#ifndef MAIN
#define MAIN

#include<aces_simulator.h>

int main(int argc, char* argv[]){
    if(argc < -1){
        cout<<"error in main, too few args"<<endl;
        return 0;
    }
    simulator s;
    if(argc < 6){
        mtx_file_path = "/home/test_mat";
        mtx_mat_name = "wiki-Vote";
        cfg_spmm_cache_mshr = 16;
        cfg_spmm_cache_mshr_ts = 64;
        cfg_spmm_condense_type = -1;
        s.sim();
    }
    else{
        mtx_file_path = argv[1];
        mtx_mat_name = argv[2];
        cfg_spmm_cache_mshr = atoi(argv[3]);
        cfg_spmm_cache_mshr_ts = atoi(argv[4]);
        cfg_spmm_condense_type = atoi(argv[5]);
        s.sim();
    }
    return 0;
}

#endif