#ifndef ACES_SIMULATOR
#define ACES_SIMULATOR

#include<base.h>
#include<device_base.h>
#include<clock_manager.h>
#include<mpe.h>
#include<ape.h>
#include<cache_ape.h>
// #include<mat_ps_memory.h>
// #include<mat_a_buffer.h>
#include<mat_b_cache.h>
#include<mat_ab_memory.h>

// mult和add是否用流式处理？在write器件，可以继续read下一个a和b进行计算，同时不要全读完了一次性运行get partial，要来一个算一个，同时在a和b还有s之中加入最大缓存限制
// add结束后可能在rewrite里面有多个回写未完成，不能直接使用finish_rewrite_csr_row
void generate_mat(int row, int column, double sparse, vector<coo>& mat){
    int num = (double)((double)row*(double)column)*sparse;
    mat.clear();

    set<pair<int, int>> tmp;
    int count = 0;
    while(count < num){
        double v = (double)rand()/RAND_MAX*5;
        int rtmp = rand()%row, ctmp = rand()%column;
        if(tmp.find({rtmp, ctmp}) == tmp.end()){
            tmp.insert({rtmp, ctmp});
            mat.push_back(coo{rtmp, ctmp, v});
            count++;
        }
    }
    sort(mat.begin(), mat.end(), coo_cmp);
}

bool tims_stamp_first(const pair<cint, pair<cint, coo>> p1, const pair<cint, pair<cint, coo>> p2){
    return p1.second.first < p2.second.first;
}

void csr_to_meta_convert(const vector<int>& row_offset, vector<pair<int, int>>& meta){
    meta.clear();
    for(int i=0;i<row_offset.size()-1;i++){
        if(row_offset[i+1] - row_offset[i] <= 0)
            continue;
        int c_num = row_offset[i+1] - row_offset[i];
        for(int j=0;j<c_num;j++)
            meta.push_back(make_pair(i, j));
    }
}

class simulator
{
private:
    vector<shared_ptr<device_base>> device_list;
    clock_manager cm;

    vector<mpe> mpe_list;
    vector<ape> ape_list;
    vector<cache_ape> cape_list;
    // mat_a_buffer a_buffer;
    mat_b_cache b_cache;
    mat_ab_memory ab_mem;
    // mat_ps_memory ps_mem;


    queue<pair<string, coo>> mem_cache_rd_data;

    queue<bool> cache_mem_wr_comp;
    queue<pair<int, int>> cache_mem_wr_name;
    queue<coo> cache_mem_wr_value;

    queue<mem_pkg> cache_mem_rd_pkg;
    // queue<mem_pkg> cache_mem_rd_b_pkg[2];
    // queue<mem_pkg> cache_mem_rd_ps_pkg[2];

    queue<bool> mpe_ape_valid[cfg_spmm_cache_cn];
    queue<bool> mpe_ape_end[cfg_spmm_cache_cn];
    queue<coo> mpe_ape_data[cfg_spmm_cache_cn];

    queue<bool> cache_mpe_rd_valid[cfg_spmm_cache_cn];
    queue<coo> cache_mpe_rd_data[cfg_spmm_cache_cn];
    queue<mem_pkg> mpe_cache_rd_pkg[cfg_spmm_cache_cn];

    queue<bool> cache_ape_rd_valid[cfg_spmm_cache_cn];
    queue<coo> cache_ape_rd_data[cfg_spmm_cache_cn];
    queue<mem_pkg> ape_cache_rd_pkg[cfg_spmm_cache_cn];

    queue<bool> ape_cache_wr_valid[cfg_spmm_cache_cn];
    queue<bool> ape_cache_wr_comp[cfg_spmm_cache_cn];
    queue<coo> ape_cache_wr_value[cfg_spmm_cache_cn];


    //mat a demintion
    int matrix_w, matrix_h;
    double matrix_sparse;

    void status_print(ofstream &fout, int spmm_task_index, int pass_index);
    
    //----------------------task get----------------------//
    void get_mat_acc_seq(int begin_row, int end_row, int condense_type, 
                         vector<pair<int, int>> &a_acc_seq, vector<pair<int, int>> &b_acc_seq);
    void parse_mat(double factor, vector<pair<int, int>>& block_list);
    void get_task(vector<pair<int, int>>& block_list, vector<spmm_task> &task_seq);
    //pass_index:   0, 1, 2 for three test_pass
    //              3 for pass
    void set_task(cint clock, int &spmm_task_index, int &pass_index);
    //----------------------mat get-----------------------//
    // void create_ab_mat(int dimention, double sparse);
    // void read_ab_mat(int dimention, double sparse);
    void read_file(string file_name, int &h, int &w, double &s, vector<vector<coo>> &mat);
    void read_mat(string file_path, string mat_name);
    //-----------------cache ape task get-----------------//
    void cache_add_seq_init(int csr_row, vector<int> &all_ps_size, map<int, cache_add_task> &cat);
    void cache_add_task_update(int csr_row);
    //--------------------performance---------------------//
    void get_pass_performance(pass_info &pass, cint task_start_time);

    void init_device();
    void connect_device();
    void process_device();
    void idle();
public:
    void sim();
    simulator();
    ~simulator();
};

//int cn, int delay, int cap, int c_row_s, int max_pre_row, int cmd_per_c, int mem_cn, int mem_d, int mshr_ts, int mshr_ls_ts, string name

simulator::simulator(){
    device_list.clear();
    this->mpe_list.resize(cfg_spmm_cache_cn);
    this->ape_list.resize(cfg_spmm_cache_cn);
    this->cape_list.resize(cfg_spmm_cache_cn);

    //test
    // map<int, cache_add_task> cat = {};
    // vector<int> a = {5,9,6,3,6,3,4,8};
    // cache_add_seq_init(0, a, cat);
}

void simulator::read_file(string file_name, int &h, int &w, double &s, vector<vector<coo>> &mat){
    int n = 0;
    coo p = {0,0,0};

    ifstream fin;
    fin.open(file_name);
    fin>>h>>w>>n;
    s = (double)n/((double)h*(double)w);
    fin>>p.r>>p.c>>p.v;
    mat.clear();
    mat.push_back(vector<coo>{p});
    int r = 0;
    for(int i=1;i<n;i++){
        fin>>p.r>>p.c>>p.v;
        if(mat[r].front().r == p.r){
            mat[r].push_back(p);
        }
        else{
            mat.push_back(vector<coo>{p});
            r += 1;
        }
    }
    fin.close();
}

