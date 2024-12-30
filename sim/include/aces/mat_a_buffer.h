#ifndef MAT_A_BUFFER
#define MAT_A_BUFFER

#include<base.h>
#include<device_base.h>

// all coor in program is based on the coor in csr format, coor in coo format is pure data

struct a_cache_cmd{
    cint clock;
    int cn;
    int csr_row, csr_col;

    a_cache_cmd(cint clock = 0, int cn = -1, 
              int csr_row = -1, int csr_col = -1){
        this->clock = clock;
        this->cn = cn;
        this->csr_row = csr_row;
        this->csr_col = csr_col;
    }

    bool operator<(const a_cache_cmd &p) const{
        if(this->clock == p.clock)
            return this->cn < p.cn;
        return this->clock < p.clock;
    }
    bool operator>(const a_cache_cmd &p) const{
        if(this->clock == p.clock)
            return this->cn > p.cn;
        return this->clock > p.clock;
    }
};

struct a_cache_access{
    cint clock;
    int csr_row;
    int csr_col;

    a_cache_access(cint clock = 0, int csr_row = -1, int csr_col = -1){
        this->clock = clock;
        this->csr_row = csr_row;
        this->csr_col = csr_col;
    }
};

struct a_cache_cmd_great{
    bool operator()(const a_cache_cmd &p1, const a_cache_cmd &p2){
        return p1 > p2;
    }
};

class mat_a_buffer : public device_base{
private:
    //------------------Mult side(read)------------------//
    //output data
    vector<coo> out_mult_rd_data;
    vector<bool> out_mult_rd_valid;
    vector<bool> out_mult_rd_busy;
    //input data(don't use param "num" in the pkg)
    vector<mem_pkg> in_mult_rd_pkg;
    //-----------------Memory side(read)-----------------//
    //output data 16 channel for 16 mpes, 16 channel for 16 apes
    queue<mem_pkg> out_mem_rd_pkg;
    //input data
    queue<pair<string, coo>> in_mem_rd_data;
    //-------------------Inner storage-------------------//
    map<pair<int, int>, pair<int, int>> coo2csr_map;
    //<<csr row, csr col>, read cnt>
    map<pair<int, int>, int> a_buffer;
    map<pair<int, int>, bool> a_valid;
    //-------------------Inner param-------------------//
    int a_channel_num;
    int mem_channel_num;
    int delay;
    //--process cmd--//
    int cmd_process_per_cycle;
    int read_cmd_max_size;
    vector<queue<a_cache_access>> a_read_output_stagging;
    priority_queue<a_cache_cmd, vector<a_cache_cmd>, a_cache_cmd_great> a_read_cmd_stagging;

    //--prefetch a--//
    //(1 prefetcher only and prefetch 16 elem per cycle)
    bool a_fetcher_working;
    vector<pair<int, int>> a_acc_seq;
    int a_max_prefetch_num;
    int a_prefetch_index;
    //csr col
    int a_read_row_index;

    //mem side load monitoring
        //the max req num is mem-channel-num in every cycle
    int mem_rd_req_cnt, mem_wr_req_cnt;

        //with MSHR
    //<<coo row, coo col>, pe index>
    map<pair<int, int>, vector<int>> mshr_table;
    
        //performance
    bool p_check;
    //cache hit
    long ch;
    //cache miss and not in MSHR
    long fcm;
    //cache miss and in MSHR
    long nfcm;
    
    bool a_read_process(cint clock);
    void update_a_mshr(cint clock, pair<int, int> csr_coor);
    cint a_latest_avalivle_time(cint clock, int cn);
    void clock_apply_mpe_preprocess(cint clock);
    void clock_apply_mem_process(cint clock);
public:
    mat_a_buffer(int cn, int delay, int max_pre_row, int cmd_per_c, int mem_cn, string name);
    ~mat_a_buffer();

    void reset_task();
    void add_a_acc_seq(vector<pair<int, int>> &add_seq);
    
    void set_mult_input_pkg(cint clock, int cn, mem_pkg pkg);
    void get_mult_output(cint clock, int cn, coo& data, bool& valid, bool& rd_busy);

    void set_mem_rd_input(cint clock, queue<pair<string, coo>> &data);
    void get_mem_rd_output(cint clock, queue<mem_pkg> &rd_pkg);

    bool clock_apply(cint clock);
    void clock_update(cint clock);
    
    void print_device_status(ofstream &fout);
    void performance_check_begin(cint clock);
    void performance_check_end(cint clock);
};

