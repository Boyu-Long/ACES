#ifndef BASE
#define BASE

#include<iostream>
#include<fstream>
#include<utility>
#include<queue>
#include<deque>
#include<map>
#include<set>
#include<memory>
#include<cstdlib>
#include<algorithm>
#include<iomanip>
#include<tuple>
#include<cassert>
#include<time.h>

using namespace std;

const bool cache_debug_info_output = false;
const bool pe_debug_info_output = false;

#define GLOBAL_CACHE_WITH_MSHR

#ifndef GLOBAL_CACHE_WITH_MSHR
#define GLOBAL_CACHE_NO_MSHR
    const int config_cache_mshr = 1, config_cache_mshr_ts = 1;
#else
    // const int config_cache_mshr = 16, config_cache_mshr_ts = 64;
#endif

// //cache param
// const int config_cache_cn = 4, config_cache_d = 2;
// const int config_cache_bank = 16, config_cache_way = 16;
// const int config_cache_cap = 1024, config_cache_cmd_per_clock = 4;
// //c_cache param
// const int config_cache_cn_c = 2, config_cache_d_c = 2;
// const int config_cache_bank_c = 1, config_cache_way_c = 64;
// const int config_cache_cap_c = 128, config_cache_cmd_per_clock_c = 4;
// const int config_cache_mshr_c = 4, config_cache_mshr_ts_c = 16;
// //mem param
// const int config_mem_cn = 16, config_mem_d = 18;
// const string config_mem_name = "mem";

// pair<data, clock>
typedef unsigned long int cint;
typedef pair<double, cint> cdata;
typedef queue<cdata> fifo;

struct coo{
    int r;
    int c;
    double v;

    coo(int r = -1, int c = -1, double v = 0){
        this->r = r;
        this->c = c;
        this->v = v;
    }

    bool operator<(const coo &p) const{
        if(this->r < p.r)
            return true;
        if(this->r == p.r){
            if(this->c < p.c)
                return true;
            if(this->c == p.c)
                return this->v < p.v;
            if(this->c > p.c)
                return false;
        }
        //if(this->r > p.r)
        return false;
    }
    bool operator==(const coo &p) const{
        return this->c == p.c && this->r == p.r && this->v == p.v;
    }
    bool operator>(const coo &p) const{
        if(this->r > p.r)
            return true;
        if(this->r == p.r){
            if(this->c > p.c)
                return true;
            if(this->c == p.c)
                return this->v > p.v;
            if(this->c < p.c)
                return false;
        }
        if(this->r < p.r)
            return false;
    }
};

struct mem_pkg{
    string mat_name;
    int mat_row;
    int mat_colmn;
    int num;
    bool valid;
    cint time_stamp;

    mem_pkg(string mat_name = "", int mat_row = -1, int mat_colmn = -1, 
            int num = -1, bool valid = false, cint time_stamp = 0){
        this->mat_name = mat_name;
        this->mat_row = mat_row;
        this->mat_colmn = mat_colmn;
        this->num = num;
        this->valid = valid;
        this->time_stamp = time_stamp;
    }

    bool operator>(const mem_pkg &pkg) const{
        if(this->time_stamp == pkg.time_stamp){
            if(this->mat_name == pkg.mat_name){
                if(this->mat_row == pkg.mat_row){
                    return this->mat_colmn > pkg.mat_colmn;
                }
                return this->mat_row > pkg.mat_row;
            }
            return this->mat_name > pkg.mat_name;
        }
        return this->time_stamp > pkg.time_stamp;
    }
};

//for pe(add/mult)
enum in_status{
    in_idle,
    in_read,
    in_full
};
enum out_status{
    out_idle,
    out_process,
    out_write,
    out_full
};
enum data_location{
    cc,//c_cache
    memory,
    mult,
    add,
    cache
};

double get_rand_double(int min, int max){
    double m1 = (double)(rand() % 101) / 101;
    min++;
    double m2 = (double)((rand() % (max - min + 1)) + min);
    return m1 + m2;
}

ostream &operator<<(ostream& s, const cdata& p){
    return s<<"[data: "<<p.first<<", clock:"<<p.second<<"]";
}

ostream &operator<<(ostream& s, const coo& p){
        return s<<p.r<<", "<<p.c<<", "<<p.v;
}

bool coo_cmp(const coo& p1, const coo& p2){
    if(p1.r < p2.r)
        return true;
    if(p1.r == p2.r)
        return p1.c <= p2.c;
    //if(p1.r > p2.r)
    return false;
}

bool coo_coor_eq(const coo& p1, const coo& p2){
    return  p1.r == p2.r && p1.c == p2.c;
}

bool coo_coor_ls(const coo& p1, const coo& p2){
    return p1.r < p2.r || (p1.r == p2.r && p1.c < p2.c);
}