void simulator::read_mat(string file_path, string mat_name){
    int h, w;
    double s;
    string test_sample_path = file_path + "/" + mat_name + "/";
    string test_sample_file_name = mat_name + "_r.txt";
    read_file(test_sample_path + test_sample_file_name, this->matrix_h, this->matrix_w, this->matrix_sparse, mat_a_read);
    if(this->matrix_h == this->matrix_w)
        mat_b_read.assign(mat_a_read.begin(), mat_a_read.end());
    else{
        test_sample_file_name = mat_name + "_c.txt";
        read_file(test_sample_path + test_sample_file_name, h, w, s, mat_b_read);
    }
}

simulator::~simulator(){
    
}

void simulator::status_print(ofstream &fout, int spmm_task_index, int pass_index){
    // fout<<setfill('-')<<setw(50)<<""<<endl;
    fout<<pass_index<<",,,,,"<<endl;
    if(0 <= pass_index && pass_index < 3){
        //fout<<"sample pass: "<<pass_index<<"."<<endl;
        task_seq[spmm_task_index].test_pass[pass_index].pass_info_output(fout);
    }
    else if(pass_index == 3){
        //fout<<"cal pass"<<endl;
        task_seq[spmm_task_index].pass.pass_info_output(fout);
    }
}

bool len_cmp(const pair<int, int> &a, const pair<int, int> &b){
    return a.second > b.second;
}

bool get_next_block(double row_size, double last_row_size, double factor){
    return (row_size < last_row_size*factor || row_size*factor > last_row_size) && abs((last_row_size-row_size)) > 1;
}

void simulator::parse_mat(double factor, vector<pair<int, int>>& block_list){
    //sort mat A with row length
    vector<pair<int, int>> row_len = {};
    for(int i=0;i<mat_a_read.size();i++)
        row_len.push_back({i, mat_a_read[i].size()});
    sort(row_len.begin(), row_len.end(), len_cmp);

    //a
    vector<vector<coo>> tmp_a(mat_a_read);
    mat_a_read.clear();
    for(pair<int, int> p : row_len)
        mat_a_read.push_back(tmp_a[p.first]);
    
    for(int i=0;i<mat_a_read.size();i++)
        coo2csr_a.insert({mat_a_read[i].front().r, i});
    //b
    for(int i=0;i<mat_b_read.size();i++){
        if(max_prefetch_dis_b < mat_b_read[i].size()/prefetch_dis_factor_b)
            max_prefetch_dis_b = mat_b_read[i].size()/prefetch_dis_factor_b;
        coo2csr_b.insert({mat_b_read[i].front().r, i});
    }
    max_reuse_dis_b = __INT_MAX__ - max_prefetch_dis_b;
    //ps
    for(pair<int, int> p : coo2csr_a)
        coo2csr_ps.emplace(p);
    mat_ps_read.resize(mat_a_read.size());
    for(int i=0;i<mat_ps_read.size();i++){
        mat_ps_read[i].clear();
    }
    mat_ps_read_size.assign(mat_ps_read.size(), 0);
    mat_ps_read_stamp.assign(mat_ps_read.size(), 0);
    for(int i=0;i<mat_ps_read.size();i++)
        mat_ps_wr_cnt.insert({{i, 0}, 0});
    

    //get block_list with similar row length
    block_list.clear();
    int start_row = 0;
    double last_row_len = mat_a_read[0].size();
    for(int i=1;i<mat_a_read.size();i++){
        if(get_next_block(mat_a_read[i].size(), last_row_len, factor)){
            block_list.push_back({start_row, i-1});
            start_row = i;
        }
        else if(i == mat_a_read.size()-1){
            block_list.push_back({start_row, i});
            break;
        }
        last_row_len = mat_a_read[i].size();
    }
}

void simulator::get_task(vector<pair<int, int>>& block_list, vector<spmm_task> &task_seq){
    task_seq.clear();
    int test_step = cfg_spmm_cache_cn*2;
    for(pair<int, int> block : block_list){
        int br = block.first, er = block.second;
        if(block.second - block.first + 1 > 3*cfg_spmm_cache_cn){
            //get 3 test pass
            vector<pass_info> test_pass = {};
            for(int i=0;i<3;i++){
                int condense_type = (cfg_spmm_condense_type != -1) ? cfg_spmm_condense_type : 2-i;
                test_pass.push_back(pass_info(br+test_step*i, br+test_step*(i+1)-1, condense_type));
            }
            //get pass (condense type will be decided later)
            pass_info pass(br+test_step*3, er, -1);
            task_seq.push_back(spmm_task(pass, test_pass));
        }
        else{
            int condense_type = (cfg_spmm_condense_type != -1) ? cfg_spmm_condense_type : 2;
            task_seq.push_back(spmm_task(pass_info(br, er, condense_type)));
        }
    }
}

void simulator::init_device(){
    cint clock = this->cm.get_clock();
    //mpe list, ape list and cape
    for(int i=0;i<cfg_spmm_cache_cn;i++){
        // this->mpe_list[i].set_cache_a_input(emp_p, false, true);
        // this->mpe_list[i].set_cache_b_input(emp_coo_q, emp_bool_q, false);
        this->mpe_list[i].set_adder_input(-1, false);
        // this->ape_list[i].set_cache_rd_input(emp_bool_q, true, emp_coo_q);
        this->ape_list[i].set_cache_wr_input(true);
        // this->ape_list[i].set_mult_input(emp_coo_q, emp_bool_q, emp_bool_q);
        // this->cape_list[i].set_mem_rd_input(false, true, {-1, -1}, emp_p);
        // this->cape_list[i].set_mem_wr_input(true);
    }
    //a buffer
    // this->a_buffer.set_mem_rd_input(clock, emp_mem_in_q);
    // for(int i=0;i<cfg_spmm_cache_cn;i++)
    //     this->a_buffer.set_mult_input_pkg(clock, i, emp_pkg);
    
    //b cache
    // this->b_cache.set_mem_rd_input(clock, emp_mem_in_q);
    this->b_cache.set_mem_wr_input(clock, true);
    // for(int i=0;i<cfg_spmm_cache_cn;i++){
    //     this->b_cache.set_mult_input_pkg(clock, i, emp_pkg_q);
    //     this->b_cache.set_add_rd_input(clock, i, emp_pkg_q);
    //     this->b_cache.set_add_wr_input(clock, i, emp_bool_q, emp_bool_q, emp_coo_q);
    // }
    //ab mem
    // this->ab_mem.set_wr_input(clock, emp_bool_q, emp_name_q, emp_coo_q);
    // this->ab_mem.set_rd_input(clock, emp_pkg_q);
    //ps mem
    // for(int i=0;i<cfg_spmm_cache_cn;i++){
    //     this->ps_mem.set_wr_input(clock, i, emp_bool_q, emp_bool_q, emp_coo_q, emp_name_q);
    //     this->ps_mem.set_rd_input(clock, i, emp_pkg);
    // }
}