// read: "A, (0,1)" means: read (0,3), the column is in csr format (one at a time)
// name: A
//      |-----|
// (0,0)|(0,3)|(0,4)(0,5)
// (1,0)(1,2)(1,3)(1,5)(1,7)
// (2,1)(2,3)(2,4)

// read from input port -> read_cmd_stagging -> process

mat_a_buffer::mat_a_buffer(
int cn          =   cfg_spmm_cache_cn, 
int delay       =   cfg_spmm_cache_d, 
int max_pre_row =   cfg_spmm_cache_max_pre_a_num, 
int cmd_per_c   =   cfg_spmm_cache_cmd_per_clock, 
int mem_cn      =   cfg_spmm_mem_cn/4, 
string name     =   "a_buffer"
) : device_base(name){
    //------------------Mult side(read)------------------//
    //output data
    this->out_mult_rd_data.resize(cn);
    this->out_mult_rd_valid.resize(cn);
    this->out_mult_rd_busy.resize(cn);
    fill(this->out_mult_rd_valid.begin(), this->out_mult_rd_valid.end(), false);
    fill(this->out_mult_rd_busy.begin(), this->out_mult_rd_busy.end(), true);

    //input data(don't use param "num" in the pkg)
    this->in_mult_rd_pkg.resize(cn);
    //-----------------Memory side(read)-----------------//
    //output data
    clear(this->out_mem_rd_pkg);
    //input data
    clear(this->in_mem_rd_data);
    //-------------------Inner storage-------------------//
    this->coo2csr_map.clear();
    //<<csr row, csr col>, read cnt>
    this->a_buffer.clear();
    //-------------------Inner param-------------------//
    this->a_channel_num = cn;
    this->mem_channel_num = mem_cn;
    this->delay = delay;
    this->cmd_process_per_cycle = cmd_per_c;
    int read_cmd_max_size = cn*4;

    this->a_read_output_stagging.resize(this->a_channel_num);
    for(int i=0;i<this->a_channel_num;i++)
        clear(this->a_read_output_stagging[i]);
    while(!this->a_read_cmd_stagging.empty())
        this->a_read_cmd_stagging.pop();

    //--prefetch a--//
    //(1 prefetcher only and prefetch 16 elem per cycle)
    this->a_fetcher_working = false;
    this->a_max_prefetch_num = max_pre_row;
    this->a_acc_seq.clear();
    this->a_prefetch_index = -1;
    this->a_read_row_index = 0;

        //with MSHR
    //<<coo row, coo col>, pe index>
    this->mshr_table.clear();

        //performance
    this->p_check = false;
    //cache hit | first cache miss | non-first cache miss
    this->ch = 0;this->fcm = 0;this->nfcm = 0;
}

mat_a_buffer::~mat_a_buffer(){
}

void mat_a_buffer::reset_task(){
    //------------------Mult side(read)------------------//
    //output data
    fill(this->out_mult_rd_valid.begin(), this->out_mult_rd_valid.end(), false);
    fill(this->out_mult_rd_busy.begin(), this->out_mult_rd_busy.end(), true);

    //input data(don't use param "num" in the pkg)
    fill(this->in_mult_rd_pkg.begin(), this->in_mult_rd_pkg.end(), emp_pkg);
    //-----------------Memory side(read)-----------------//
    //output data
    clear(this->out_mem_rd_pkg);
    //input data
    clear(this->in_mem_rd_data);
    //-------------------Inner storage-------------------//
    this->coo2csr_map.clear();
    //<<csr row, csr col>, read cnt>
    this->a_buffer.clear();
    //-------------------Inner param-------------------//
    for(int i=0;i<this->a_channel_num;i++)
        clear(this->a_read_output_stagging[i]);
    while(!this->a_read_cmd_stagging.empty())
        this->a_read_cmd_stagging.pop();

    //--prefetch a--//
    //(1 prefetcher only and prefetch 16 elem per cycle)
    this->a_fetcher_working = false;
    this->a_acc_seq.clear();
    this->a_prefetch_index = -1;
    this->a_read_row_index = 0;

        //with MSHR
    //<<coo row, coo col>, pe index>
    this->mshr_table.clear();

        //performance
    this->p_check = false;
    //cache hit | first cache miss | non-first cache miss
    this->ch = 0;this->fcm = 0;this->nfcm = 0;
}

void mat_a_buffer::add_a_acc_seq(vector<pair<int, int>> &add_seq){
    //get new acc seq
    this->a_acc_seq.insert(a_acc_seq.begin(), add_seq.begin(), add_seq.end());
}