bool wr_stg_cmp(const pair<pair<int, int>, coo> &p1, const pair<pair<int, int>, coo> &p2){
    if(p1.first.first < p2.first.first)
        return true;
    if(p1.first.first == p2.first.first)
        return p1.first.second < p2.first.second;
    //if(p1.first.first > p2.first.first)
    return false;
}

namespace std{
template<>
class hash<coo>{
    public:
        size_t operator()(const coo& p)const{
            return hash<int>()(p.c) + hash<int>()(p.r) + hash<double>()(p.v);
        }
};
};

template <typename T>
void clear(queue<T> &q){
    queue<T> empty;
    while(!empty.empty())
        empty.pop();
    q.swap(empty);
}

//aces
//mpe param
const int cfg_spmm_mpe_op_size = 1024;
const int cfg_spmm_ape_op_size = 1024;
//cache param
const int cfg_spmm_cache_cn = 16, cfg_spmm_cache_d = 2;
const int cfg_spmm_spmm_cache_rs = 64, cfg_spmm_cache_pre_row = cfg_spmm_cache_cn*2;
const int cfg_spmm_cache_cap = 1024*1024, cfg_spmm_cache_cmd_per_clock = 16;
const int cfg_spmm_cache_max_pre_a_num = 128;
//mem param
const int cfg_spmm_mem_cn = 16, cfg_spmm_mem_d = 18;
int cfg_spmm_cache_mshr = 16, cfg_spmm_cache_mshr_ts = 64;

int cfg_spmm_condense_type = -1;

struct mult_task{
    int acc_seq_index;
    //<csr row, csr col>
    pair<int, int> coor_a;
    //<csr row, row size>
    pair<int, int> info_b;
    //the first and the last poing in b row (coo format)
    pair<int, int> begin_b_p, end_b_p;

    mult_task(int acc_seq_index = -1, pair<int, int> coor_a = {-1, -1}, pair<int, int> info_b = {-1, -1}, 
              pair<int, int> begin_b_p = {-1, -1}, pair<int, int> end_b_p = {-1, -1}){
        this->acc_seq_index = acc_seq_index;
        this->coor_a = coor_a;
        this->info_b = info_b;
        this->begin_b_p = begin_b_p;
        this->end_b_p = end_b_p;
    }
};

//for humffman tree
struct cache_add_task{
    int csr_row;
    int size;
    //stamp
    int src1, src2, tar;
    //stamp
    int next;
    bool src1_finish, src2_finish;
    cache_add_task(int csr_row = -1, int size = -1, int src1 = -1, 
                   int src2 = -1, int tar = -1){
        this->csr_row = csr_row;
        this->size = size;
        this->src1 = src1;
        this->src2 = src2;
        this->tar = tar;
        this->next = -1;
        this->src1_finish = false;
        this->src2_finish = false;
    }
};

struct cache_add_task_great{
    bool operator()(const cache_add_task &p1, const cache_add_task &p2){
        return p1.size > p2.size;
    }
};

struct pass_info{
    int begin_row, end_row;
    //0 for outer
    //1 for fully-condense
    //2 for mid-condense
    int condense_type;
    cint process_time;
    cint mult_process_time;
    int max_ps_row_size;
    double b_ps_ratio;
    unsigned long long mult_task_num,   pure_add_task_num;
    unsigned long long a_pe_read_cnt,   b_pe_read_cnt,      ps_pe_read_cnt;
    unsigned long long                  b_pe_miss_cnt,      ps_pe_miss_cnt;
    unsigned long long                  b_cache_stall,      ps_cache_stall;
    unsigned long long a_mem_traffic,   b_mem_rd_traffic,   ps_mem_rd_traffic;
    unsigned long long                                      ps_mem_wr_traffic;
    unsigned long long mpe_idle_time,   ape_idle_time;
    unsigned long long ape_to_early,    ape_no_task_cyc,    ape_task_occupy_cyc;
    pass_info(int begin_row = -1, int end_row = -1, int condense_type = -1){
        this->begin_row = begin_row;
        this->end_row = end_row;
        this->condense_type = condense_type;

        this->process_time = 0;
        this->mult_process_time = 0;

        this->max_ps_row_size = 0;
        this->b_ps_ratio = 0;

        this->mult_task_num = 0;
        this->pure_add_task_num = 0;

        this->a_pe_read_cnt = 0;
        this->b_pe_read_cnt = 0;
        this->ps_pe_read_cnt = 0;

        this->b_pe_miss_cnt = 0;
        this->ps_pe_miss_cnt = 0;

        this->b_cache_stall = 0;
        this->ps_cache_stall = 0;

        this->a_mem_traffic = 0;
        this->b_mem_rd_traffic = 0;
        this->ps_mem_rd_traffic = 0;
        this->ps_mem_wr_traffic = 0;

        this->mpe_idle_time = 0;
        this->ape_idle_time = 0;

        this->ape_to_early = 0;
        this->ape_no_task_cyc = 0;
        this->ape_task_occupy_cyc = 0;
    }
    void pass_info_output(ofstream &fout){
        fout<<this->begin_row<<","<<this->end_row<<","<<this->condense_type<<",,,"<<endl;

        fout<<this->max_ps_row_size<<","<<this->b_ps_ratio<<",,,,"<<endl;

        fout<<this->process_time<<","<<this->mult_process_time<<","<<this->mult_task_num<<","<<this->pure_add_task_num<<",,"<<endl;

        fout<<this->a_pe_read_cnt<<","<<this->a_mem_traffic<<",,,,"<<endl;
        fout<<this->b_cache_stall<<","<<this->b_pe_read_cnt<<","<<this->b_pe_miss_cnt<<","<<this->b_mem_rd_traffic<<",,"<<endl;
        fout<<this->ps_cache_stall<<","<<this->ps_pe_read_cnt<<","<<this->ps_pe_miss_cnt<<","<<this->ps_mem_rd_traffic<<","<<this->ps_mem_wr_traffic<<","<<endl;

        fout<<this->mpe_idle_time<<","<<this->ape_idle_time<<",,,,"<<endl;

        fout<<this->ape_to_early<<","<<this->ape_no_task_cyc<<","<<this->ape_task_occupy_cyc<<",,,"<<endl;
    }
};