void simulator::connect_device(){
    cint clock = this->cm.get_clock();
    //connection between mem, a-buffer and b-cache
    {
    queue<mem_pkg> rd_pkg_q_a, rd_pkg_q_b;
    queue<pair<string, coo>> data_a, data_b, data;
    queue<pair<int, int>> name;
    queue<bool> comp;
    queue<coo> value;
    bool wr_busy;

    // this->a_buffer.get_mem_rd_output(clock, rd_pkg_q_a);
    // this->b_cache.get_mem_rd_output(clock, rd_pkg_q_b);
    // while(!rd_pkg_q_a.empty()){
    //     rd_pkg_q_b.push(rd_pkg_q_a.front());
    //     rd_pkg_q_a.pop();
    // }
    // this->ab_mem.set_rd_input(clock, rd_pkg_q_b);

    // swap(this->ab_mem.in_rd_pkg, this->b_cache.out_mem_rd_pkg);
    // swap(this->ab_mem.in_rd_pkg_b, this->b_cache.out_mem_rd_pkg_b);
    // swap(this->ab_mem.in_rd_pkg_ps, this->b_cache.out_mem_rd_pkg_ps);

    // this->ab_mem.get_rd_output(clock, data);
    // while(!data.empty()){
    //     if(data.front().first.compare("a") == 0)
    //         data_a.push(data.front());
    //     else
    //         data_b.push(data.front());
    //     data.pop();
    // }
    // //this->a_buffer.set_mem_rd_input(clock, data_a);
    // this->b_cache.set_mem_rd_input(clock, data_b);
    
    // this->b_cache.get_mem_wr_output(clock, comp, name, value);
    // this->ab_mem.set_wr_input(clock, comp, name, value);
    
    this->ab_mem.get_wr_output(clock, wr_busy);
    this->b_cache.set_mem_wr_input(clock, wr_busy);
    }

    //connection between a-buffer, b-cache, mpe and ape
    {
    int send_csr_row;
    coo data;
    bool valid, busy, send_data;
    mem_pkg rd_pkg;

    // queue<mem_pkg> rd_pkg_q;
    // queue<coo> data_q;
    // queue<bool> valid_q, comp_q;
    for(int i=0;i<cfg_spmm_cache_cn;i++){
        //---------------mpe & a buffer---------------//
        // this->mpe_list[i].get_cache_a_output(rd_pkg);
        // this->a_buffer.set_mult_input_pkg(clock, i, rd_pkg);

        // this->a_buffer.get_mult_output(clock, i, data, valid, busy);
        // this->mpe_list[i].set_cache_a_input(data, valid, busy);
        //---------------mpe & b cache----------------//
        // swap(this->mpe_list[i].out_cache_rd_b_pkg, this->b_cache.in_mult_rd_pkg[i]);
        // this->mpe_list[i].get_cache_b_output(rd_pkg_q);
        // this->b_cache.set_mult_input_pkg(clock, i, rd_pkg_q);

        // swap(this->mpe_list[i].in_cache_rd_b_valid, this->b_cache.out_mult_rd_valid[i]);
        // swap(this->mpe_list[i].in_cache_rd_b_data, this->b_cache.out_mult_rd_data[i]);
        this->b_cache.get_mult_output(clock, i, busy);
        this->mpe_list[i].set_cache_b_input(busy);
        //-------------ape & b cache (rd)-------------//
        // swap(this->ape_list[i].out_cache_rd_ps_pkg, this->b_cache.in_add_rd_pkg[i]);
        // this->ape_list[i].get_cache_rd_output(rd_pkg_q);
        // this->b_cache.set_add_rd_input(clock, i, rd_pkg_q);

        // swap(this->ape_list[i].in_cache_rd_ps_valid, this->b_cache.out_add_rd_valid[i]);
        // swap(this->ape_list[i].in_cache_rd_ps_data, this->b_cache.out_add_rd_data[i]);
        this->b_cache.get_add_rd_output(clock, i, busy);
        this->ape_list[i].set_cache_rd_input(busy);
        //-------------ape & b cache (wr)-------------//
        // swap(this->b_cache.in_add_wr_valid[i], this->ape_list[i].out_cache_wr_ps_valid);
        // swap(this->b_cache.in_add_wr_comp[i], this->ape_list[i].out_cache_wr_ps_comp);
        // swap(this->b_cache.in_add_wr_value[i], this->ape_list[i].out_cache_wr_ps_value);
        // this->ape_list[i].get_cache_wr_output(valid_q, comp_q, data_q);
        // this->b_cache.set_add_wr_input(clock, i, valid_q, comp_q, data_q);

        this->b_cache.get_add_wr_output(clock, i, busy);
        this->ape_list[i].set_cache_wr_input(busy);
        //-----------------ape & mpe------------------//
        this->ape_list[i].get_mult_output(send_csr_row, send_data);
        this->mpe_list[i].set_adder_input(send_csr_row, send_data);
        
        // swap(this->mpe_list[i].out_add_data, this->ape_list[i].in_mult_data);
        // swap(this->mpe_list[i].out_add_valid, this->ape_list[i].in_mult_valid);
        // swap(this->mpe_list[i].out_add_end, this->ape_list[i].in_mult_end);
        // this->mpe_list[i].get_adder_output(data_q, comp_q, valid_q);
        // this->ape_list[i].set_mult_input(data_q, comp_q, valid_q);
    }
    }
    //connection between cape and ps mem
    // {
    // coo data;
    // pair<int, int> name;
    // bool valid, busy, comp, send_data;
    // mem_pkg rd_pkg;

    // queue<coo> data_q;
    // queue<bool> valid_q, comp_q;
    // queue<pair<int, int>> name_q;
    // for(int i=0;i<cfg_spmm_cache_cn;i++){
    //     //--------------cape & mem (rd)---------------//
    //     this->cape_list[i].get_mem_rd_output(rd_pkg);
    //     this->ps_mem.set_rd_input(clock, i, rd_pkg);

    //     this->ps_mem.get_rd_output(clock, i, valid, busy, name, data);
    //     this->cape_list[i].set_mem_rd_input(valid, busy, name, data);
    //     //--------------cape & mem (wr)---------------//
    //     this->cape_list[i].get_mem_wr_output(valid_q, comp_q, data_q, name_q);
    //     this->ps_mem.set_wr_input(clock, i, valid_q, comp_q, data_q, name_q);

    //     this->ps_mem.get_wr_output(clock, i, busy);
    //     this->cape_list[i].set_mem_wr_input(busy);
    // }
    // }
}

