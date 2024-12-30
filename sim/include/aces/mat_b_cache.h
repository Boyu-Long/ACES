#ifndef MAT_B_CACHE
#define MAT_B_CACHE

#include<base.h>
#include<device_base.h>

// all coor in program is based on the coor in csr format, coor in coo format is pure data
struct b_cache_row{
    int csr_row;
    //must begin with 0 ~ size-1
    int size;
    int b_row_size;
    int next_coo_col;

    b_cache_row(int csr_row = -1, int size = -1, 
              int b_row_size = -1, int next_coo_col = -1){
        this->csr_row = csr_row;
        this->size = size;
        this->b_row_size = b_row_size;
        this->next_coo_col = next_coo_col;
    }
};

enum ps_cache_state{
    add_write,

};

struct ps_cache_row{
    int csr_row;
    vector<coo> elem;

    ps_cache_row(int csr_row = -1){
        this->csr_row = csr_row;
        this->elem = {};
    }
};

struct b_valid_cache_row{
    int read_cnt;
    b_cache_row info;

    b_valid_cache_row(int read_cnt = -1, 
    b_cache_row info = b_cache_row()){
        this->read_cnt = read_cnt;
        this->info = info;
    }
};

struct ps_valid_cache_row{
    int read_cnt;
    ps_cache_row info;
    int rewrite_index;

    ps_valid_cache_row(int read_cnt = -1, 
    ps_cache_row info = ps_cache_row()){
        this->read_cnt = read_cnt;
        this->info = info;
        this->rewrite_index = 0;
    }
};

struct b_invalid_cache_row{
    long reuse_dis;
    long end_mult_index;
    int prefetch_dis;
    b_cache_row info;

    b_invalid_cache_row(long reuse_dis = -1, long prefetch_dis = -1, long end_mult_index = -1, 
                      b_cache_row info = b_cache_row()){
        this->reuse_dis = reuse_dis;
        this->prefetch_dis = prefetch_dis;
        this->info = info;
        this->end_mult_index = end_mult_index;
    }
};

struct ps_invalid_cache_row{
    int reuse_dis;
    int prefetch_dis;
    int remain_size;
    ps_cache_row info;

    ps_invalid_cache_row(int reuse_dis = -1, int prefetch_dis = -1, int remain_size = -1,
                      ps_cache_row info = ps_cache_row()){
        this->reuse_dis = reuse_dis;
        this->prefetch_dis = prefetch_dis;
        this->remain_size = remain_size;
        this->info = info;
    }
};

struct b_cache_cmd{
    cint clock;
    int cn;
    int csr_row, csr_col;

    b_cache_cmd(cint clock = 0, int cn = -1, 
              int csr_row = -1, int csr_col = -1){
        this->clock = clock;
        this->cn = cn;
        this->csr_row = csr_row;
        this->csr_col = csr_col;
    }

    bool operator<(const b_cache_cmd &p) const{
        if(this->clock == p.clock)
            return this->cn < p.cn;
        return this->clock < p.clock;
    }
    bool operator>(const b_cache_cmd &p) const{
        if(this->clock == p.clock)
            return this->cn > p.cn;
        return this->clock > p.clock;
    }
};

struct ps_cache_cmd{
    cint clock;
    int cn;
    int row, col;

    ps_cache_cmd(cint clock = 0, int cn = -1, 
              int row = -1, int col = -1){
        this->clock = clock;
        this->cn = cn;
        this->row = row;
        this->col = col;
    }

    bool operator<(const ps_cache_cmd &p) const{
        if(this->clock == p.clock)
            return this->cn < p.cn;
        return this->clock < p.clock;
    }
    bool operator>(const ps_cache_cmd &p) const{
        if(this->clock == p.clock)
            return this->cn > p.cn;
        return this->clock > p.clock;
    }
};

struct b_mshr_info{
    bool valid;
    int csr_row;
    int begin, end;

    b_mshr_info(bool valid = false, int csr_row = -1, 
                int begin = -1, int end = -1){
        this->valid = valid;
        this->csr_row = csr_row;
        this->begin = begin;
        this->end = end;
    }
};

struct ps_mshr_info{
    bool valid;
    int row;
    int begin, end;

    ps_mshr_info(bool valid = false, int row = -1, 
                 int begin = -1, int end = -1){
        this->valid = valid;
        this->row = row;
        this->begin = begin;
        this->end = end;
    }
};

struct b_cache_access{
    cint clock;
    int csr_row;
    int csr_col;

    b_cache_access(cint clock = 0, int csr_row = -1, int csr_col = -1){
        this->clock = clock;
        this->csr_row = csr_row;
        this->csr_col = csr_col;
    }
};

struct ps_cache_access{
    cint clock;
    coo value;

    ps_cache_access(cint clock = 0, coo value = coo{-1, -1, -1}){
        this->clock = clock;
        this->value = value;
    }
};

struct ps_rewr_index{
    cint clock;
    int stamp;
    int csr_row;

    ps_rewr_index(cint clock = 0, int stamp = 0, int csr_row = -1){
        this->clock = clock;
        this->stamp = stamp;
        this->csr_row = csr_row;
    }

    bool operator<(const ps_rewr_index& p) const{
        // if(this->stamp == p.stamp){
        //     if(this->csr_row == p.csr_row)
        //         return this->clock < p.clock;
        //     return this->csr_row < p.csr_row;
        // }
        // return this->stamp < p.stamp;
        if(this->clock == p.clock){
            if(this->stamp == p.stamp)
                return this->csr_row < p.csr_row;
            return this->stamp < p.stamp;
        }
        return this->clock < p.clock;
    }
};

struct ps_rewr_info{
    queue<coo> elem;
    bool ape_rewr_end;

    ps_rewr_info(){
        this->elem = {};
        this->ape_rewr_end = false;
    }
};

template<typename T>
struct b_cache_great{
    bool operator()(const T &p1, const T &p2){
        return p1 > p2;
    }
};

class mat_b_cache : public device_base{
private:
    //------------------Mult side(read)------------------//
    //output data
    // vector<queue<coo>> out_mult_rd_data;
    // vector<queue<bool>> out_mult_rd_valid;
    // vector<coo> out_mult_rd_data;
    // vector<bool> out_mult_rd_valid;
    vector<bool> out_mult_rd_busy;

    //input data(don't use param "num" in the pkg)
    // vector<queue<mem_pkg>> in_mult_rd_pkg;
    //-----------------Adder side(read)------------------//
    
    // vector<coo> out_add_rd_data;
    // vector<bool> out_add_rd_valid;
    vector<bool> out_add_rd_busy;
    
    //----------------Adder side(write)-----------------//
    //output
    vector<bool> out_add_wr_busy;
    
    //-----------------Memory side(read)-----------------//
    //output data 16 channel for 16 mpes, 16 channel for 16 apes
    // queue<mem_pkg> out_mem_rd_pkg;
    //input data
    // queue<pair<string, coo>> in_mem_rd_data;
    //-----------------Memory side(write)----------------//
    //input
    bool in_mem_wr_busy;
    // //output
    // queue<bool> out_mem_wr_comp;
    // //stamp, csr row
    // queue<pair<int, int>> out_mem_wr_name;
    // queue<coo> out_mem_wr_value;
    //-------------------Inner storage-------------------//
    int cache_size;
    int cache_row_size;
    double b_ps_cache_ratio;
    //--cache data--//
    //<csr row, valid_row>
    int b_valid_cache_size, b_invalid_cache_size;
    map<int, b_valid_cache_row> b_valid_cache;
    vector<b_invalid_cache_row> b_invalid_cache;
    int cmp_mult_index;

    int ps_valid_cache_size, ps_invalid_cache_size, ps_max_size;
    map<int, ps_valid_cache_row> ps_valid_cache;
    vector<ps_invalid_cache_row> ps_invalid_cache;
    map<int, vector<coo>> ps_wr_stagging;
    //if read a 6-num row but cache only has 4 invalid num, then pre_delete = 2
    //valid cache size will stop the read at 4th until cache has a new invalid row
    int ps_cache_pre_delete_num;
    //-------------------Inner param-------------------//
    int b_channel_num, ps_channel_num;
    int mem_channel_num;
    int delay;
    //--process cmd--//
    int cmd_process_per_cycle;
    int read_cmd_max_size;
    vector<queue<b_cache_access>> b_read_output_stagging;
    priority_queue<b_cache_cmd, vector<b_cache_cmd>, b_cache_great<b_cache_cmd>> b_read_cmd_stagging;

    vector<queue<ps_cache_access>> ps_read_output_stagging;
    priority_queue<ps_cache_cmd, vector<ps_cache_cmd>, b_cache_great<ps_cache_cmd>> ps_read_cmd_stagging;

    //--prefetch b--//
    //(16 prefetcher work the same as 16 mpe)
    //there is no relationship between mpe and prefetcher
    vector<bool> b_fetcher_working;
    vector<pair<int, int>> b_acc_seq;
    int b_max_prefetch_row;
    int b_max_mult_index, b_max_prefetch_index;
    vector<int> b_mult_index;
    vector<int> b_prefetch_index;
    //csr col
    vector<int> b_read_row_index;

    //--prefetch ps--//
    //(1 prefetcher only and prefetch 16 elem per row per cycle)
    //one row can only have one ape and mpe
    //there is no relationship between ape and prefetcher
    //ps acc can not determin the size
    vector<int> ps_prefetch_seq;
    bool ps_fetcher_working;
    map<int, int> ps_row_size;
    vector<int> ps_acc_seq;
    int ps_max_prefetch_row;
    int ps_max_add_index;
    vector<int> ps_add_index;
    //int ps_prefetch_index;
    //csr col
    int ps_read_row_index;

    //--rewrite--//
    //rewrite is bigger than max size but ape is still reading
    //when reading is end, erase row
    int rewrite_max_num;
    set<int> rd_occupy_stagging_row;
    //all the ape that has a result which size is bigger than max ps size
    map<int, ps_rewr_index> rewrite_ape_stagging;
    //stamp and csr row to ensure uniqueness
    map<ps_rewr_index, ps_rewr_info> rewrite_stagging;
    set<int> waiting_last_rewrite;
    
    //mem side load monitoring
        //the max req num is mem-channel-num in every cycle
    int mem_rd_req_cnt, mem_wr_req_cnt;