// sample row is 3*(32 row) = 96 row
struct spmm_task{
    pass_info pass;
    vector<pass_info> test_pass;
    spmm_task(pass_info pass = pass_info(), vector<pass_info> test_pass = {}){
        this->pass = pass;
        this->test_pass.assign(test_pass.begin(), test_pass.end());
    }
};

//the first elem in b row is the largest
vector<vector<coo>> mat_a_read = {}, mat_b_read = {};
map<int, int> coo2csr_a = {}, coo2csr_b = {};

//vector of all the psum for row
vector<int> mat_ps_read_stamp = {};
vector<vector<coo>> mat_ps_read = {};
vector<int> mat_ps_read_size = {};
//<csr row, stamp>
//multiple re-write in mat_b_cache rewrite_stagging
map<pair<int, int>, int> mat_ps_wr_cnt = {};
map<int, int> coo2csr_ps = {};
//<csr row, all row>
map<int, vector<vector<coo>>> mat_ps_all = {};

int max_reuse_dis_b = 0, max_prefetch_dis_b = 0;
int prefetch_dis_factor_b = 1;
int prefetch_dis_factor_ps = 1;
//empty object
coo emp_p = {0, 0, 0};
mem_pkg emp_pkg = {"", 0, 0, 0, false};
vector<coo> emp_coo_v = {};
queue<mem_pkg> emp_pkg_q;
queue<pair<string, coo>> emp_mem_in_q;
queue<coo> emp_coo_q;
queue<bool> emp_bool_q;
queue<pair<int, int>> emp_name_q;

//-------------------task seq-------------------//
vector<pair<int, int>> block_list = {};
vector<spmm_task> task_seq = {};
vector<pair<int, int>> b_acc_seq = {}, a_acc_seq = {};
//------------------mult task-------------------//
int next_mult_index = 0;


//#define MPE_RECORD_SQ
#ifndef MPE_RECORD_SQ
#define MPE_RECORD_BUFFER
#endif

//<csr_row, b_acc_seq index>
vector<queue<pair<int, int>>> mpe_record_buffer = {};
//<csr, queue<b_acc_seq index>>
vector<map<int, queue<int>>> mpe_record_sq = {};
//ape task scheduler
set<int> ps_rd_occupy_row = {}, ps_wr_occupy_row = {};
//-------------------add task-------------------//
int add_comp_task_num = 0, ape_work_num = 0;
//----------------cache add task----------------//
int cache_ape_work_num = 0;
int total_cache_ape_work = 0, finish_cache_ape_work = 0;
queue<cache_add_task> cache_add_task_pool;
//<row, stamp, task>
map<int, map<int, cache_add_task>> cache_add_task_map = {};
//-----------------ps row state-----------------//
//all -> finish add -> finish rewrite -> finish_mem_csr_row -> begin pure cache add -> finish pure rewrite
//row, a_acc_num
map<int, int> all_csr_row = {};
set<int> finish_add_csr_row = {}, finish_rewrite_csr_row = {}, finish_mem_csr_row = {};
set<int> begin_pure_cache_add_csr_row = {};
//<csr_row, stamp>
set<pair<int, int>> finish_pure_rewrite_info = {};
//-----------------performance------------------//
cint task_begin_clock;


int b_cache_ps_prefetch_index;

int mult_percentage_output = 0, add_percentage_output = 0, percentage_step = 25;
int pure_add_finish_num = 0, total_pure_add_num = 0;

string mtx_file_path, mtx_mat_name;
int cmp_mult_index;
#endif