void simulator::process_device(){
    cint clock = this->cm.get_clock();
    this->ab_mem.clock_apply(clock);
    // this->ps_mem.clock_apply(clock);
    // this->a_buffer.clock_apply(clock);
    this->b_cache.clock_apply(clock);
    for(int i=0;i<cfg_spmm_cache_cn;i++){
        this->mpe_list[i].clock_apply(clock);
        this->ape_list[i].clock_apply(clock);
        this->cape_list[i].clock_apply(clock);
    }
    
    this->ab_mem.clock_update(clock);
    // this->ps_mem.clock_update(clock);
    // this->a_buffer.clock_update(clock);
    this->b_cache.clock_update(clock);
    for(int i=0;i<cfg_spmm_cache_cn;i++){
        this->mpe_list[i].clock_update(clock);
        this->ape_list[i].clock_update(clock);
        this->cape_list[i].clock_update(clock);
    }
}

void simulator::idle(){
    for(int i=0;i<1000;i++)
        this->process_device();
}

bool col_first_cmp(const coo& p1, const coo& p2){
    if(p1.c < p2.c)
        return true;
    if(p1.c == p2.c)
        return p1.r <= p2.r;
    return false;
}

void csc_mat_get_acc_seq(vector<int> &csr_col_record, vector<coo>& csc_mat, vector<pair<int, int>> &a_acc_seq, vector<pair<int, int>> &b_acc_seq){
    coo p;
    for(int i=0;i<csc_mat.size();i++){
        p = csc_mat[i];
        //has corresponding b row
        if(coo2csr_b.find(p.c) != coo2csr_b.end()){
            assert(coo_coor_eq(mat_a_read[coo2csr_a[p.r]][csr_col_record[coo2csr_a[p.r]]], p));
            a_acc_seq.push_back({coo2csr_a[p.r], csr_col_record[coo2csr_a[p.r]]});
            b_acc_seq.push_back({coo2csr_b[p.c], mat_b_read[coo2csr_b[p.c]].size()});
        }
        csr_col_record[coo2csr_a[p.r]] += 1;
    }
}

void csr_mat_get_acc_seq(int begin_row, int end_row, vector<int> &col_index_offset, vector<vector<coo>> &mat_a, vector<pair<int, int>> &a_acc_seq, vector<pair<int, int>> &b_acc_seq){
    int mat_a_max_row = 0;
    for(const vector<coo>& r : mat_a){
        if(mat_a_max_row < r.size())
            mat_a_max_row = r.size();
    }
    for(int col_index = 0;col_index<mat_a_max_row;col_index++){
        for(int i=begin_row;i<=end_row;i++){
            vector<coo> &a_row = mat_a[i];
            if(a_row.size() > col_index){
                coo p_a = a_row[col_index];
                if(coo2csr_b.find(p_a.c) != coo2csr_b.end()){
                    a_acc_seq.push_back(make_pair(coo2csr_a[p_a.r], col_index+col_index_offset[coo2csr_a[p_a.r]]));
                    b_acc_seq.push_back(make_pair(coo2csr_b[p_a.c], mat_b_read[coo2csr_b[p_a.c]].size()));
                }
            }
        }
    }
}

void simulator::get_mat_acc_seq(int begin_row, int end_row, int condense_type, 
                                vector<pair<int, int>> &a_acc_seq, vector<pair<int, int>> &b_acc_seq){
    a_acc_seq.clear();b_acc_seq.clear();
    int mat_a_max_row = 0;
    for(const vector<coo>& r : mat_a_read){
        if(mat_a_max_row < r.size())
            mat_a_max_row = r.size();
    }

    int step = 0;
    //0 for outer
    if(condense_type == 0){
        step = 1;
        //get csc format
        vector<coo> mat_a_col_first = {};
        for(int i=begin_row;i<=end_row;i++){
            mat_a_col_first.insert(mat_a_col_first.end(), mat_a_read[i].begin(), mat_a_read[i].end());
        }
        sort(mat_a_col_first.begin(), mat_a_col_first.end(), col_first_cmp);
        //get acc seq
        vector<int> csr_col_record(mat_a_read.size(), 0);
        csc_mat_get_acc_seq(csr_col_record, mat_a_col_first, a_acc_seq, b_acc_seq);
    }
    //1 for fully-condense
    else if(condense_type == 1){
        vector<int> col_index_offset(mat_a_read.size(), 0);
        csr_mat_get_acc_seq(begin_row, end_row, col_index_offset, mat_a_read, a_acc_seq, b_acc_seq);
    }
    //2 for mid-condense
    else if(condense_type == 2){
        //ceil to ensure only two partitions
        step = (this->matrix_w+2-1)/2;
        vector<vector<coo>> mat_a_1 = {}, mat_a_2 = {};
        vector<int> col_index_offset_1(mat_a_read.size(), 0), col_index_offset_2(mat_a_read.size(), 0);
        mat_a_1.resize(mat_a_read.size());mat_a_2.resize(mat_a_read.size());
        for(int i=begin_row;i<=end_row;i++){
            for(coo p : mat_a_read[i]){
                if(p.c <= step)
                    mat_a_1[coo2csr_a[p.r]].push_back(p);
                else
                    mat_a_2[coo2csr_a[p.r]].push_back(p);
            }
        }
        for(int i=begin_row;i<=end_row;i++)
            col_index_offset_2[i] += mat_a_1[i].size();
        csr_mat_get_acc_seq(begin_row, end_row, col_index_offset_1, mat_a_1, a_acc_seq, b_acc_seq);
        csr_mat_get_acc_seq(begin_row, end_row, col_index_offset_2, mat_a_2, a_acc_seq, b_acc_seq);
    }
    else
        assert(false);
}

void simulator::cache_add_seq_init(int csr_row, vector<int> &all_ps_size, map<int, cache_add_task> &cat){
    cat.clear();
    //init small head
    priority_queue<cache_add_task, vector<cache_add_task>, cache_add_task_great> small_head;
    for(int i=0;i<all_ps_size.size();i++)
        small_head.push(cache_add_task(csr_row, all_ps_size[i], -1, -1, i));
    
    if(small_head.size() == 1)
        return;
    
    
    int next_stamp = all_ps_size.size();
    while(small_head.size() > 1){
        cache_add_task next_task(csr_row, 0, -1, -1, -1);
        cache_add_task now;
        //set tar
        next_task.tar = next_stamp;
        //connect src1 and next
        now = small_head.top();
        small_head.pop();
        next_task.src1 = now.tar;
        now.next = next_task.tar;
        next_task.size += now.size;
        if(now.src1 != -1 && now.src2 != -1)
            cat.insert({now.tar, now});
        if(now.src1 == -1 && now.src2 == -1)
            next_task.src1_finish = true;
        //connect src2 and next
        now = small_head.top();
        small_head.pop();
        next_task.src2 = now.tar;
        now.next = next_task.tar;
        next_task.size += now.size;
        if(now.src1 != -1 && now.src2 != -1)
            cat.insert({now.tar, now});
        if(now.src1 == -1 && now.src2 == -1)
            next_task.src2_finish = true;
        //set new task in small head
        small_head.push(next_task);
        next_stamp += 1;
    }
    cat.insert({small_head.top().tar, small_head.top()});
    finish_pure_rewrite_info.emplace(make_pair(small_head.top().csr_row, small_head.top().tar));
    total_cache_ape_work += cat.size();
    if(csr_row != 3)
        return;
    for(pair<int, cache_add_task> task : cat){
        if(task.second.next == 106){

        }
    }
}