        //with MSHR
    int mshr_ts, mshr_ls_ts;
    //<csr row, mshr index>
    //a row may has multiple index
    map<int, vector<int>> b_mshr_map;
    vector<b_mshr_info> b_mshr_table;
    //vector for mshr table, map for <csr col, vector<pe_cn>>
    vector<map<int, vector<int>>> b_mshr_ls_table;
    
    //<csr row, mshr index>
    //a row may has multiple index
    map<int, vector<int>> ps_mshr_map;
    vector<ps_mshr_info> ps_mshr_table;
    //vector for mshr table, map for <csr col, vector<pe_cn>>
    vector<map<int, vector<int>>> ps_mshr_ls_table;
    
    int ls_table_count;
    
    // bool reverse_dis_cmp_b(const b_invalid_cache_row &row1, const b_invalid_cache_row &row2);

    void use_b_invalid_cache(int &row_size, int &valid_size);
    void use_ps_invalid_cache(int &row_size, int &valid_size);

    void get_new_b_row_place(cint clock, int prefetch_index, int csr_row, int &row_size, int find_index);
    void get_new_ps_row_place(cint clock, int csr_row, int &row_size, int find_index);

    void read_next_b_row(cint clock, int prefetch_index);
    void read_next_ps_row(cint clock);

    void delete_b_cache_row(int row_index, long end_mult_index, int now_mult_index);
    void delete_ps_cache_row(int row_index);

    bool b_read_process(cint clock);
    bool ps_read_process(cint clock);

    void update_b_mshr(cint clock, int csr_row, int csr_col);
    void update_ps_mshr(cint clock, int csr_row, int csr_col);

    cint b_latest_avalivle_time(cint clock, int cn);
    cint ps_latest_avalivle_time(cint clock, int cn);

    void clock_apply_mpe_preprocess(cint clock);
    void clock_apply_ape_preprocess(cint clock);

    void clock_apply_mem_process(cint clock);

    void update_ps_cache_row_info(cint clock, ps_valid_cache_row &ps_row);
    void remove_exceeding_limit_row(cint clock, int csr_row);
    void ape_write_back_in_cache(cint clock, int pe_index, int csr_row, coo write_back_value, bool wr_comp);

public:
    int ps_prefetch_index;
    //-------------------Performance-------------------//
    //cache hit
    unsigned long long b_ch, ps_ch;
    //cache miss and not in MSHR
    unsigned long long b_fcm, ps_fcm;
    //cache miss and in MSHR
    unsigned long long b_nfcm, ps_nfcm;
    //mem_pkg num of b and ps
    unsigned long long b_mem_rd_traffic,    ps_mem_rd_traffic;
    unsigned long long                      ps_mem_wr_traffic;
    unsigned long long b_cache_stall,       ps_cache_stall;

    //output data 16 channel for 16 mpes, 16 channel for 16 apes
    queue<mem_pkg> *out_mem_rd_pkg;
    // queue<mem_pkg> *out_mem_rd_pkg_b;
    // queue<mem_pkg> *out_mem_rd_pkg_ps;

    queue<pair<string, coo>> *in_mem_rd_data;

    //output data
    vector<queue<coo>*> out_mult_rd_data;
    vector<queue<bool>*> out_mult_rd_valid;

    //input data(don't use param "num" in the pkg)
    vector<queue<mem_pkg>*> in_mult_rd_pkg;

    //input
    vector<queue<bool>*> in_add_wr_valid;
    vector<queue<bool>*> in_add_wr_comp;
    vector<queue<coo>*> in_add_wr_value;

    //input data(don't use param "num" in the pkg)
    vector<queue<mem_pkg>*> in_add_rd_pkg;

    //output data
    vector<queue<coo>*> out_add_rd_data;
    vector<queue<bool>*> out_add_rd_valid;


    //output
    queue<bool> *out_mem_wr_comp;
    //stamp, csr row
    queue<pair<int, int>> *out_mem_wr_name;
    queue<coo> *out_mem_wr_value;

    mat_b_cache(int cn, int delay, int cap, int c_row_s, int max_pre_row, int cmd_per_c, int mem_cn, int mshr_ts, int mshr_ls_ts, string name);
    ~mat_b_cache();

    void begin_task(cint clock);
    void reset_task();

    void set_ps_max_size(int size){this->ps_max_size = size;};
    void set_b_ps_cache_ratio(double r){this->b_ps_cache_ratio = r;};

    void add_b_acc_seq(vector<pair<int, int>> &add_seq);
    void add_ps_acc_seq(vector<pair<int, int>> &add_seq);
    int add_ps_prefetch_seq(int add_csr_row);
    void set_mult_task_end(int mpe_index, int next_mult_index);
    void set_add_task_end(int ape_index, int next_add_index);

    void set_mult_input_pkg(cint clock, int cn, queue<mem_pkg> &pkg);
    void get_mult_output(cint clock, int cn, bool& rd_busy);

    void set_add_rd_input(cint clock, int cn, queue<mem_pkg> &pkg);
    void get_add_rd_output(cint clock, int cn, bool& rd_busy);

    //void set_add_wr_input(cint clock, int cn, bool valid, bool comp, coo value);
    void set_add_wr_input(cint clock, int cn, queue<bool> &valid, queue<bool> &comp, queue<coo> &value);
    void get_add_wr_output(cint clock, int cn, bool& wr_busy);

    void set_mem_rd_input(cint clock, queue<pair<string, coo>> &data);
    void get_mem_rd_output(cint clock, queue<mem_pkg> &rd_pkg);

    void set_mem_wr_input(cint clock, bool wr_busy);
    void get_mem_wr_output(cint clock, queue<bool> &comp, queue<pair<int, int>> &name, queue<coo> &value);

    bool clock_apply(cint clock);
    void clock_update(cint clock);
    
    void print_device_status(ofstream &fout);
};

// read: "A, (0,1)" means: read (0,3), the column is in csr format (one at a time)
// name: A
//      |-----|
// (0,0)|(0,3)|(0,4)(0,5)
// (1,0)(1,2)(1,3)(1,5)(1,7)
// (2,1)(2,3)(2,4)

// read from input port -> read_cmd_stagging -> process

mat_b_cache::mat_b_cache(
int cn          =   cfg_spmm_cache_cn, 
int delay       =   cfg_spmm_cache_d, 
int cap         =   cfg_spmm_cache_cap, 
int c_row_s     =   cfg_spmm_spmm_cache_rs, 
int max_pre_row =   cfg_spmm_cache_pre_row, 
int cmd_per_c   =   cfg_spmm_cache_cmd_per_clock, 
int mem_cn      =   cfg_spmm_mem_cn, 
int mshr_ts     =   cfg_spmm_cache_mshr, 
int mshr_ls_ts  =   cfg_spmm_cache_mshr_ts, 
string name     =   "b_cache"
) : device_base(name){
    //-------------------PE side(read)-------------------//
    //output data
    this->out_mult_rd_data.resize(cn);
    this->out_mult_rd_valid.resize(cn);
    this->out_mult_rd_busy.resize(cn);
    // for(int i=0;i<cn;i++){
    //     clear(this->out_mult_rd_valid[i]);
    //     clear(this->out_mult_rd_data[i]);
    // }
    // fill(this->out_mult_rd_valid.begin(), this->out_mult_rd_valid.end(), false);
    fill(this->out_mult_rd_busy.begin(), this->out_mult_rd_busy.end(), true);

    //input data(don't use param "num" in the pkg)
    this->in_mult_rd_pkg.resize(cn);
    // for(int i=0;i<cn;i++)
    //     clear(this->in_mult_rd_pkg[i]);
    //-----------------Adder side(read)------------------//
    //output data
    this->out_add_rd_data.resize(cn);
    this->out_add_rd_valid.resize(cn);
    this->out_add_rd_busy.resize(cn);
    // for(int i=0;i<cn;i++){
    //     clear(this->out_add_rd_data[i]);
    //     clear(this->out_add_rd_valid[i]);
    // }
    // fill(this->out_add_rd_valid.begin(), this->out_add_rd_valid.end(), false);
    fill(this->out_add_rd_busy.begin(), this->out_add_rd_busy.end(), true);


    //input data(don't use param "num" in the pkg)
    this->in_add_rd_pkg.resize(cn);
    // for(int i=0;i<cn;i++)
    //     clear(this->in_add_rd_pkg[i]);
    //----------------Adder side(write)-----------------//
    //input
    this->in_add_wr_valid.resize(cn);
    this->in_add_wr_comp.resize(cn);
    this->in_add_wr_value.resize(cn);
    // for(int i=0;i<cn;i++){
    //     clear(this->in_add_wr_valid[i]);
    //     clear(this->in_add_wr_comp[i]);
    //     clear(this->in_add_wr_value[i]);
    // }
    // fill(this->in_add_wr_valid.begin(), this->in_add_wr_valid.end(), false);
    // fill(this->in_add_wr_comp.begin(), this->in_add_wr_comp.end(), false);

    //output
    this->out_add_wr_busy.resize(cn);
    fill(this->out_add_wr_busy.begin(), this->out_add_wr_busy.end(), false);
    //-----------------Memory side(read)-----------------//
    //output data 16 channel for 16 mpes, 16 channel for 16 apes
    // clear(this->out_mem_rd_pkg);
    //input data
    // clear(this->in_mem_rd_data);
    //-----------------Memory side(write)----------------//
    //input
    this->in_mem_wr_busy = true;
    
    //output
    // clear(this->out_mem_wr_comp);
    // clear(this->out_mem_wr_name);
    // clear(this->out_mem_wr_value);
    //-------------------Inner storage-------------------//
    this->cache_row_size = c_row_s;
    this->cache_size = cap;
    //b cache data
    this->b_valid_cache_size = 0;
    this->b_invalid_cache_size = cap/2;
    this->b_valid_cache.clear();
    this->b_invalid_cache.clear();
    this->b_invalid_cache.push_back(b_invalid_cache_row{__INT_MAX__, 0, -1, b_cache_row{-1, this->b_invalid_cache_size, -1, -1}});
    //ps cache data
    this->ps_max_size = 0;
    this->ps_valid_cache_size = 0;
    this->ps_invalid_cache_size = this->cache_size-this->b_invalid_cache_size;
    this->ps_valid_cache.clear();
    this->ps_invalid_cache.push_back(ps_invalid_cache_row{__INT_MAX__, 0, this->ps_invalid_cache_size, ps_cache_row{-1}});
    this->ps_wr_stagging.clear();
    this->ps_cache_pre_delete_num = 0;
    //-------------------Inner param-------------------//
    this->mem_channel_num = mem_cn;
    this->delay = delay;
    //this->cmd_process_per_cycle = cmd_per_c;
    this->cmd_process_per_cycle = __INT_MAX__;
    this->read_cmd_max_size = cn*4;

    //--process b cmd--//
    this->b_channel_num = cn;
    this->b_read_output_stagging.resize(this->b_channel_num);
    for(int i=0;i<this->b_channel_num;i++)
        clear(this->b_read_output_stagging[i]);
    while(!this->b_read_cmd_stagging.empty())
        this->b_read_cmd_stagging.pop();
    
    //--process ps cmd--//
    this->ps_channel_num = cn;
    this->ps_read_output_stagging.resize(this->ps_channel_num);
    for(int i=0;i<this->ps_channel_num;i++)
        clear(this->ps_read_output_stagging[i]);
    while(!this->ps_read_cmd_stagging.empty())
        this->ps_read_cmd_stagging.pop();

    //--prefetch b--//
    this->b_fetcher_working.resize(this->b_channel_num);
    this->b_max_prefetch_row = max_pre_row;
    this->b_acc_seq.clear();
    this->b_max_mult_index = 0;
    this->b_mult_index.resize(this->b_channel_num);
    this->b_prefetch_index.resize(this->b_channel_num);
    this->b_read_row_index.resize(this->b_channel_num);
    fill(this->b_fetcher_working.begin(), this->b_fetcher_working.end(), false);
    fill(this->b_mult_index.begin(), this->b_mult_index.end(), -1);
    fill(this->b_read_row_index.begin(), this->b_read_row_index.end(), -1);
    fill(this->b_prefetch_index.begin(), this->b_prefetch_index.end(), -1);
    this->b_max_prefetch_index = -1;

    //--prefetch ps--//
    this->ps_row_size.clear();
    this->ps_prefetch_seq.clear();
    this->ps_fetcher_working = false;
    this->ps_max_prefetch_row = max_pre_row;
    this->ps_acc_seq.clear();
    this->ps_max_add_index = -1;
    this->ps_add_index.resize(this->ps_channel_num);
    this->ps_prefetch_index = -1;
    this->ps_read_row_index = 0;
    fill(this->ps_add_index.begin(), this->ps_add_index.end(), -1);

    //--rewrite--//
    this->rewrite_max_num = __INT_MAX__;
    this->rd_occupy_stagging_row.clear();
    this->rewrite_ape_stagging.clear();
    this->rewrite_stagging.clear();
    this->waiting_last_rewrite.clear();

    //mem side load monitoring
        //the max req num is mem-channel-num in every cycle
    this->mem_rd_req_cnt = 0;
    this->mem_wr_req_cnt = 0;

    //--MSHR--//
    this->mshr_ts = mshr_ts;
    this->mshr_ls_ts = mshr_ls_ts;
    this->ls_table_count = 0;
    //--MSHR b--//
    this->b_mshr_table.resize(mshr_ts);
    this->b_mshr_ls_table.resize(mshr_ts);
    fill(this->b_mshr_table.begin(), this->b_mshr_table.end(), b_mshr_info{false, -1, -1, -1});
    for(int i=0;i<mshr_ts;i++)
        this->b_mshr_ls_table[i].clear();
    //--MSHR ps--//
    this->ps_mshr_table.resize(mshr_ts);
    this->ps_mshr_ls_table.resize(mshr_ts);
    fill(this->ps_mshr_table.begin(), this->ps_mshr_table.end(), ps_mshr_info{false, -1, -1, -1});
    for(int i=0;i<mshr_ts;i++)
        this->ps_mshr_ls_table[i].clear();
    
        //performance
    //cache hit | first cache miss | non-first cache miss
    this->b_ch = 0;this->b_fcm = 0;this->b_nfcm = 0;
    this->ps_ch = 0;this->ps_fcm = 0;this->ps_nfcm = 0;
    this->b_mem_rd_traffic = 0;this->ps_mem_rd_traffic = 0;
    this->ps_mem_wr_traffic = 0;
    this->b_cache_stall = 0;
    this->ps_cache_stall = 0;
}