void mat_a_buffer::clock_update(cint clock){
    //input update
    fill(this->in_mult_rd_pkg.begin(), this->in_mult_rd_pkg.end(), mem_pkg{"", 0, 0, 0, false, 0});
    
    //mpe-side output update
    for(int cn = 0;cn<this->a_channel_num;cn++){
        if(!this->a_read_output_stagging[cn].empty() && this->a_read_output_stagging[cn].front().clock == clock){
            a_cache_access access = this->a_read_output_stagging[cn].front();
            this->out_mult_rd_data[cn] = mat_a_read[access.csr_row][access.csr_col];
            this->out_mult_rd_valid[cn] = true;
            this->a_read_output_stagging[cn].pop();
            this->a_buffer[make_pair(access.csr_row, access.csr_col)] -= 1;
            if(this->a_buffer[make_pair(access.csr_row, access.csr_col)] == 0){
                this->a_buffer.erase(make_pair(access.csr_row, access.csr_col));
                this->a_valid.erase(make_pair(access.csr_row, access.csr_col));
                this->coo2csr_map.erase(make_pair(mat_a_read[access.csr_row][access.csr_col].r, mat_a_read[access.csr_row][access.csr_col].c));
            }
        }
        else{
            this->out_mult_rd_valid[cn] = false;
        }
    }

    //read cmd stagging has limited size (each channel has 2 for rd and 2 for wr in average)
    if(this->a_read_output_stagging.size() >= this->a_channel_num*4){
        fill(this->out_mult_rd_busy.begin(), this->out_mult_rd_busy.end(), true);
    }
    else{
        if(this->out_mult_rd_busy.front())
            fill(this->out_mult_rd_busy.begin(), this->out_mult_rd_busy.end(), false);
    }

    //prefetch a row
    clear(this->out_mem_rd_pkg);
    while(this->out_mem_rd_pkg.size()<this->mem_channel_num){
        if(this->a_buffer.size() >= this->a_max_prefetch_num)
            break;
        
        if(this->a_prefetch_index >= (int)this->a_acc_seq.size() - 1)
            break;
        
        pair<int, int> next_read_a = this->a_acc_seq[this->a_prefetch_index+1];
        if(this->a_buffer.find(next_read_a) != this->a_buffer.end()){
            this->a_buffer[next_read_a] += 1;
            continue;
        }

        this->a_prefetch_index += 1;
        this->a_buffer[next_read_a] = 1;
        this->a_valid[next_read_a] = false;
        pair<int, int> coo_coor = {mat_a_read[next_read_a.first][next_read_a.second].r, mat_a_read[next_read_a.first][next_read_a.second].c};
        this->coo2csr_map.emplace(make_pair(coo_coor, next_read_a));
        this->out_mem_rd_pkg.push(mem_pkg{"a", next_read_a.first, next_read_a.second, 1, true, clock});
    }
}

bool mat_a_buffer::clock_apply(cint clock){
    //get read result
    this->clock_apply_mem_process(clock);
    //get requir from mpe
    this->clock_apply_mpe_preprocess(clock);
    //get requir from ape
    //cache hit and miss
    this->a_read_process(clock);
    return true;
}

bool mat_a_buffer::a_read_process(cint clock){
    //vector<pair<pair<int, int>, int>> access_list = {};
    vector<a_cache_cmd> skip_cmd = {};
    // the number of cmd which can be processed per cycle
    int cmd_count = 0;

    while(cmd_count < cmd_process_per_cycle && !this->a_read_cmd_stagging.empty()){
        //--------------------------------------------get cmd--------------------------------------------//
        // get cmd
        a_cache_cmd rcmd = this->a_read_cmd_stagging.top();
        this->a_read_cmd_stagging.pop();
        // if already skip any cmd, the new cmd can only have the same timestamp
        if(!skip_cmd.empty() && rcmd.clock >= skip_cmd.front().clock){
            skip_cmd.push_back(rcmd);
            break;
        }
        //-------------------------------------------cache hit-------------------------------------------//
        pair<int, int> rcmd_coor = make_pair(rcmd.csr_row, rcmd.csr_col);
        if(this->a_buffer.find(rcmd_coor) != this->a_buffer.end() && this->a_valid[rcmd_coor]){
            this->a_read_output_stagging[rcmd.cn].push(a_cache_access{this->a_latest_avalivle_time(clock, rcmd.cn), rcmd.csr_row, rcmd.csr_col});
            cmd_count++;
            if(cache_debug_info_output)
                cout<<this->device_name<<": a cache hit: ("<<rcmd.csr_row<<", "<<rcmd.csr_col<<")"<<endl;

            if(this->p_check)
                this->ch += 1;
        }
        //-------------------------------------------cache miss-------------------------------------------//
        else{
            if(cache_debug_info_output)
                cout<<this->device_name<<": a cache miss"<<endl;

            //search mshr table
            bool first_miss;
            if(this->mshr_table.find(rcmd_coor) != this->mshr_table.end()){
                this->mshr_table[rcmd_coor].push_back(rcmd.cn);
                first_miss = false;
            }
            else{
                this->mshr_table.emplace(make_pair(rcmd_coor, vector<int>{rcmd.cn}));
                first_miss = true;
            }

            cmd_count++;
            if(cache_debug_info_output){
                if(first_miss)
                    cout<<this->device_name<<": first missing with MSHR"<<endl;
                else
                    cout<<this->device_name<<": matched missing with MSHR"<<endl;
                cout<<this->device_name<<": cache miss: ("<<rcmd.csr_row<<", "<<rcmd.csr_col<<")"<<endl;
            }
            if(this->p_check){
                if(first_miss)
                    this->fcm += 1;
                else
                    this->nfcm += 1;
            }
        }
    }
    for(a_cache_cmd rcmd : skip_cmd)
        this->a_read_cmd_stagging.push(rcmd);
    return true;
}