void simulator::cache_add_task_update(int csr_row){
    //init all add task
    vector<int> all_ps_size = {};
    for(vector<coo> &r : mat_ps_all[csr_row])
        all_ps_size.push_back(r.size());
    cache_add_task_map[csr_row].clear();
    cache_add_seq_init(csr_row, all_ps_size, cache_add_task_map[csr_row]);
    //set task which has no dependence into task pool
    for(auto &task : cache_add_task_map[csr_row]){
        if(task.second.src1_finish && task.second.src2_finish)
            cache_add_task_pool.push(task.second);
    }
}

void simulator::set_task(cint clock, int &spmm_task_index, int &pass_index){

    this->ab_mem.reset_task();
    // this->ps_mem.reset_task();
    // this->a_buffer.reset_task();
    this->b_cache.reset_task();
    for(int i=0;i<cfg_spmm_cache_cn;i++){
        this->mpe_list[i].reset_task();
        this->ape_list[i].reset_task();
        this->cape_list[i].reset_task();
    }

    //-------------------task seq-------------------//
    //vector<pair<int, int>> block_list = {};
    b_acc_seq.clear();
    a_acc_seq.clear();
    //------------------mult task-------------------//
    next_mult_index = 0;
    for(int i=0;i<mpe_record_sq.size();i++)
        mpe_record_sq[i].clear();
    for(int i=0;i<mpe_record_buffer.size();i++)
        clear(mpe_record_buffer[i]);
    for(int i=0;i<mpe_record_buffer.size();i++)
        clear(mpe_record_buffer[i]);
    ps_rd_occupy_row.clear();
    ps_wr_occupy_row.clear();
    //-------------------add task-------------------//
    add_comp_task_num = 0, ape_work_num = 0;
    //----------------cache add task----------------//
    cache_ape_work_num = 0;
    total_cache_ape_work = 0, finish_cache_ape_work = 0;
    clear(cache_add_task_pool);
    cache_add_task_map.clear();
    //-----------------ps row state-----------------//
    all_csr_row.clear();
    finish_add_csr_row.clear();
    finish_rewrite_csr_row.clear();
    finish_mem_csr_row.clear();
    begin_pure_cache_add_csr_row.clear();
    finish_pure_rewrite_info.clear();
    //-----------------performance------------------//
    task_begin_clock = this->cm.get_clock();

    b_cache_ps_prefetch_index = 0;

    //some how get the a and b acc seq
    if(spmm_task_index >= task_seq.size())
        assert(false);
    
    spmm_task &task = task_seq[spmm_task_index];
    if(pass_index == -1)
        pass_index = (task.test_pass.empty()) ? 3 : 0;
    //in test pass
    if(0 <= pass_index && pass_index < 3){
        pass_info &pass = task.test_pass[pass_index];
        this->get_mat_acc_seq(pass.begin_row, pass.end_row, pass.condense_type, a_acc_seq, b_acc_seq);
    }
    //in real pass
    else if(pass_index == 3){
        pass_info &pass = task.pass;
        //get the min clock
        if(!task.test_pass.empty()){
            int min_clock_index = -1;
            unsigned long long min_clock = __LONG_LONG_MAX__;
            for(int i=0;i<3;i++){
                if(task.test_pass[i].process_time < min_clock){
                    min_clock = task.test_pass[i].process_time;
                    min_clock_index = i;
                }
            }
            pass.condense_type = task.test_pass[min_clock_index].condense_type;
        }
        else
            pass.condense_type = 2;
        this->get_mat_acc_seq(pass.begin_row, pass.end_row, pass.condense_type, a_acc_seq, b_acc_seq);
    }
    else
        assert(false);
    
    cout<<setfill('=')<<setw(50)<<""<<endl;
    if(0 <= pass_index && pass_index < 3)
        cout<<mtx_mat_name<<" | sample pass "<<pass_index<<" start, condense type: "<<task.test_pass[pass_index].condense_type<<endl;
    else if(pass_index == 3)
        cout<<mtx_mat_name<<" | pass start, condense type: "<<task.pass.condense_type<<endl;
    else
        assert(false);

    //get all csr
    for(int i=0;i<a_acc_seq.size();i++){
        pair<int, int> info = a_acc_seq[i];
        if(all_csr_row.find(info.first) == all_csr_row.end())
            all_csr_row.emplace(make_pair(info.first, 1));
        else
            all_csr_row[info.first] += 1;
    }

    //get last size and ratio
    int b_row_ave_len = this->matrix_sparse*this->matrix_h;
    double row_num = cfg_spmm_cache_pre_row+cfg_spmm_cache_cn;
    double b_cache_size = row_num*b_row_ave_len*30, ps_cache_size = cfg_spmm_cache_cap-b_cache_size;
    //assert(ps_cache_size >= 1);
    double b_ps_ratio = b_cache_size/ps_cache_size;
    if(b_ps_ratio > 0.5 || b_ps_ratio <= 0){
        cout<<mtx_mat_name<<" | Too large b cache"<<endl;
        b_ps_ratio = 0.5;
        b_cache_size = cfg_spmm_cache_cap*b_ps_ratio;
        ps_cache_size = cfg_spmm_cache_cap-b_cache_size;
        cout<<mtx_mat_name<<" | Too large b cache: "<<b_cache_size<<" "<<ps_cache_size<<endl;
        cout<<cfg_spmm_cache_cap-b_cache_size<<" "<<row_num*b_row_ave_len*30<<endl;
        cout<<this->matrix_sparse<<" "<<this->matrix_h<<endl;
    }
    int ps_max_size = min((cfg_spmm_cache_cap-b_cache_size)/64, row_num*b_row_ave_len*30);
    this->b_cache.set_ps_max_size(ps_max_size);
    this->b_cache.set_b_ps_cache_ratio(b_ps_ratio);

    if(0 <= pass_index && pass_index < 3){
        task_seq[spmm_task_index].test_pass[pass_index].max_ps_row_size = ps_max_size;
        task_seq[spmm_task_index].test_pass[pass_index].b_ps_ratio = b_ps_ratio;
    }
    else{
        task_seq[spmm_task_index].pass.max_ps_row_size = ps_max_size;
        task_seq[spmm_task_index].pass.b_ps_ratio = b_ps_ratio;
    }

    // this->a_buffer.add_a_acc_seq(a_acc_seq);
    this->b_cache.add_b_acc_seq(b_acc_seq);
    this->b_cache.add_ps_acc_seq(a_acc_seq);
    this->b_cache.begin_task(clock);
}