mat_b_cache::~mat_b_cache(){
}

void mat_b_cache::begin_task(cint clock){
    for(int i=0;i<this->b_channel_num;i++)
        this->read_next_b_row(clock, i);
    this->read_next_ps_row(clock);
}

void mat_b_cache::reset_task(){
    //-------------------PE side(read)-------------------//
    //output data
    // for(int i=0;i<this->b_channel_num;i++){
    //     clear(this->out_mult_rd_valid[i]);
    //     clear(this->out_mult_rd_data[i]);
    // }
    // fill(this->out_mult_rd_valid.begin(), this->out_mult_rd_valid.end(), false);
    fill(this->out_mult_rd_busy.begin(), this->out_mult_rd_busy.end(), true);

    //input data(don't use param "num" in the pkg)
    // for(int i=0;i<this->b_channel_num;i++)
    //     clear(this->in_mult_rd_pkg[i]);
    // fill(this->in_mult_rd_pkg.begin(), this->in_mult_rd_pkg.end(), emp_pkg);
    //-----------------Adder side(read)------------------//
    //output data
    // for(int i=0;i<this->ps_channel_num;i++){
    //     clear(this->out_add_rd_data[i]);
    //     clear(this->out_add_rd_valid[i]);
    // }
    // fill(this->out_add_rd_valid.begin(), this->out_add_rd_valid.end(), false);
    fill(this->out_add_rd_busy.begin(), this->out_add_rd_busy.end(), true);


    //input data(don't use param "num" in the pkg)
    // for(int i=0;i<this->ps_channel_num;i++)
    //     clear(this->in_add_rd_pkg[i]);
    //----------------Adder side(write)-----------------//
    //input
    // for(int i=0;i<this->ps_channel_num;i++){
    //     clear(this->in_add_wr_valid[i]);
    //     clear(this->in_add_wr_comp[i]);
    //     clear(this->in_add_wr_value[i]);
    // }
    // fill(this->in_add_wr_valid.begin(), this->in_add_wr_valid.end(), false);
    // fill(this->in_add_wr_comp.begin(), this->in_add_wr_comp.end(), false);

    //output
    fill(this->out_add_wr_busy.begin(), this->out_add_wr_busy.end(), false);
    //-----------------Memory side(read)-----------------//
    //output data 16 channel for 16 mpes, 16 channel for 16 apes
    // clear(this->out_mem_rd_pkg);
    //input data
    // clear(this->in_mem_rd_data);
    //-----------------Memory side(write)----------------//
    //input
    this->in_mem_wr_busy = true;
    
    //output
    // clear(this->out_mem_wr_comp);
    // clear(this->out_mem_wr_name);
    // clear(this->out_mem_wr_value);
    //-------------------Inner storage-------------------//
    //b cache data
    assert(this->b_valid_cache_size == 0);
    this->b_valid_cache.clear();
    for(auto &row : this->b_invalid_cache)
        row.reuse_dis = max_reuse_dis_b;
    //ps cache data
    this->ps_max_size = 0;
    assert(this->b_valid_cache_size == 0);
    this->ps_valid_cache.clear();
    this->ps_invalid_cache.clear();
    this->ps_invalid_cache.push_back(ps_invalid_cache_row{__INT_MAX__, 0, this->ps_invalid_cache_size, ps_cache_row{-1}});
    this->ps_wr_stagging.clear();
    this->ps_cache_pre_delete_num = 0;
    //-------------------Inner param-------------------//
    //--process b cmd--//
    for(int i=0;i<this->b_channel_num;i++)
        clear(this->b_read_output_stagging[i]);
    while(!this->b_read_cmd_stagging.empty())
        this->b_read_cmd_stagging.pop();
    
    //--process ps cmd--//
    for(int i=0;i<this->ps_channel_num;i++)
        clear(this->ps_read_output_stagging[i]);
    while(!this->ps_read_cmd_stagging.empty())
        this->ps_read_cmd_stagging.pop();

    //--prefetch b--//
    this->b_acc_seq.clear();
    this->b_max_mult_index = 0;
    fill(this->b_fetcher_working.begin(), this->b_fetcher_working.end(), false);
    fill(this->b_mult_index.begin(), this->b_mult_index.end(), -1);
    fill(this->b_read_row_index.begin(), this->b_read_row_index.end(), -1);
    fill(this->b_prefetch_index.begin(), this->b_prefetch_index.end(), -1);
    this->b_max_prefetch_index = -1;

    //--prefetch ps--//
    this->ps_row_size.clear();
    this->ps_prefetch_seq.clear();
    this->ps_fetcher_working = false;
    this->ps_acc_seq.clear();
    this->ps_max_add_index = -1;
    this->ps_prefetch_index = -1;
    this->ps_read_row_index = 0;
    fill(this->ps_add_index.begin(), this->ps_add_index.end(), -1);

    //--rewrite--//
    this->rewrite_max_num = __INT_MAX__;
    this->rd_occupy_stagging_row.clear();
    this->rewrite_ape_stagging.clear();
    this->rewrite_stagging.clear();
    this->waiting_last_rewrite.clear();

    //mem side load monitoring
        //the max req num is mem-channel-num in every cycle
    this->mem_rd_req_cnt = 0;
    this->mem_wr_req_cnt = 0;

    //--MSHR--//
    this->ls_table_count = 0;
    //--MSHR b--//
    fill(this->b_mshr_table.begin(), this->b_mshr_table.end(), b_mshr_info{false, -1, -1, -1});
    for(int i=0;i<mshr_ts;i++)
        this->b_mshr_ls_table[i].clear();
    //--MSHR ps--//
    fill(this->ps_mshr_table.begin(), this->ps_mshr_table.end(), ps_mshr_info{false, -1, -1, -1});
    for(int i=0;i<mshr_ts;i++)
        this->ps_mshr_ls_table[i].clear();
    
        //performance
    //cache hit | first cache miss | non-first cache miss
    this->b_ch = 0;this->b_fcm = 0;this->b_nfcm = 0;
    this->ps_ch = 0;this->ps_fcm = 0;this->ps_nfcm = 0;
    this->b_mem_rd_traffic = 0;this->ps_mem_rd_traffic = 0;
    this->ps_mem_wr_traffic = 0;
    this->b_cache_stall = 0;
    this->ps_cache_stall = 0;
}