cint mat_a_buffer::a_latest_avalivle_time(cint clock, int cn){
    if(this->a_read_output_stagging[cn].empty())
        return clock+this->delay;
    else
        return max(clock+this->delay, (unsigned long)this->a_read_output_stagging[cn].back().clock + 1);
}

void mat_a_buffer::update_a_mshr(cint clock, pair<int, int> csr_coor){
    // do not have this row
    if(this->mshr_table.find(csr_coor) == this->mshr_table.end())
        return;
    
    //process all ls
    vector<int> &ls_table = this->mshr_table[csr_coor];
    for(int pe_index : ls_table)
        this->a_read_output_stagging[pe_index].push(a_cache_access{this->a_latest_avalivle_time(clock, pe_index), csr_coor.first, csr_coor.second});
}

void mat_a_buffer::clock_apply_mem_process(cint clock){
    //get read result
    pair<string, coo> p;
    while(!this->in_mem_rd_data.empty()){
        p = this->in_mem_rd_data.front();
        //get a
        if(p.first.compare("a") == 0){
            if(this->coo2csr_map.find(make_pair(p.second.r, p.second.c)) == this->coo2csr_map.end())
                assert(false);
            pair<int, int> csr_coor = this->coo2csr_map[make_pair(p.second.r, p.second.c)];
            if(!this->a_valid[csr_coor]){
                this->a_valid[csr_coor] = true;
                this->update_a_mshr(clock, csr_coor);
            }
        }
        this->in_mem_rd_data.pop();
    }
}

void mat_a_buffer::clock_apply_mpe_preprocess(cint clock){
    //get requir from mpe
    for(int i=0;i<this->a_channel_num;i++){
        if(this->in_mult_rd_pkg[i].valid)
            this->a_read_cmd_stagging.push(a_cache_cmd{clock, i, this->in_mult_rd_pkg[i].mat_row, this->in_mult_rd_pkg[i].mat_colmn});
    }
}

void mat_a_buffer::set_mult_input_pkg(cint clock, int cn, mem_pkg pkg){
    this->in_mult_rd_pkg[cn] = pkg;
}

void mat_a_buffer::get_mult_output(cint clock, int cn, coo& data, bool& valid, bool& rd_busy){
    data = this->out_mult_rd_data[cn];
    valid = this->out_mult_rd_valid[cn];
    rd_busy = this->out_mult_rd_busy[cn];
}

void mat_a_buffer::set_mem_rd_input(cint clock, queue<pair<string, coo>> &data){
    clear(this->in_mem_rd_data);
    this->in_mem_rd_data.swap(data);
}

void mat_a_buffer::get_mem_rd_output(cint clock, queue<mem_pkg> &rd_pkg){
    clear(rd_pkg);
    rd_pkg.swap(this->out_mem_rd_pkg);
}

void mat_a_buffer::print_device_status(ofstream &fout){

}

void mat_a_buffer::performance_check_begin(cint clock){
    this->p_check = true;
    this->ch = 0;
    this->fcm = 0;
    this->nfcm = 0;
    cout<<this->device_name<<"performance-check begin at "<<clock<<endl;
}

void mat_a_buffer::performance_check_end(cint clock){
    this->p_check = false;
    cout<<this->device_name<<"performance-check end at "<<clock<<endl;
    cout<<this->device_name<<" performance: "<<endl;
    cout<<"cache hit: "<<this->ch<<endl;
    cout<<"first cache miss: "<<this->fcm<<endl;
    cout<<"non-first cache miss: "<<this->nfcm<<endl;
}

#endif