void simulator::get_pass_performance(pass_info &pass, cint task_start_time){
    pass.process_time = this->cm.get_clock() - task_start_time;
    pass.mult_task_num = a_acc_seq.size();
    for(auto &row_task : cache_add_task_map){
        pass.pure_add_task_num += row_task.second.size();
    }

    pass.a_pe_read_cnt = a_acc_seq.size();
    pass.b_pe_read_cnt = this->b_cache.b_ch + this->b_cache.b_fcm + this->b_cache.b_nfcm;
    pass.ps_pe_read_cnt = this->b_cache.ps_ch + this->b_cache.ps_fcm + this->b_cache.ps_nfcm;

    pass.b_pe_miss_cnt = this->b_cache.b_fcm + this->b_cache.b_nfcm;
    pass.ps_pe_miss_cnt = this->b_cache.ps_fcm + this->b_cache.ps_nfcm;

    pass.b_cache_stall = this->b_cache.b_cache_stall;
    pass.ps_cache_stall = this->b_cache.ps_cache_stall;

    pass.a_mem_traffic = a_acc_seq.size();
    pass.b_mem_rd_traffic = this->b_cache.b_mem_rd_traffic;
    // pass.ps_mem_rd_traffic = this->b_cache.ps_mem_rd_traffic + this->ps_mem.ps_mem_rd_traffic;
    // pass.ps_mem_wr_traffic = this->b_cache.ps_mem_wr_traffic + this->ps_mem.ps_mem_wr_traffic;

    for(mpe &m : this->mpe_list)
        pass.mpe_idle_time += m.idle_cnt;
    for(ape &a : this->ape_list){
        pass.ape_idle_time += a.idle_cnt;
        pass.ape_to_early += a.to_early_cnt;
    }
    for(cache_ape &c : this->cape_list)
        pass.ape_idle_time += c.idle_cnt;
}