bool reverse_dis_cmp_b(const b_invalid_cache_row &row1, const b_invalid_cache_row &row2){
    long pd1 = row1.reuse_dis + row1.prefetch_dis - (cmp_mult_index - row1.end_mult_index);
    long pd2 = row2.reuse_dis + row2.prefetch_dis - (cmp_mult_index - row2.end_mult_index);
    if(pd1 < pd2)
       return true;
    if(pd1 == pd2)
        return row1.reuse_dis < row2.reuse_dis;
    return false;
}

bool reverse_dis_cmp_ps(const ps_invalid_cache_row &row1, const ps_invalid_cache_row &row2){
    if(row1.reuse_dis + row1.prefetch_dis == row2.reuse_dis + row2.prefetch_dis)
        return row1.reuse_dis < row2.reuse_dis;
    return row1.reuse_dis + row1.prefetch_dis < row2.reuse_dis + row2.prefetch_dis;
}

void mat_b_cache::add_b_acc_seq(vector<pair<int, int>> &add_seq){
    //update reuse distance
    for(b_invalid_cache_row &row : this->b_invalid_cache){
        if(row.reuse_dis == max_reuse_dis_b){
            for(int i=0;i<add_seq.size();i++){
                if(row.info.csr_row == add_seq[i].first){
                    row.reuse_dis = i+this->b_acc_seq.size()-this->b_max_mult_index;
                    continue;
                }
            }
        }
    }
    cmp_mult_index = -1;
    sort(this->b_invalid_cache.begin(), this->b_invalid_cache.end(), reverse_dis_cmp_b);
    //get new acc seq
    this->b_acc_seq.insert(b_acc_seq.begin(), add_seq.begin(), add_seq.end());
}

void mat_b_cache::add_ps_acc_seq(vector<pair<int, int>> &add_seq){
    //update reuse distance
    for(ps_invalid_cache_row &row : this->ps_invalid_cache){
        if(row.reuse_dis + row.prefetch_dis == __INT_MAX__){
            for(int i=0;i<add_seq.size();i++){
                if(row.info.csr_row == add_seq[i].first){
                    row.reuse_dis = i+this->ps_acc_seq.size()-this->ps_max_add_index;
                    continue;
                }
            }
        }
    }
    sort(this->ps_invalid_cache.begin(), this->ps_invalid_cache.end(), reverse_dis_cmp_ps);
    //get new acc seq
    for(pair<int, int> info : add_seq)
        this->ps_acc_seq.push_back(info.first);
    //this->ps_acc_seq.insert(ps_acc_seq.begin(), add_seq.begin(), add_seq.end());
}

int mat_b_cache::add_ps_prefetch_seq(int add_csr_row){
    this->ps_prefetch_seq.push_back(add_csr_row);
    return this->ps_prefetch_seq.size()-1;
}

void mat_b_cache::set_mult_task_end(int mpe_index, int next_mult_index){
    int mpe_mult_index = this->b_mult_index[mpe_index];
    this->b_mult_index[mpe_index] = next_mult_index;
    this->b_max_mult_index = *max_element(this->b_mult_index.begin(), this->b_mult_index.end());
    if(mpe_mult_index < 0)
        return;
    pair<int, int> mpe_acc_p = this->b_acc_seq[mpe_mult_index];
    this->b_valid_cache[mpe_acc_p.first].read_cnt -= 1;
    if(this->b_valid_cache[mpe_acc_p.first].read_cnt == 0){
        this->delete_b_cache_row(mpe_acc_p.first, mpe_mult_index, next_mult_index);
    }
}

void mat_b_cache::set_add_task_end(int ape_index, int next_add_index){
    this->ps_add_index[ape_index] = next_add_index;
    this->ps_max_add_index = *max_element(this->ps_add_index.begin(), this->ps_add_index.end());
}

void mat_b_cache::use_b_invalid_cache(int &row_size, int &valid_size){
    while(row_size > 0 && !this->b_invalid_cache.empty()){
        if(this->b_invalid_cache.back().info.size <= row_size){
            this->b_invalid_cache_size -= this->b_invalid_cache.back().info.size;
            valid_size += this->b_invalid_cache.back().info.size;
            row_size -= this->b_invalid_cache.back().info.size;
            this->b_invalid_cache.pop_back();
        }
        else{
            this->b_invalid_cache_size -= row_size;
            valid_size += row_size;
            this->b_invalid_cache.back().info.size -= row_size;
            row_size = 0;
        }
    }
}

void mat_b_cache::use_ps_invalid_cache(int &row_size, int &valid_size){
    //invalid update
    while(row_size > 0 && !this->ps_invalid_cache.empty()){
        if(this->ps_invalid_cache.back().remain_size <= row_size){
            this->ps_invalid_cache_size -= this->ps_invalid_cache.back().remain_size;
            valid_size += this->ps_invalid_cache.back().remain_size;
            row_size -= this->ps_invalid_cache.back().remain_size;
            this->ps_invalid_cache.pop_back();
        }
        else{
            this->ps_invalid_cache_size -= row_size;
            valid_size += row_size;
            this->ps_invalid_cache.back().remain_size -= row_size;
            row_size = 0;
        }
    }
}

void mat_b_cache::get_new_b_row_place(cint clock, int prefetch_index, int csr_row, int &row_size, int find_index){
    //get new row info
    this->b_prefetch_index[prefetch_index] = this->b_max_prefetch_index + 1;
    this->b_max_prefetch_index += 1;
    b_cache_row new_row = b_cache_row{csr_row, 0, row_size, mat_b_read[csr_row].front().c};
    this->b_read_row_index[prefetch_index] = 0;

    //in invalid
    if(find_index != -1){
        b_cache_row &replace_row = this->b_invalid_cache[find_index].info;
        this->b_invalid_cache_size -= replace_row.size;
        this->b_valid_cache_size += replace_row.size;
        row_size -= replace_row.size;
        new_row.size = replace_row.size;
        new_row.next_coo_col = (row_size > 0) ? mat_b_read[new_row.csr_row][new_row.size].c : -1;
        this->b_read_row_index[prefetch_index] = new_row.size;
        this->b_invalid_cache.erase(this->b_invalid_cache.begin()+find_index);
    }

    //get row
    this->b_valid_cache[csr_row] = b_valid_cache_row{1, new_row};

    //update mshr
    if(find_index != -1){
        for(int i=0;i<this->b_valid_cache[csr_row].info.size;i++)
            this->update_b_mshr(clock, csr_row, i);
    }
}

void mat_b_cache::get_new_ps_row_place(cint clock, int csr_row, int &row_size, int find_index){
    //get new row info
    this->ps_prefetch_index += 1;
    ps_cache_row new_row = ps_cache_row{csr_row};
    this->ps_read_row_index = 0;
    
    //in invalid
    if(find_index != -1){
        ps_invalid_cache_row &replace_row = this->ps_invalid_cache[find_index];
        this->ps_invalid_cache_size -= replace_row.remain_size;
        this->ps_valid_cache_size += replace_row.remain_size;
        row_size -= replace_row.remain_size;
        replace_row.info.elem.resize(replace_row.remain_size);
        new_row.elem.assign(replace_row.info.elem.begin(), replace_row.info.elem.end());
        this->ps_read_row_index = replace_row.remain_size;
        this->ps_invalid_cache.erase(this->ps_invalid_cache.begin()+find_index);
    }

    //get row
    this->ps_valid_cache[csr_row] = ps_valid_cache_row{1, new_row};

    //update mshr
    if(find_index != -1){
        for(int i=0;i<this->ps_valid_cache[csr_row].info.elem.size();i++)
            this->update_ps_mshr(clock, csr_row, i);
    }
}

void mat_b_cache::read_next_b_row(cint clock, int prefetch_index){
    int row_size = 0, csr_row = 0;
    while(true){
        if(this->b_max_prefetch_index - this->b_max_mult_index >= this->b_max_prefetch_row){
            this->b_fetcher_working[prefetch_index] = false;
            return;
        }
        if(this->b_max_prefetch_index >= (int)this->b_acc_seq.size() - 1){
            this->b_fetcher_working[prefetch_index] = false;
            return;
        }
        
        //in valid
        csr_row = this->b_acc_seq[this->b_max_prefetch_index+1].first;
        if(this->b_valid_cache.find(csr_row) != this->b_valid_cache.end()){
            this->b_prefetch_index[prefetch_index] = this->b_max_prefetch_index + 1;
            this->b_max_prefetch_index += 1;
            this->b_valid_cache[csr_row].read_cnt += 1;
            continue;
        }

        //do not have enough space
        row_size = this->b_acc_seq[this->b_max_prefetch_index+1].second;
        if(this->b_invalid_cache_size + this->ps_invalid_cache_size < row_size){
            this->b_fetcher_working[prefetch_index] = false;
            return;
        }
        
        //find in invalid
        int find_index = -1;
        for(int i=0;i<this->b_invalid_cache.size();i++){
            if(this->b_invalid_cache[i].info.csr_row == csr_row){
                find_index = i;
                break;
            }
        }
        
        //b is out of bound
        if(this->b_invalid_cache_size+this->b_valid_cache_size > (int)((double)this->cache_size*this->b_ps_cache_ratio)){
            //continue to use ps invalid row
            if(row_size > this->b_invalid_cache_size){
                this->b_fetcher_working[prefetch_index] = false;
                return;
            }
            //only use b invalid row
            else{
                this->get_new_b_row_place(clock, prefetch_index, csr_row, row_size, find_index);
                //invalid dose not have a complete row
                if(row_size > 0){
                    this->b_fetcher_working[prefetch_index] = true;
                    //invalid update
                    use_b_invalid_cache(row_size, this->b_valid_cache_size);
                    assert(row_size == 0);
                    break;
                }
            }
        }
        //use b invalid row first and use ps invalid row later
        else{
            this->get_new_b_row_place(clock, prefetch_index, csr_row, row_size, find_index);
            //invalid dose not have a complete row
            if(row_size > 0){
                this->b_fetcher_working[prefetch_index] = true;
                //invalid update
                use_b_invalid_cache(row_size, this->b_valid_cache_size);
                use_ps_invalid_cache(row_size, this->b_valid_cache_size);
                assert(row_size == 0);
                break;
            }
        }
    }
}

void mat_b_cache::read_next_ps_row(cint clock){
    int row_size = 0, csr_row = 0;
    while(true){
        // if(this->ps_prefetch_index - this->ps_max_add_index >= this->ps_max_prefetch_row){
        //     this->ps_fetcher_working = false;
        //     return;
        // }
        if(this->ps_prefetch_index >= (int)this->ps_prefetch_seq.size() - 1){
            this->ps_fetcher_working = false;
            return;
        }
        
        //in valid
        csr_row = this->ps_prefetch_seq[this->ps_prefetch_index+1];
        if(this->ps_valid_cache.find(csr_row) != this->ps_valid_cache.end()){
            this->ps_prefetch_index += 1;
            this->ps_valid_cache[csr_row].read_cnt += 1;
            assert(false);
            continue;
        }

        //do not in valid and has space
        row_size = mat_ps_read_size[csr_row];
        if(this->b_invalid_cache_size + this->ps_invalid_cache_size < row_size){
            this->ps_fetcher_working = false;
            return;
        }

        //find in invalid
        int find_index = -1;
        for(int i=0;i<this->ps_invalid_cache.size();i++){
            if(this->ps_invalid_cache[i].info.csr_row == csr_row){
                find_index = i;
                break;
            }
        }

        //ps is out of bound
        if(this->ps_invalid_cache_size+this->ps_valid_cache_size > (int)((double)this->cache_size*(1-this->b_ps_cache_ratio))){
            //continue to use b invalid row
            if(row_size > this->ps_invalid_cache_size){
                this->ps_fetcher_working = false;
                return;
            }
            //only use ps invalid row
            else{
                this->get_new_ps_row_place(clock, csr_row, row_size, find_index);
                //invalid dose not have a complete row
                if(row_size > 0){
                    //record row size
                    assert(this->ps_row_size.find(csr_row) == this->ps_row_size.end());
                    this->ps_row_size.emplace(make_pair(csr_row, mat_ps_read_size[csr_row]));
                    this->ps_fetcher_working = true;
                    //invalid update
                    use_ps_invalid_cache(row_size, this->ps_valid_cache_size);
                    assert(row_size == 0);
                    break;
                }
            }
        }
        //use ps invalid row first and use b invalid row later
        else{
            this->get_new_ps_row_place(clock, csr_row, row_size, find_index);
            //invalid dose not have a complete row
            if(row_size > 0){
                //record row size
                assert(this->ps_row_size.find(csr_row) == this->ps_row_size.end());
                this->ps_row_size.emplace(make_pair(csr_row, mat_ps_read_size[csr_row]));
                
                this->ps_fetcher_working = true;
                //invalid update
                use_ps_invalid_cache(row_size, this->ps_valid_cache_size);
                use_b_invalid_cache(row_size, this->ps_valid_cache_size);
                assert(row_size == 0);
                break;
            }
        }
    }
}

void mat_b_cache::clock_update(cint clock){
    //input update
    // fill(this->in_mult_rd_pkg.begin(), this->in_mult_rd_pkg.end(), mem_pkg{"", 0, 0, 0, false, 0});
    
    //mpe-side output update
    for(int cn = 0;cn<this->b_channel_num;cn++){
        while(!this->b_read_output_stagging[cn].empty() && this->b_read_output_stagging[cn].front().clock == clock){
            b_cache_access access = this->b_read_output_stagging[cn].front();
            this->out_mult_rd_data[cn]->push(mat_b_read[access.csr_row][access.csr_col]);
            this->out_mult_rd_valid[cn]->push(true);
            this->b_read_output_stagging[cn].pop();
        }
        // if(!this->b_read_output_stagging[cn].empty() && this->b_read_output_stagging[cn].front().clock == clock){
        //     b_cache_access access = this->b_read_output_stagging[cn].front();
        //     this->out_mult_rd_data[cn] = mat_b_read[access.csr_row][access.csr_col];
        //     this->out_mult_rd_valid[cn] = true;
        //     this->b_read_output_stagging[cn].pop();
        // }
        // else{
        //     this->out_mult_rd_valid[cn] = false;
        // }
    }

    //ape-side output update
    for(int cn = 0;cn<this->ps_channel_num;cn++){
        while(!this->ps_read_output_stagging[cn].empty() && this->ps_read_output_stagging[cn].front().clock == clock){
            ps_cache_access access = this->ps_read_output_stagging[cn].front();
            this->out_add_rd_data[cn]->push(access.value);
            this->out_add_rd_valid[cn]->push(true);
            this->ps_read_output_stagging[cn].pop();
        }
        // if(!this->ps_read_output_stagging[cn].empty() && this->ps_read_output_stagging[cn].front().clock == clock){
        //     ps_cache_access access = this->ps_read_output_stagging[cn].front();
        //     this->out_add_rd_data[cn] = access.value;
        //     this->out_add_rd_valid[cn] = true;
        //     this->ps_read_output_stagging[cn].pop();
        // }
        // else{
        //     this->out_add_rd_valid[cn] = false;
        // }
    }

    //read cmd stagging has limited size (each channel has 2 for rd and 2 for wr in average)
    if(this->b_read_cmd_stagging.size() >= this->b_channel_num*4){
        fill(this->out_mult_rd_busy.begin(), this->out_mult_rd_busy.end(), true);
    }
    else{
        if(this->out_mult_rd_busy.front())
            fill(this->out_mult_rd_busy.begin(), this->out_mult_rd_busy.end(), false);
    }
    if(this->ps_read_cmd_stagging.size() >= this->ps_channel_num*4){
        fill(this->out_add_rd_busy.begin(), this->out_add_rd_busy.end(), true);
    }
    else{
        if(this->out_add_rd_busy.front())
            fill(this->out_add_rd_busy.begin(), this->out_add_rd_busy.end(), false);
    }
    
    //prefetch b row
    // clear(this->out_mem_rd_pkg);
    for(int i=0;i<this->b_channel_num;i++){
        if(!this->b_fetcher_working[i])
            this->read_next_b_row(clock, i);
        if(this->b_fetcher_working[i]){
            this->out_mem_rd_pkg->push(mem_pkg{"b", this->b_acc_seq[this->b_prefetch_index[i]].first, this->b_read_row_index[i], 1, true, clock});
            this->b_read_row_index[i] += 1;
            if(this->b_read_row_index[i] == this->b_acc_seq[this->b_prefetch_index[i]].second)
                this->b_fetcher_working[i] = false;
        }
    }
    //prefetch ps row
    int b_rd_pkg_num = this->out_mem_rd_pkg->size();
    while(this->out_mem_rd_pkg->size()-b_rd_pkg_num<this->ps_channel_num && this->ps_cache_pre_delete_num == 0){
        if(!this->ps_fetcher_working)
            this->read_next_ps_row(clock);
        if(this->ps_fetcher_working){
            this->out_mem_rd_pkg->push(mem_pkg{"ps", this->ps_prefetch_seq[this->ps_prefetch_index], this->ps_read_row_index, 1, true, clock});
            this->ps_read_row_index += 1;
            if(this->ps_read_row_index == this->ps_row_size[this->ps_prefetch_seq[this->ps_prefetch_index]])
                this->ps_fetcher_working = false;
        }
        else
            break;
    }
    //rewrite
    // clear(this->out_mem_wr_value);
    // clear(this->out_mem_wr_comp);
    // clear(this->out_mem_wr_name);
    set<ps_rewr_index> erase_set = {};
    for(auto &re_row : this->rewrite_stagging){
        while(this->out_mem_wr_value->size() < this->rewrite_max_num && !re_row.second.elem.empty()){
            this->out_mem_wr_name->push(make_pair(re_row.first.stamp, re_row.first.csr_row));
            this->out_mem_wr_value->push(re_row.second.elem.front());
            re_row.second.elem.pop();
            if(re_row.second.elem.empty() && re_row.second.ape_rewr_end){
                this->out_mem_wr_comp->push(true);
                erase_set.emplace(re_row.first);
            }
            else{
                this->out_mem_wr_comp->push(false);
            }
        }
    }
    this->ps_mem_wr_traffic += this->out_mem_wr_value->size();
    for(auto &re_index : erase_set){
        this->rewrite_stagging.erase(re_index);
        //last rewrite is end
        if(this->waiting_last_rewrite.find(re_index.csr_row) != this->waiting_last_rewrite.end()){
            bool flag = true;
            for(auto &re_row : this->rewrite_stagging){
                if(re_row.first.csr_row == re_index.csr_row){
                    flag = false;
                    break;
                }
            }
            if(flag){
                finish_rewrite_csr_row.emplace(re_index.csr_row);
                this->waiting_last_rewrite.erase(re_index.csr_row);
            }
        }
    }
}

bool mat_b_cache::clock_apply(cint clock){
    //get read result
    this->clock_apply_mem_process(clock);
    //get requir from mpe
    this->clock_apply_mpe_preprocess(clock);
    //get requir from ape
    this->clock_apply_ape_preprocess(clock);
    //cache hit and miss
    this->b_read_process(clock);
    this->ps_read_process(clock);

    // clear(*this->out_mem_rd_pkg);
    // clear(*this->out_mem_rd_pkg_b);
    // clear(*this->out_mem_rd_pkg_ps);
    clear(*this->in_mem_rd_data);  

    for(int i=0;i<this->b_channel_num;i++){
        // clear(*this->out_mult_rd_data[i]);
        // clear(*this->out_mult_rd_valid[i]);
        clear(*this->in_mult_rd_pkg[i]);
        // clear(*this->in_add_wr_value[i]);  
    }

    for(int i=0;i<this->ps_channel_num;i++){
        // clear(*this->out_add_rd_data[i]);
        // clear(*this->out_add_rd_valid[i]);
        clear(*this->in_add_rd_pkg[i]);
        // clear(*this->in_add_wr_value[i]);  
    }

    for(int i=0;i<this->ps_channel_num;i++){
        clear(*this->in_add_wr_valid[i]);
        clear(*this->in_add_wr_comp[i]);
        clear(*this->in_add_wr_value[i]);  
    }

    return true;
}