void simulator::sim(){
    clock_t clock_begin, clock_end;
    cout<<mtx_mat_name<<" | sim begin, mat: "<<mtx_mat_name<<endl;
    clock_begin = clock();
    srand(0);
    // create_ab_mat();
    //read_ab_mat(5, 0.5);
    // read_ab_mat(1E4, 1E-4);
    this->read_mat(mtx_file_path, mtx_mat_name);
    // this->mem.set_mat_ab("a", mat_a_read);
    // this->mem.set_mat_ab("a", mat_b_read);

    mpe_record_sq.resize(cfg_spmm_cache_cn);
    mpe_record_buffer.resize(cfg_spmm_cache_cn);
    for(int i=0;i<cfg_spmm_cache_cn;i++){
        this->mpe_list[i].set_index(i);
        this->ape_list[i].set_index(i);
        this->cape_list[i].set_index(i);
    }
    this->init_device();

    int spmm_task_index = 0, pass_index = -1;
    this->parse_mat(0.5, block_list);
    this->get_task(block_list, task_seq);
    this->set_task(0, spmm_task_index, pass_index);

    clear(this->mem_cache_rd_data);
    clear(this->cache_mem_wr_comp);
    clear(this->cache_mem_wr_name);
    clear(this->cache_mem_wr_value);
    clear(this->cache_mem_rd_pkg);
    // clear(this->cache_mem_rd_b_pkg[0]);
    // clear(this->cache_mem_rd_b_pkg[1]);
    // clear(this->cache_mem_rd_ps_pkg[0]);
    // clear(this->cache_mem_rd_ps_pkg[1]);
    for(int i=0;i<cfg_spmm_cache_cn;i++){
        clear(this->mpe_ape_valid[i]);
        clear(this->mpe_ape_end[i]);
        clear(this->mpe_ape_data[i]);

        clear(this->cache_mpe_rd_valid[i]);
        clear(this->cache_mpe_rd_data[i]);
        clear(this->mpe_cache_rd_pkg[i]);

        clear(this->cache_ape_rd_valid[i]);
        clear(this->cache_ape_rd_data[i]);
        clear(this->ape_cache_rd_pkg[i]);

        clear(this->ape_cache_wr_valid[i]);
        clear(this->ape_cache_wr_comp[i]);
        clear(this->ape_cache_wr_value[i]);
    }

    this->ab_mem.in_rd_pkg = &this->cache_mem_rd_pkg;
    this->b_cache.out_mem_rd_pkg = &this->cache_mem_rd_pkg;

    this->ab_mem.in_wr_comp = &this->cache_mem_wr_comp;
    this->b_cache.out_mem_wr_comp = &this->cache_mem_wr_comp;

    this->ab_mem.in_wr_name = &this->cache_mem_wr_name;
    this->b_cache.out_mem_wr_name = &this->cache_mem_wr_name;

    this->ab_mem.in_wr_value = &this->cache_mem_wr_value;
    this->b_cache.out_mem_wr_value = &this->cache_mem_wr_value;

    this->ab_mem.out_data = &this->mem_cache_rd_data;
    this->b_cache.in_mem_rd_data = &this->mem_cache_rd_data;

    // this->ab_mem.in_rd_pkg_b = &this->cache_mem_rd_b_pkg[0];
    // this->b_cache.out_mem_rd_pkg_b = &this->cache_mem_rd_b_pkg[1];
    // this->ab_mem.in_rd_pkg_ps = &this->cache_mem_rd_ps_pkg[0];
    // this->b_cache.out_mem_rd_pkg_ps = &this->cache_mem_rd_ps_pkg[1];
    for(int i=0;i<cfg_spmm_cache_cn;i++){
        this->mpe_list[i].out_add_valid = &mpe_ape_valid[i];
        this->ape_list[i].in_mult_valid = &mpe_ape_valid[i];
        this->mpe_list[i].out_add_end = &mpe_ape_end[i];
        this->ape_list[i].in_mult_end = &mpe_ape_end[i];
        this->mpe_list[i].out_add_data = &mpe_ape_data[i];
        this->ape_list[i].in_mult_data = &mpe_ape_data[i];

        this->b_cache.out_mult_rd_valid[i] = &cache_mpe_rd_valid[i];
        this->mpe_list[i].in_cache_rd_b_valid = &cache_mpe_rd_valid[i];
        this->b_cache.out_mult_rd_data[i] = &cache_mpe_rd_data[i];
        this->mpe_list[i].in_cache_rd_b_data = &cache_mpe_rd_data[i];
        this->b_cache.in_mult_rd_pkg[i] = &mpe_cache_rd_pkg[i];
        this->mpe_list[i].out_cache_rd_b_pkg = &mpe_cache_rd_pkg[i];

        this->b_cache.out_add_rd_valid[i] = &cache_ape_rd_valid[i];
        this->ape_list[i].in_cache_rd_ps_valid = &cache_ape_rd_valid[i];
        this->b_cache.out_add_rd_data[i] = &cache_ape_rd_data[i];
        this->ape_list[i].in_cache_rd_ps_data = &cache_ape_rd_data[i];
        this->b_cache.in_add_rd_pkg[i] = &ape_cache_rd_pkg[i];
        this->ape_list[i].out_cache_rd_ps_pkg = &ape_cache_rd_pkg[i];

        this->b_cache.in_add_wr_valid[i] = &ape_cache_wr_valid[i];
        this->ape_list[i].out_cache_wr_ps_valid = &ape_cache_wr_valid[i];
        this->b_cache.in_add_wr_comp[i] = &ape_cache_wr_comp[i];
        this->ape_list[i].out_cache_wr_ps_comp = &ape_cache_wr_comp[i];
        this->b_cache.in_add_wr_value[i] = &ape_cache_wr_value[i];
        this->ape_list[i].out_cache_wr_ps_value = &ape_cache_wr_value[i];
    }

    ofstream fout(mtx_file_path + "/" + mtx_mat_name + "_ACES_performance.txt");
    unsigned long long mpe_time = 0, ape_time = 0, apply_time = 0, update_time = 0, connect_time = 0, other_time = 0;
    clock_t mpet[2], apet[2], applyt[2], updatet[2], connectt[2], othert[2];
    unsigned long long pea_time = 0, apea_time = 0, abmema_time = 0, bcachea_time = 0, psmema_time = 0, cape_time = 0;
    clock_t peat[2], apeat[2], abmemat[2], bcacheat[2], psmemat[2], capet[2];
    while(true){
        mpet[0] = clock();
        int percentage = (int)(100*next_mult_index/a_acc_seq.size());
        if(percentage >= mult_percentage_output){
            cout<<mtx_mat_name<<" | mult task complete: "<<percentage<<"%."<<endl;
            mult_percentage_output += percentage_step;
            if(percentage == 100)
                cout<<mtx_mat_name<<" | mult task cyc: "<<this->cm.get_clock()<<endl;;
        }

        if(total_pure_add_num == 0 && begin_pure_cache_add_csr_row.size() == all_csr_row.size()){
            for(auto &row_task : cache_add_task_map){
                total_pure_add_num += row_task.second.size();
            }
            cout<<mtx_mat_name<<" | *pure add task num: "<<total_pure_add_num<<endl;
        }

        percentage =(total_pure_add_num != 0) ? (int)(100*pure_add_finish_num/total_pure_add_num) : 0;
        if(percentage >= add_percentage_output && (total_pure_add_num != 0)){
            cout<<mtx_mat_name<<" | pure add task complete: "<<percentage<<"%."<<endl;
            add_percentage_output += percentage_step;
            if(percentage == 100)
                cout<<mtx_mat_name<<" | pure add task cyc: "<<this->cm.get_clock()<<endl;;
        }

        if(next_mult_index == a_acc_seq.size()){
            if(0 <= pass_index && pass_index <=2)
                task_seq[spmm_task_index].test_pass[pass_index].mult_process_time = this->cm.get_clock();
            else
                task_seq[spmm_task_index].pass.mult_process_time = this->cm.get_clock();
        }

        //mpe
        for(int i=0;i<cfg_spmm_cache_cn;i++){
            if(this->mpe_list[i].mult_task_end){
                if(next_mult_index < a_acc_seq.size()){
                    this->mpe_list[i].set_task(mult_task{next_mult_index, a_acc_seq[next_mult_index], b_acc_seq[next_mult_index], {-1, -1}, {-1, -1}});
                    this->b_cache.set_mult_task_end(i, next_mult_index);
                    next_mult_index += 1;
                }
                else
                    this->b_cache.set_mult_task_end(i, -1);
            }
        }
        mpet[1] = clock();
        mpe_time += mpet[1]-mpet[0];

        apet[0] = clock();
        //ape
        ape_work_num = 0;
        for(int i=0;i<cfg_spmm_cache_cn;i++){
            if(this->ape_list[i].add_task_end){
                int next_add_task_index = this->ape_list[i].get_new_task(this->cm.get_clock());
                if(next_add_task_index >= 0){
                    this->b_cache.set_add_task_end(i, next_add_task_index);
                    ape_work_num += 1;
                    int fetch_index = this->b_cache.add_ps_prefetch_seq(a_acc_seq[next_add_task_index].first);
                    this->ape_list[i].set_cache_prefetch_index(fetch_index);
                }
                else{
                    if(next_add_task_index == -1 && mult_percentage_output <= 100){
                        if(0 <= pass_index && pass_index <=2)
                            task_seq[spmm_task_index].test_pass[pass_index].ape_no_task_cyc += 1;
                        else
                            task_seq[spmm_task_index].pass.ape_no_task_cyc += 1;
                    }
                    else if(next_add_task_index == -2){
                        if(0 <= pass_index && pass_index <=2)
                            task_seq[spmm_task_index].test_pass[pass_index].ape_task_occupy_cyc += 1;
                        else
                            task_seq[spmm_task_index].pass.ape_task_occupy_cyc += 1;
                    }
                }
            }
            else
                ape_work_num += 1;
        }
        if(ps_wr_occupy_row.size() > ape_work_num+16){
            assert(false);
        }
        apet[1] = clock();
        ape_time += (apet[1]-apet[0]);

        //cache_ape
        for(int i=0;i<cfg_spmm_cache_cn;i++){
            if(!this->cape_list[i].get_enable())
                continue;
            if(this->cape_list[i].add_task_end){
                if(!this->cape_list[i].task_end_process){
                    pure_add_finish_num += 1;
                    //update dependence
                    pair<int, int> target_info = this->cape_list[i].get_add_task_target_info();
                    assert(target_info.first != -1 && target_info.second != -1);
                    this->cape_list[i].task_end_process = true;
                    assert(cache_add_task_map.find(target_info.second) != cache_add_task_map.end());
                    assert(cache_add_task_map[target_info.second].find(target_info.first) != cache_add_task_map[target_info.second].end());
                    //update correponding task
                    cache_add_task &finish_task = cache_add_task_map[target_info.second][target_info.first];
                    if(finish_task.next != -1){
                        cache_add_task &next_task = cache_add_task_map[target_info.second][finish_task.next];
                        if(next_task.src1 == target_info.first){
                            assert(!next_task.src1_finish);
                            next_task.src1_finish = true;
                        }
                        else if(next_task.src2 == target_info.first){
                            assert(!next_task.src2_finish);
                            next_task.src2_finish = true;
                        }
                        else
                            assert(false);
                        if(next_task.src1_finish && next_task.src2_finish){
                            cache_add_task_pool.push(next_task);
                        }
                    }
                }
                //get new task
                if(!cache_add_task_pool.empty()){
                    cache_add_task add_task = cache_add_task_pool.front();
                    this->cape_list[i].get_new_task(add_task);
                    cache_add_task_pool.pop();
                }
            }
        }

        othert[0] = clock();
        //last work
        if(add_comp_task_num + ape_work_num == a_acc_seq.size()){
            //if there is new idle ape, trans it to cache-ape
            int new_enable_num = cfg_spmm_cache_cn-cache_ape_work_num-ape_work_num;
            assert(new_enable_num >= 0);
            for(int i=0;i<new_enable_num;i++){
                this->cape_list[i+cache_ape_work_num].set_enable(true);
            }
            if(cache_ape_work_num == 0 && new_enable_num > 0)
                cout<<mtx_mat_name<<" | *pure add task begin at: "<<this->cm.get_clock()<<endl;
            cache_ape_work_num += new_enable_num;
        }
        //if mult is end and rewrite is end, then begin pure cache add
        set<int> erase_set = {};
        for(int csr_row : finish_mem_csr_row){
            if(begin_pure_cache_add_csr_row.find(csr_row) == begin_pure_cache_add_csr_row.end()){
                begin_pure_cache_add_csr_row.emplace(csr_row);
                erase_set.emplace(csr_row);
                //init all add task
                if(mat_ps_read[csr_row].size() > 0)
                    mat_ps_all[csr_row].push_back(mat_ps_read[csr_row]);
                this->cache_add_task_update(csr_row);
            }
        }
        for(int r : erase_set)
            finish_mem_csr_row.erase(r);
        othert[1] = clock();
        other_time += (othert[1]-othert[0]);

        // applyt[0] = clock();
        cint cm_clock = this->cm.get_clock();
        abmemat[0] = clock();
        this->ab_mem.clock_apply(cm_clock);
        abmemat[1] = clock();
        // psmemat[0] = clock();
        // this->ps_mem.clock_apply(cm_clock);
        // psmemat[1] = clock();
        // this->a_buffer.clock_apply(cm_clock);
        bcacheat[0] = clock();
        this->b_cache.clock_apply(cm_clock);
        bcacheat[1] = clock();
        peat[0] = clock();
        for(int i=0;i<cfg_spmm_cache_cn;i++){
            this->mpe_list[i].clock_apply(cm_clock);
            this->ape_list[i].clock_apply(cm_clock);
            this->cape_list[i].clock_apply(cm_clock);
        }
        peat[1] = clock();
        // applyt[1] = clock();

        pea_time += (peat[1]-peat[0]);
        apea_time += (apeat[1]-apeat[0]);
        abmema_time += (abmemat[1]-abmemat[0]);
        bcachea_time += (bcacheat[1]-bcacheat[0]);
        // psmema_time += (double)(psmemat[1]-psmemat[0]);
        cape_time += (capet[1]-capet[0]);

        updatet[0] = clock();
        this->ab_mem.clock_update(cm_clock);
        // this->ps_mem.clock_update(cm_clock);
        // this->a_buffer.clock_update(cm_clock);
        this->b_cache.clock_update(cm_clock);
        for(int i=0;i<cfg_spmm_cache_cn;i++){
            this->mpe_list[i].clock_update(cm_clock);
            this->ape_list[i].clock_update(cm_clock);
            // this->cape_list[i].clock_update(cm_clock);
        }
        updatet[1] = clock();
        update_time += (updatet[1]-updatet[0]);

        // this->process_device();
        connectt[0] = clock();
        this->connect_device();
        connectt[1] = clock();
        connect_time += (connectt[1]-connectt[0]);
        this->cm.clock_step();

        b_cache_ps_prefetch_index = this->b_cache.ps_prefetch_index;

        //end this sim
        if(add_comp_task_num == a_acc_seq.size() && begin_pure_cache_add_csr_row.size() == all_csr_row.size() && finish_pure_rewrite_info.empty()){
            if(0 <= pass_index && pass_index < 3){
                pass_info &pass = task_seq[spmm_task_index].test_pass[pass_index];
                this->get_pass_performance(pass, task_begin_clock);
                this->status_print(fout, spmm_task_index, pass_index);
                pass_index += 1;
            }
            else if(pass_index == 3){
                pass_info &pass = task_seq[spmm_task_index].pass;
                this->get_pass_performance(pass, task_begin_clock);
                this->status_print(fout, spmm_task_index, pass_index);
                pass_index = -1;
                spmm_task_index += 1;
            }
            else
                assert(false);
            if(spmm_task_index >= task_seq.size())
                break;
        //↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑--End of a pass--↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑//

        //↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓--Begin of a pass--↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓//
            // if(pass_index == 2)
            //     break;
            //get the best condense type
            if(pass_index == 3 && !task_seq[spmm_task_index].test_pass.empty()){
                vector<pass_info> &test_pass = task_seq[spmm_task_index].test_pass;
                int best_condense_type = -1;
                double best_time = __DBL_MAX__;
                for(int i=0;i<3;i++){
                    double time_per_task = (double)test_pass[i].process_time/(double)(test_pass[i].mult_task_num + test_pass[i].pure_add_task_num);
                    if(best_time > time_per_task){
                        best_condense_type = test_pass[i].condense_type;
                        best_time = time_per_task;
                    }
                }
                task_seq[spmm_task_index].pass.condense_type = best_condense_type;
            }
            this->set_task(this->cm.get_clock(), spmm_task_index, pass_index);
        
            mult_percentage_output = 0;
            add_percentage_output = 0;
            pure_add_finish_num = 0;
            total_pure_add_num = 0;
        }

        // if(this->cm.get_clock() == 50000)
        //     break;
    }

    clock_end = clock();
    cout<<mtx_mat_name<<" | time: "<<(clock_end-clock_begin)/CLOCKS_PER_SEC<<" s"<<endl;

    fout.close();
    cout<<mtx_mat_name<<" | task end"<<endl;

    // mpe_time = 0, ape_time = 0, apply_time = 0, update_time = 0, connect_time = 0, other_time;
    apply_time = (double)(pea_time + apea_time + abmema_time + bcachea_time + psmema_time + cape_time);
    cout<<"mpe time: "<<mpe_time<<endl;
    cout<<"ape time: "<<ape_time<<endl;
        
    cout<<"     pea time: "<<pea_time<<endl;
    cout<<"     ab mema time: "<<abmema_time<<endl;
    cout<<"     b cachea time: "<<bcachea_time<<endl;
    cout<<"     ps mema time: "<<psmema_time<<endl;
    cout<<"apply time: "<<apply_time<<endl;

    cout<<"update time: "<<update_time<<endl;
    cout<<"connect time: "<<connect_time<<endl;
    cout<<"other time: "<<other_time<<endl;
}

#endif