void mat_b_cache::delete_b_cache_row(int row_index, long end_mult_index, int now_mult_index){
    assert(this->b_valid_cache.find(row_index) != this->b_valid_cache.end() && 
           this->b_valid_cache[row_index].read_cnt == 0);
    this->b_invalid_cache_size += this->b_valid_cache[row_index].info.b_row_size;
    this->b_valid_cache_size -= this->b_valid_cache[row_index].info.b_row_size;
    //init invalid cache row
    b_invalid_cache_row invalid_row = b_invalid_cache_row{max_reuse_dis_b, max_prefetch_dis_b, end_mult_index, this->b_valid_cache[row_index].info};
    this->b_valid_cache.erase(row_index);
    //get prefetch distance
    invalid_row.prefetch_dis = invalid_row.info.b_row_size/prefetch_dis_factor_b;
    invalid_row.info.b_row_size = -1;
    //get reuse distance
    for(int i=b_max_mult_index+1;i<this->b_acc_seq.size();i++){
        if(invalid_row.info.csr_row == this->b_acc_seq[i].first){
            invalid_row.reuse_dis = i-this->b_max_mult_index;
            break;
        }
    }
    //update other invalid row
    // for(b_invalid_cache_row &row : this->b_invalid_cache){
    //     if(row.reuse_dis != max_reuse_dis_b){
    //         row.reuse_dis -= 1;
    //     }
    // }
    //push and sort
    this->b_invalid_cache.push_back(invalid_row);
    cmp_mult_index = now_mult_index;
    sort(this->b_invalid_cache.begin(), this->b_invalid_cache.end(), reverse_dis_cmp_b);
}

void mat_b_cache::delete_ps_cache_row(int row_index){
    assert(this->ps_valid_cache.find(row_index) != this->ps_valid_cache.end() && 
           this->ps_valid_cache[row_index].read_cnt == 0);
    //init invalid cache row
    if(this->ps_valid_cache[row_index].info.elem.size() == 0){
        this->ps_valid_cache.erase(row_index);
        return;
    }
    ps_invalid_cache_row invalid_row = ps_invalid_cache_row{__INT_MAX__, 0, (int)this->ps_valid_cache[row_index].info.elem.size(), this->ps_valid_cache[row_index].info};
    int original_ps_row_size = invalid_row.remain_size;
    this->ps_valid_cache.erase(row_index);
    //if there is pre delete, then fix it
    if(invalid_row.remain_size <= this->ps_cache_pre_delete_num){
        this->ps_cache_pre_delete_num -= invalid_row.remain_size;
        return;
    }
    else{
        invalid_row.remain_size -= this->ps_cache_pre_delete_num;
        this->ps_cache_pre_delete_num = 0;
    }
    this->ps_invalid_cache_size += invalid_row.remain_size;
    this->ps_valid_cache_size -= invalid_row.remain_size;
    //get prefetch distance
    invalid_row.prefetch_dis = original_ps_row_size/prefetch_dis_factor_ps;
    invalid_row.reuse_dis = __INT_MAX__ - invalid_row.prefetch_dis;
    //get reuse distance
    for(int i=ps_max_add_index+1;i<this->ps_acc_seq.size();i++){
        if(invalid_row.info.csr_row == this->ps_acc_seq[i]){
            invalid_row.reuse_dis = i-this->ps_max_add_index;
            break;
        }
    }
    //update other invalid row
    for(ps_invalid_cache_row &row : this->ps_invalid_cache){
        if(row.reuse_dis + row.prefetch_dis != __INT_MAX__)
            row.reuse_dis -= 1;
    }
    //push and sort
    this->ps_invalid_cache.push_back(invalid_row);
    sort(this->ps_invalid_cache.begin(), this->ps_invalid_cache.end(), reverse_dis_cmp_ps);
}

bool mat_b_cache::b_read_process(cint clock){
    //vector<pair<pair<int, int>, int>> access_list = {};
    vector<b_cache_cmd> skip_cmd = {};
    // the number of cmd which can be processed per cycle
    int cmd_count = 0;

    while(cmd_count < cmd_process_per_cycle && !this->b_read_cmd_stagging.empty()){
        //--------------------------------------------get cmd--------------------------------------------//
        // get cmd
        b_cache_cmd rcmd = this->b_read_cmd_stagging.top();
        this->b_read_cmd_stagging.pop();
        // if already skip any cmd, the new cmd can only have the same timestamp
        if(!skip_cmd.empty() && rcmd.clock >= skip_cmd.front().clock){
            skip_cmd.push_back(rcmd);
            // this->b_cache_stall += 1;
            break;
        }
        //-------------------------------------------cache hit-------------------------------------------//
        if(this->b_valid_cache.find(rcmd.csr_row) != this->b_valid_cache.end() && 
           this->b_valid_cache[rcmd.csr_row].info.size > rcmd.csr_col){
            this->b_read_output_stagging[rcmd.cn].push(b_cache_access{this->b_latest_avalivle_time(clock, rcmd.cn), rcmd.csr_row, rcmd.csr_col});
            cmd_count++;
            if(cache_debug_info_output)
                cout<<this->device_name<<": b cache hit: ("<<rcmd.csr_row<<", "<<rcmd.csr_col<<")"<<endl;
            this->b_ch += 1;
        }
        //-------------------------------------------cache miss-------------------------------------------//
        else{
            if(cache_debug_info_output)
                cout<<this->device_name<<": b cache miss"<<endl;
            //search L/S table
            if(this->ls_table_count >= this->mshr_ls_ts){
                skip_cmd.push_back(rcmd);
                // this->b_cache_stall += 1;
                // break;
                continue;
            }

            bool first_miss = false;
            int replace_cache_row = -1;
            //search mshr table
            int mshr_entry = -1, invalid_entry = -1;
            if(this->b_mshr_map.find(rcmd.csr_row) != this->b_mshr_map.end()){
                for(int index : this->b_mshr_map[rcmd.csr_row]){
                    if(this->b_mshr_table[index].begin <= rcmd.csr_col && rcmd.csr_col <= this->b_mshr_table[index].end){
                        mshr_entry = index;
                        break;
                    }
                }
            }
            //first missing
            if(mshr_entry == -1){
                first_miss = true;
                //find empty mshr entry
                for(int i=0;i<this->mshr_ts;i++){
                    if(!this->b_mshr_table[i].valid){
                        invalid_entry = i;
                        break;
                    }
                }
                //first missing check
                if(invalid_entry == -1){
                    skip_cmd.push_back(rcmd);
                    // this->b_cache_stall += 1;
                    // break;
                    continue;
                }
            }

            mshr_entry = (first_miss) ? invalid_entry : mshr_entry;
            //first missing process
            if(first_miss){
                //mshr map process
                if(this->b_mshr_map.find(rcmd.csr_row) == this->b_mshr_map.end())
                    this->b_mshr_map[rcmd.csr_row] = {};
                this->b_mshr_map[rcmd.csr_row].push_back(mshr_entry);
                //mshr process
                this->b_mshr_table[mshr_entry].valid = true;
                this->b_mshr_table[mshr_entry].csr_row = rcmd.csr_row;
                this->b_mshr_table[mshr_entry].begin = (rcmd.csr_col/this->cache_row_size)*this->cache_row_size;
                this->b_mshr_table[mshr_entry].end = this->b_mshr_table[mshr_entry].begin + this->cache_row_size - 1;
            }

            cmd_count++;
            if(cache_debug_info_output){
                if(first_miss)
                    cout<<this->device_name<<": first missing with MSHR entry: "<<mshr_entry<<endl;
                else
                    cout<<this->device_name<<": matched missing with MSHR entry "<<mshr_entry<<endl;
                cout<<this->device_name<<": cache miss: ("<<rcmd.csr_row<<", "<<rcmd.csr_col<<")"<<endl;
            }
            if(first_miss)
                this->b_fcm += 1;
            else
                this->b_nfcm += 1;

            //shared process in load/store table
            if(this->b_mshr_ls_table[mshr_entry].find(rcmd.csr_col) == this->b_mshr_ls_table[mshr_entry].end())
                this->b_mshr_ls_table[mshr_entry][rcmd.csr_col] = {};
            this->b_mshr_ls_table[mshr_entry][rcmd.csr_col].push_back(rcmd.cn);
            this->ls_table_count += 1;
        }
    }
    if(!skip_cmd.empty())
        this->b_cache_stall += 1;
    for(b_cache_cmd rcmd : skip_cmd)
        this->b_read_cmd_stagging.push(rcmd);
    return true;
}

bool mat_b_cache::ps_read_process(cint clock){
    //vector<pair<pair<int, int>, int>> access_list = {};
    vector<ps_cache_cmd> skip_cmd = {};
    // the number of cmd which can be processed per cycle
    int cmd_count = 0;

    while(cmd_count < cmd_process_per_cycle && !this->ps_read_cmd_stagging.empty()){
        //--------------------------------------------get cmd--------------------------------------------//
        // get cmd
        ps_cache_cmd rcmd = this->ps_read_cmd_stagging.top();
        this->ps_read_cmd_stagging.pop();
        // if already skip any cmd, the new cmd can only have the same timestamp
        if(!skip_cmd.empty() && rcmd.clock >= skip_cmd.front().clock){
            skip_cmd.push_back(rcmd);
            
            break;
        }
        //-------------------------------------------cache hit-------------------------------------------//
        if(this->ps_valid_cache.find(rcmd.row) != this->ps_valid_cache.end() && 
           this->ps_valid_cache[rcmd.row].info.elem.size() > rcmd.col){
            this->ps_read_output_stagging[rcmd.cn].push(ps_cache_access{this->ps_latest_avalivle_time(clock, rcmd.cn), 
                                                        this->ps_valid_cache[rcmd.row].info.elem[rcmd.col]});
            cmd_count++;
            if(cache_debug_info_output)
                cout<<this->device_name<<": b cache hit: ("<<rcmd.row<<", "<<rcmd.col<<")"<<endl;
            this->ps_ch += 1;
        }
        //-------------------------------------------cache miss-------------------------------------------//
        else{
            if(cache_debug_info_output)
                cout<<this->device_name<<": b cache miss"<<endl;
            //search L/S table
            if(this->ls_table_count >= this->mshr_ls_ts){
                skip_cmd.push_back(rcmd);
                // this->ps_cache_stall += 1;
                // break;
                continue;
            }

            bool first_miss = false;
            int replace_cache_row = -1;
            //search mshr table
            int mshr_entry = -1, invalid_entry = -1;
            if(this->ps_mshr_map.find(rcmd.row) != this->ps_mshr_map.end()){
                for(int index : this->ps_mshr_map[rcmd.row]){
                    if(this->ps_mshr_table[index].begin <= rcmd.col && rcmd.col <= this->ps_mshr_table[index].end){
                        mshr_entry = index;
                        break;
                    }
                }
            }
            //first missing
            if(mshr_entry == -1){
                first_miss = true;
                //find empty mshr entry
                for(int i=0;i<this->mshr_ts;i++){
                    if(!this->ps_mshr_table[i].valid){
                        invalid_entry = i;
                        break;
                    }
                }
                //first missing check
                if(invalid_entry == -1){
                    skip_cmd.push_back(rcmd);
                    // this->ps_cache_stall += 1;
                    // break;
                    continue;
                }
            }

            mshr_entry = (first_miss) ? invalid_entry : mshr_entry;
            //first missing process
            if(first_miss){
                //mshr map process
                if(this->ps_mshr_map.find(rcmd.row) == this->ps_mshr_map.end())
                    this->ps_mshr_map[rcmd.row] = {};
                this->ps_mshr_map[rcmd.row].push_back(mshr_entry);
                //mshr process
                this->ps_mshr_table[mshr_entry].valid = true;
                this->ps_mshr_table[mshr_entry].row = rcmd.row;
                this->ps_mshr_table[mshr_entry].begin = (rcmd.col/this->cache_row_size)*this->cache_row_size;
                this->ps_mshr_table[mshr_entry].end = this->ps_mshr_table[mshr_entry].begin + this->cache_row_size - 1;
            }

            cmd_count++;
            if(cache_debug_info_output){
                if(first_miss)
                    cout<<this->device_name<<": first missing with MSHR entry: "<<mshr_entry<<endl;
                else
                    cout<<this->device_name<<": matched missing with MSHR entry "<<mshr_entry<<endl;
                cout<<this->device_name<<": cache miss: ("<<rcmd.row<<", "<<rcmd.col<<")"<<endl;
            }
            if(first_miss)
                this->ps_fcm += 1;
            else
                this->ps_nfcm += 1;

            //shared process in load/store table
            if(this->ps_mshr_ls_table[mshr_entry].find(rcmd.col) == this->ps_mshr_ls_table[mshr_entry].end())
                this->ps_mshr_ls_table[mshr_entry][rcmd.col] = {};
            this->ps_mshr_ls_table[mshr_entry][rcmd.col].push_back(rcmd.cn);
            this->ls_table_count += 1;
        }
    }
    if(!skip_cmd.empty())
        this->ps_cache_stall += 1;
    for(ps_cache_cmd rcmd : skip_cmd)
        this->ps_read_cmd_stagging.push(rcmd);
    return true;
}

cint mat_b_cache::b_latest_avalivle_time(cint clock, int cn){
    if(this->b_read_output_stagging[cn].empty())
        return clock+this->delay;
    else{
        assert((unsigned long)this->b_read_output_stagging[cn].back().clock <= clock+this->delay);
        return clock+this->delay;
        // return max(clock+this->delay, (unsigned long)this->b_read_output_stagging[cn].back().clock + 1);
    }
}

cint mat_b_cache::ps_latest_avalivle_time(cint clock, int cn){
    if(this->ps_read_output_stagging[cn].empty())
        return clock+this->delay;
    else{
        assert((unsigned long)this->ps_read_output_stagging[cn].back().clock <= clock+this->delay);
        return clock+this->delay;
    }
}

void mat_b_cache::update_b_mshr(cint clock, int csr_row, int csr_col){
    // do not have this row
    if(this->b_mshr_map.find(csr_row) == this->b_mshr_map.end())
        return;
    int mshr_index = -1;
    for(int index : this->b_mshr_map[csr_row]){
        if(this->b_mshr_table[index].begin <= csr_col && csr_col <= this->b_mshr_table[index].end){
            mshr_index = index;
            break;
        }
    }
    // do not have this set
    if(mshr_index == -1)
        return;
    if(this->b_mshr_ls_table[mshr_index].find(csr_col) == this->b_mshr_ls_table[mshr_index].end())
        return;
    //process all ls
    vector<int> &ls_table = this->b_mshr_ls_table[mshr_index][csr_col];
    for(int pe_index : ls_table)
        this->b_read_output_stagging[pe_index].push(b_cache_access{this->b_latest_avalivle_time(clock, pe_index), csr_row, csr_col});
    //update ls size and ls table
    this->ls_table_count -= ls_table.size();
    this->b_mshr_ls_table[mshr_index].erase(csr_col);
    if(this->b_mshr_ls_table[mshr_index].empty()){
        this->b_mshr_table[mshr_index].valid = false;
        for(vector<int>::iterator it = this->b_mshr_map[csr_row].begin();it != this->b_mshr_map[csr_row].end();it++){
            if((*it) == mshr_index){
                this->b_mshr_map[csr_row].erase(it);
                break;
            }
        }
        if(this->b_mshr_map[csr_row].empty())
            this->b_mshr_map.erase(csr_row);
    }
}

void mat_b_cache::update_ps_mshr(cint clock, int csr_row, int csr_col){
    // do not have this row
    if(this->ps_mshr_map.find(csr_row) == this->ps_mshr_map.end())
        return;
    int mshr_index = -1;
    for(int index : this->ps_mshr_map[csr_row]){
        if(this->ps_mshr_table[index].begin <= csr_col && csr_col <= this->ps_mshr_table[index].end){
            mshr_index = index;
            break;
        }
    }
    // do not have this set
    if(mshr_index == -1)
        return;
    if(this->ps_mshr_ls_table[mshr_index].find(csr_col) == this->ps_mshr_ls_table[mshr_index].end())
        return;
    //process all ls
    vector<int> &ls_table = this->ps_mshr_ls_table[mshr_index][csr_col];
    for(int pe_index : ls_table)
        this->ps_read_output_stagging[pe_index].push(ps_cache_access{this->ps_latest_avalivle_time(clock, pe_index), 
                                                     this->ps_valid_cache[csr_row].info.elem[csr_col]});
    //update ls size and ls table
    this->ls_table_count -= ls_table.size();
    this->ps_mshr_ls_table[mshr_index].erase(csr_col);
    if(this->ps_mshr_ls_table[mshr_index].empty()){
        this->ps_mshr_table[mshr_index].valid = false;
        for(vector<int>::iterator it = this->ps_mshr_map[csr_row].begin();it != this->ps_mshr_map[csr_row].end();it++){
            if((*it) == mshr_index){
                this->ps_mshr_map[csr_row].erase(it);
                break;
            }
        }
        if(this->ps_mshr_map[csr_row].empty())
            this->ps_mshr_map.erase(csr_row);
    }
}

void mat_b_cache::clock_apply_mem_process(cint clock){
    //get read result
    pair<string, coo> p;
    while(!this->in_mem_rd_data->empty()){
        p = this->in_mem_rd_data->front();
        //get b
        if(p.first.compare("b") == 0){
            if(this->b_valid_cache.find(coo2csr_b[p.second.r]) == this->b_valid_cache.end())
                assert(false);
            
            this->b_mem_rd_traffic += 1;
            b_cache_row &b_cr = this->b_valid_cache[coo2csr_b[p.second.r]].info;
            if(b_cr.next_coo_col == p.second.c){
                b_cr.size += 1;
                if(b_cr.size < b_cr.b_row_size)
                    b_cr.next_coo_col = mat_b_read[b_cr.csr_row][b_cr.size].c;
                else
                    b_cr.next_coo_col = -1;
                this->update_b_mshr(clock, coo2csr_b[p.second.r], b_cr.size-1);
            }
            else
                assert(false);
        }
        //get ps
        else if(p.first.compare("ps") == 0){
            if(this->ps_valid_cache.find(coo2csr_ps[p.second.r]) == this->ps_valid_cache.end())
                assert(false);
            this->ps_mem_rd_traffic += 1;
            ps_cache_row &ps_cr = this->ps_valid_cache[coo2csr_ps[p.second.r]].info;
            ps_cr.elem.push_back(p.second);
            this->update_ps_mshr(clock, coo2csr_ps[p.second.r], ps_cr.elem.size()-1);
            if(ps_cr.elem.size() == this->ps_row_size[ps_cr.csr_row])
                this->ps_row_size.erase(ps_cr.csr_row);
        }
        else{
            assert(false);
        }
        this->in_mem_rd_data->pop();
    }
}

void mat_b_cache::clock_apply_mpe_preprocess(cint clock){
    //get requir from mpe
    for(int i=0;i<this->b_channel_num;i++){
        while(!this->in_mult_rd_pkg[i]->empty()){
            this->b_read_cmd_stagging.push(b_cache_cmd{clock, i, this->in_mult_rd_pkg[i]->front().mat_row, this->in_mult_rd_pkg[i]->front().mat_colmn});
            this->in_mult_rd_pkg[i]->pop();
        }
        // if(this->in_mult_rd_pkg[i].valid){
        //     this->b_read_cmd_stagging.push(b_cache_cmd{clock, i, this->in_mult_rd_pkg[i].mat_row, this->in_mult_rd_pkg[i].mat_colmn});
        // }
    }
}

void mat_b_cache::update_ps_cache_row_info(cint clock, ps_valid_cache_row &ps_row){
    ps_wr_occupy_row.erase(ps_row.info.csr_row);
    ps_row.read_cnt -= 1;
    ps_row.rewrite_index = 0;
    //update dirty info
    mat_ps_read[ps_row.info.csr_row].clear();
    mat_ps_read_size[ps_row.info.csr_row] = ps_row.info.elem.size();
    //become invalid and write back
    if(ps_row.read_cnt <= 0){
        //get rewrite info
        if(!ps_row.info.elem.empty()){
            mat_ps_read[ps_row.info.csr_row].assign(ps_row.info.elem.begin(), ps_row.info.elem.end());
            // ps_rewr_index re_index(clock, mat_ps_read_stamp[ps_row.info.csr_row], ps_row.info.csr_row);
            // this->rewrite_stagging.emplace(make_pair(re_index, ps_rewr_info()));
            // this->rewrite_stagging[re_index].ape_rewr_end = true;
            // mat_ps_wr_cnt[{ps_row.info.csr_row, mat_ps_read_stamp[ps_row.info.csr_row]}] += 1;
            // for(coo p : ps_row.info.elem)
            //     this->rewrite_stagging[re_index].elem.push(p);
        }
        this->delete_ps_cache_row(ps_row.info.csr_row);
    }
    else
        assert(false);
}

void mat_b_cache::remove_exceeding_limit_row(cint clock, int csr_row){
    ps_valid_cache_row &ps_row = this->ps_valid_cache[csr_row];
    int remain_size = ps_row.info.elem.size();
    ps_row.info.elem.clear();
    //if there is pre delete, then fix it
    if(remain_size <= this->ps_cache_pre_delete_num){
        this->ps_cache_pre_delete_num -= remain_size;
    }
    else{
        remain_size -= this->ps_cache_pre_delete_num;
        this->ps_cache_pre_delete_num = 0;
    }
    if(remain_size != 0){
        //update invalid cache
        this->ps_invalid_cache_size += remain_size;
        this->ps_valid_cache_size -= remain_size;
        if(this->ps_invalid_cache.back().info.csr_row == -1)
            this->ps_invalid_cache.back().remain_size += remain_size;
        else
            this->ps_invalid_cache.push_back(ps_invalid_cache_row{__INT_MAX__, 0, remain_size, ps_cache_row{-1}});
    }
    //update mat-ps and cache
    update_ps_cache_row_info(clock, ps_row);
}

void mat_b_cache::ape_write_back_in_cache(cint clock, int pe_index, int csr_row, coo write_back_value, bool wr_comp){
    //get elem
    if(this->ps_wr_stagging.find(csr_row) == this->ps_wr_stagging.end())
        this->ps_wr_stagging.insert({csr_row, {}});
    this->ps_wr_stagging[csr_row].push_back(write_back_value);

    //bigger than max size
    if(this->ps_wr_stagging[csr_row].size() > this->ps_max_size){
        //get new rewrite
        ps_rewr_index wr_index = ps_rewr_index(clock, mat_ps_read_stamp[csr_row], csr_row);
        this->rewrite_stagging.emplace(make_pair(wr_index, ps_rewr_info()));
        for(coo &p : this->ps_wr_stagging[csr_row])
            this->rewrite_stagging[wr_index].elem.push(p);
        this->rewrite_stagging[wr_index].ape_rewr_end = wr_comp;
        mat_ps_wr_cnt[{csr_row, mat_ps_read_stamp[csr_row]}] += 1;
        mat_ps_read_stamp[csr_row] += 1;
        mat_ps_wr_cnt.insert({{csr_row, mat_ps_read_stamp[csr_row]}, 0});
        //get new rewrite-ape map
        assert(this->rewrite_ape_stagging.find(pe_index)==this->rewrite_ape_stagging.end());
        if(!wr_comp)
            this->rewrite_ape_stagging.emplace(make_pair(pe_index, wr_index));
        this->ps_wr_stagging.erase(csr_row);
        //waiting for rd finish
        if(ps_rd_occupy_row.find(csr_row) != ps_rd_occupy_row.end()){
            assert(this->rd_occupy_stagging_row.find(csr_row) == this->rd_occupy_stagging_row.end());
            this->rd_occupy_stagging_row.emplace(csr_row);
        }
        else{
            this->remove_exceeding_limit_row(clock, csr_row);
        }
    }
    //smaller than max size and complete
    else if(wr_comp){
        ps_valid_cache_row &ps_row = this->ps_valid_cache[csr_row];
        //get more number and use ps invalid
        assert(this->ps_wr_stagging[csr_row].size() >= ps_row.info.elem.size());
        int exceed_num = this->ps_wr_stagging[csr_row].size() - ps_row.info.elem.size();
        this->use_ps_invalid_cache(exceed_num, this->ps_valid_cache_size);
        this->ps_cache_pre_delete_num += exceed_num;
        //update cache elem and read cnt
        ps_row.info.elem.assign(this->ps_wr_stagging[csr_row].begin(), this->ps_wr_stagging[csr_row].end());
        this->ps_wr_stagging.erase(csr_row);
        this->update_ps_cache_row_info(clock, ps_row);
    }
}

void mat_b_cache::clock_apply_ape_preprocess(cint clock){
    //occupy rd
    set<int> erase_set = {};
    for(int csr_row : this->rd_occupy_stagging_row){
        if(ps_rd_occupy_row.find(csr_row) == ps_rd_occupy_row.end()){
            this->remove_exceeding_limit_row(clock, csr_row);
            erase_set.emplace(csr_row);
        }
    }
    for(int csr_row : erase_set)
        this->rd_occupy_stagging_row.erase(csr_row);
    //get requir from ape
    for(int i=0;i<this->ps_channel_num;i++){
        while(!this->in_add_rd_pkg[i]->empty()){
            this->ps_read_cmd_stagging.push(ps_cache_cmd{clock, i, this->in_add_rd_pkg[i]->front().mat_row, this->in_add_rd_pkg[i]->front().mat_colmn});
            this->in_add_rd_pkg[i]->pop();
        }
        // if(this->in_add_rd_pkg[i].valid)
        //     this->ps_read_cmd_stagging.push(ps_cache_cmd{clock, i, this->in_add_rd_pkg[i].mat_row, this->in_add_rd_pkg[i].mat_colmn});
    }
    //get new ps wr
    for(int i=0;i<this->ps_channel_num;i++){
        while(!this->in_add_wr_valid[i]->empty()){
            int csr_row = coo2csr_ps[this->in_add_wr_value[i]->front().r];
            //rewrite in stagging
            if(this->rewrite_ape_stagging.find(i) != this->rewrite_ape_stagging.end()){
                ps_rewr_index index = this->rewrite_ape_stagging[i];
                this->rewrite_stagging[index].elem.push(this->in_add_wr_value[i]->front());
                if(this->in_add_wr_comp[i]->front()){
                    this->rewrite_ape_stagging.erase(i);
                    this->rewrite_stagging[index].ape_rewr_end = true;
                }
            }
            //rewrite in cache
            else if(this->ps_valid_cache.find(csr_row) != this->ps_valid_cache.end()){
                this->ape_write_back_in_cache(clock, i, csr_row, this->in_add_wr_value[i]->front(), this->in_add_wr_comp[i]->front());
            }
            else{
                for(int i=0;i<b_channel_num;i++)
                    cout<<"ape: "<<i<<" task index: "<<this->ps_add_index[i]<<endl;
                cout<<i<<" "<<this->ps_add_index[i]<<endl;
                cout<<this->ps_prefetch_index<<endl;
                assert(false);
            }
            if(this->in_add_wr_comp[i]->front() && finish_add_csr_row.find(csr_row) != finish_add_csr_row.end()){
                //last rewrite
                bool end_flag = true;
                for(auto &re_row : this->rewrite_stagging){
                    if(re_row.first.csr_row == csr_row){
                        end_flag = false;
                        break;
                    }
                }
                //no other rewrite in stagging
                finish_add_csr_row.erase(csr_row);
                if(end_flag){
                    finish_mem_csr_row.insert(csr_row);
                }
                else{
                    this->waiting_last_rewrite.insert(csr_row);
                }
            }
            
            
            this->in_add_wr_valid[i]->pop();
            this->in_add_wr_value[i]->pop();
            this->in_add_wr_comp[i]->pop();
        }
        // if(this->in_add_wr_valid[i]){
        //     int csr_row = coo2csr_ps[this->in_add_wr_value[i].r];
        //     //rewrite in stagging
        //     if(this->rewrite_ape_stagging.find(i) != this->rewrite_ape_stagging.end()){
        //         ps_rewr_index index = this->rewrite_ape_stagging[i];
        //         this->rewrite_stagging[index].elem.push(this->in_add_wr_value[i]);
        //         if(this->in_add_wr_comp[i]){
        //             this->rewrite_ape_stagging.erase(i);
        //             this->rewrite_stagging[index].ape_rewr_end = true;
        //         }
        //     }
        //     //rewrite in cache
        //     else if(this->ps_valid_cache.find(csr_row) != this->ps_valid_cache.end()){
        //         this->ape_write_back_in_cache(clock, i, csr_row, this->in_add_wr_value[i], this->in_add_wr_comp[i]);
        //     }
        //     else
        //         assert(false);
        // }
    }
}

// void mat_b_cache::set_mult_input_pkg(cint clock, int cn, queue<mem_pkg> &pkg){
//     clear(this->in_mult_rd_pkg[cn]);
//     this->in_mult_rd_pkg[cn].swap(pkg);
// }

void mat_b_cache::get_mult_output(cint clock, int cn, bool& rd_busy){
    // clear(data);
    // clear(valid);
    // data.swap(this->out_mult_rd_data[cn]);
    // valid.swap(this->out_mult_rd_valid[cn]);
    rd_busy = this->out_mult_rd_busy[cn];
}

// void mat_b_cache::set_add_rd_input(cint clock, int cn, queue<mem_pkg> &pkg){
//     clear(this->in_add_rd_pkg[cn]);
//     this->in_add_rd_pkg[cn].swap(pkg);
// }

void mat_b_cache::get_add_rd_output(cint clock, int cn, bool& rd_busy){
    // clear(data);
    // clear(valid);
    // data.swap(this->out_add_rd_data[cn]);
    // valid.swap(this->out_add_rd_valid[cn]);
    rd_busy = this->out_add_rd_busy[cn];
}

// void mat_b_cache::set_add_wr_input(cint clock, int cn, queue<bool> &valid, queue<bool> &comp, queue<coo> &value){
//     clear(this->in_add_wr_valid[cn]);
//     clear(this->in_add_wr_comp[cn]);
//     clear(this->in_add_wr_value[cn]);
//     this->in_add_wr_valid[cn].swap(valid);
//     this->in_add_wr_comp[cn].swap(comp);
//     this->in_add_wr_value[cn].swap(value);
//     // this->in_add_wr_valid[cn] = valid;
//     // this->in_add_wr_comp[cn] = comp;
//     // this->in_add_wr_value[cn] = value;
// }

void mat_b_cache::get_add_wr_output(cint clock, int cn, bool& wr_busy){
    wr_busy = this->out_add_wr_busy[cn];
}

// void mat_b_cache::set_mem_rd_input(cint clock, queue<pair<string, coo>> &data){
//     clear(this->in_mem_rd_data);
//     this->in_mem_rd_data.swap(data);
// }

// void mat_b_cache::get_mem_rd_output(cint clock, queue<mem_pkg> &rd_pkg){
//     clear(rd_pkg);
//     rd_pkg.swap(this->out_mem_rd_pkg);
// }

void mat_b_cache::set_mem_wr_input(cint clock, bool wr_busy){
    this->in_mem_wr_busy = wr_busy;
}

// void mat_b_cache::get_mem_wr_output(cint clock, queue<bool> &comp, queue<pair<int, int>> &name, queue<coo> &value){
//     clear(comp);
//     comp.swap(this->out_mem_wr_comp);
//     clear(name);
//     name.swap(this->out_mem_wr_name);
//     clear(value);
//     value.swap(this->out_mem_wr_value);
// }

void mat_b_cache::print_device_status(ofstream &fout){

}

#endif