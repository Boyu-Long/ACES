#ifndef MAT_AB_MEMORY
#define MAT_AB_MEMORY

#include<base.h>
#include<device_base.h>
// all coor in program is based on the coor in csr format, coor in coo format is pure data
// only support read one elem at a time

template<typename T>
struct ab_mem_great{
    bool operator()(const T &p1, const T &p2){
        return p1 > p2;
    }
};


class mat_ab_memory : public device_base{

friend class simulator;

private:
    //output data <name, data>
    // queue<pair<string, coo>> out_data;

    //input data
        //16 elem per cycle
    // queue<bool> in_wr_comp;
    // queue<pair<int, int>> in_wr_name;
    // queue<coo> in_wr_value;
        // 16 channel(max 16 cmd per clock)
    // queue<mem_pkg> in_rd_pkg;
    //vector<mem_pkg> in_rd_pkg;
        //spcial port for delete mat, not in 16 channel, 
        //only contain the list of mat which will be deleted
    vector<string> in_delete_list;

    //inner param
        //every channel has 128 bit: 32b for row, 32b for column, 64b for value
    int channel_num;
    int delay;
    string end_task_ps_name;

    // clock, remain num, data
    queue<pair<cint, pair<string, coo>>> read_stagging;
    priority_queue<mem_pkg, vector<mem_pkg>, ab_mem_great<mem_pkg>> read_ps_stagging;
    //vector<queue<pair<pair<cint, int>, coo>>> read_stagging;

    //read
    bool read_operate(cint clock, cint clock_first, string name, int row, int column, int num);
    bool read_operate_ab(cint clock, cint clock_first, string name, int row, int column, int num);
    bool read_operate_ps(cint clock, cint clock_first, string name, int row, int column, int num);
    void read_error(cint clock, string name, int row, int column, int num, string error);
    //delete
    void delete_mat();

    //performance
    unsigned long long read_cnt, write_cnt;
public:
    //output data <name, data>
    queue<pair<string, coo>> *out_data;

    //input data
        //16 elem per cycle
    queue<bool> *in_wr_comp;
    queue<pair<int, int>> *in_wr_name;
    queue<coo> *in_wr_value;

    //input
    queue<mem_pkg> *in_rd_pkg;
    // queue<mem_pkg> *in_rd_pkg_b;
    // queue<mem_pkg> *in_rd_pkg_ps;


    void reset_task();

    void set_rd_input(cint clock, queue<mem_pkg> &rd_pkg);
    void get_rd_output(cint clock, queue<pair<string, coo>> &data);
    
    void set_wr_input(cint clock, queue<bool> &comp, queue<pair<int, int>> &name, queue<coo> &value);
    void get_wr_output(cint clock, bool &wr_busy);

    void set_end_task_name(string name){this->end_task_ps_name = name;};

    mat_ab_memory(int cn=cfg_spmm_mem_cn, int d=cfg_spmm_mem_d, string name="mem");
    ~mat_ab_memory();

    void print_device_status(ofstream &fout);

    void clock_update(cint clock);
    bool clock_apply(cint clock);
};

// read: "A, (0,1)" means: read coo(0,3), the column is in csr format
// name: A
//      |-----|
// (0,0)|(0,3)|(0,4)(0,5)
// (1,0)(1,2)(1,3)(1,5)(1,7)
// (2,1)(2,3)(2,4)
//read mat ps:
//row is the index in mat_ps

// read:            |begin   end|
//                  -------------
// cycle:       0   1   2   3   4   5   6
// --input-------------------------------
// in valid:    f   t   f   f   f   f   f
// name:        x   N   x   x   x   x   x
// coo:         x   v1  x   x   x   x   x
// num:         x   x   x   x   x   x   x
// --output------------------------------
// data:        x   x   x   x   d1  x   x
// count:       x   x   x   x   x   x   x
// valid:       f   f   f   f   t   x   x
//                      |---|
//                      delay

mat_ab_memory::mat_ab_memory(int cn, int d, string name) : device_base(name){

    this->channel_num = cn;
    this->delay = d;
    //output
    // clear(this->out_data);
    //input
    // clear(this->in_rd_pkg);
    // clear(this->in_wr_comp);
    // clear(this->in_wr_name);
    // clear(this->in_wr_value);
    //store
    clear(this->read_stagging);
    while(!this->read_ps_stagging.empty())
        this->read_ps_stagging.pop();
    this->end_task_ps_name = "";

    //performance
    this->read_cnt = 0;
    this->write_cnt = 0;
}

mat_ab_memory::~mat_ab_memory(){
}

void mat_ab_memory::reset_task(){
    //output
    // clear(this->out_data);
    //input
    // clear(this->in_rd_pkg);
    // clear(this->in_wr_comp);
    // clear(this->in_wr_name);
    // clear(this->in_wr_value);
    //store
    clear(this->read_stagging);
    while(!this->read_ps_stagging.empty())
        this->read_ps_stagging.pop();

    //performance
    this->read_cnt = 0;
    this->write_cnt = 0;
}

void mat_ab_memory::read_error(cint clock, string name, int row, int column, int num, string error){
    cout<<"Memory read error in "<<this->device_name<<", error info: "<<error<<endl;
    cout<<"clock: "<<clock<<", read_param: ("<<name<<", "<<row<<", "<<column<<", "<<num<<")"<<endl;
}

bool mat_ab_memory::read_operate(cint clock, cint clock_first, string name, int row, int column, int num){
    //read a or b
    if(name.compare("a") == 0 || name.compare("b") == 0){
        vector<vector<coo>> *mat = nullptr;
        if(name.compare("a") == 0)
            mat = &mat_a_read;
        else
            mat = &mat_b_read;
        
        if(row >= mat->size())
            this->read_error(clock, name, row, column, num, "row is out of bound");
        if(column >= (*mat)[row].size())
            this->read_error(clock, name, row, column, num, "column is out of bound");
        this->read_stagging.push({clock_first, {name, (*mat)[row][column]}});
    }
    //read p_s
    else{
        if(coo2csr_ps.find(row) == coo2csr_ps.end())
            this->read_error(clock, name, row, column, num, "row is not found");
        if(column >= mat_ps_read[coo2csr_ps[row]].size())
            this->read_error(clock, name, row, column, num, "column is out of bound");
        this->read_stagging.push({clock_first, {name, mat_ps_read[coo2csr_ps[row]][column]}});
    }
    return true;
}

bool mat_ab_memory::read_operate_ab(cint clock, cint clock_first, string name, int row, int column, int num){
    //read a or b
    if(name.compare("a") == 0 || name.compare("b") == 0){
        vector<vector<coo>> *mat = nullptr;
        if(name.compare("a") == 0)
            mat = &mat_a_read;
        else
            mat = &mat_b_read;
        
        if(row >= mat->size())
            this->read_error(clock, name, row, column, num, "row is out of bound");
        if(column >= (*mat)[row].size())
            this->read_error(clock, name, row, column, num, "column is out of bound");
        this->read_stagging.push({clock_first, {name, (*mat)[row][column]}});
        return true;
    }
    else
        return false;
}

bool mat_ab_memory::read_operate_ps(cint clock, cint clock_first, string name, int row, int column, int num){
    //read p_s
    if(name.compare("ps") == 0){
        if(row >= mat_ps_read.size())
            this->read_error(clock, name, row, column, num, "row is not found");
        if(column >= mat_ps_read_size[row])
            this->read_error(clock, name, row, column, num, "column is out of bound");
        this->read_stagging.push({clock_first, {name, mat_ps_read[row][column]}});
        return true;
    }
    else
        return false;
}

void mat_ab_memory::delete_mat(){
    this->in_delete_list.clear();
}

void mat_ab_memory::clock_update(cint clock){
    //input update
    // clear(this->in_rd_pkg);
    
    //output update
    // clear(this->out_data);
    while(!this->read_stagging.empty() && this->read_stagging.front().first == clock){
        this->out_data->push(this->read_stagging.front().second);
        this->read_stagging.pop();
    }
}

bool mat_ab_memory::clock_apply(cint clock){
    //process read cmd only
    cint clock_first;
    if(this->read_stagging.empty())
        clock_first = clock + this->delay;
    else
        clock_first = max(clock + this->delay, this->read_stagging.back().first + 1);
    assert(clock_first == clock+this->delay);
    //  4 cmd per clock(mpe a)
    // 16 cmd per clock(mpe b)
    int ab_read_cnt = 0;
    while(!this->in_rd_pkg->empty()){
        mem_pkg rd_info = this->in_rd_pkg->front();
        if(rd_info.mat_name.compare("b") == 0){
            this->read_cnt += 1;
            ab_read_cnt += 1;
            if(!this->read_operate_ab(clock, clock_first, rd_info.mat_name, rd_info.mat_row, rd_info.mat_colmn, rd_info.num)){
                assert(false);
            }
        }
        else if(rd_info.mat_name.compare("ps") == 0){
            this->read_ps_stagging.push(rd_info);
        }
        else
            assert(false);
        this->in_rd_pkg->pop();
    }
    assert(ab_read_cnt <= 20);

    // 16 cmd per clock(ape ps)
    vector<mem_pkg> skip_ps = {};
    int ps_read_cnt = 0;
    int max_ps_read_size = this->channel_num;
    //int max_ps_read_size = __INT_MAX__;
    while(!this->read_ps_stagging.empty() && ps_read_cnt < max_ps_read_size){
        mem_pkg rd_info = this->read_ps_stagging.top();
        this->read_ps_stagging.pop();
        if(rd_info.mat_colmn >= mat_ps_read[rd_info.mat_row].size() || mat_ps_wr_cnt[{rd_info.mat_row, mat_ps_read_stamp[rd_info.mat_row]}] > 0){
            skip_ps.push_back(rd_info);
        }
        else{
            if(!this->read_operate_ps(clock, clock_first, rd_info.mat_name, rd_info.mat_row, rd_info.mat_colmn, rd_info.num))
                assert(false);
            else{
                ps_read_cnt += 1;
                this->read_cnt += 1;
            }
        }
    }
    for(mem_pkg &pkg : skip_ps){
        this->read_ps_stagging.push(pkg);
    }
    //assert(ps_read_cnt <= 16);

    

    // //assert(this->in_rd_pkg.size() <= 36);
    // while(!this->in_rd_pkg.empty()){
    //     this->read_cnt += 1;
    //     mem_pkg rd_info = this->in_rd_pkg.front();
    //     if(!this->read_operate(clock, clock_first, rd_info.mat_name, rd_info.mat_row, rd_info.mat_colmn, rd_info.num)){
    //         assert(false);
    //     }
    //     this->in_rd_pkg.pop();
    // }

    int stamp, csr_row;
    bool comp;
    coo value;
    while(!this->in_wr_value->empty()){
        stamp = this->in_wr_name->front().first;
        csr_row = this->in_wr_name->front().second;
        if(csr_row >= mat_ps_read.size())
            assert(false);
        comp = this->in_wr_comp->front();
        value = this->in_wr_value->front();
        if(stamp == mat_ps_read_stamp[csr_row]){
            mat_ps_read[csr_row].push_back(value);
            if(comp){
                mat_ps_wr_cnt[{csr_row, stamp}] -= 1;
                //has another re-write
                if(mat_ps_wr_cnt[{csr_row, stamp}] != 0)
                    mat_ps_read[csr_row].clear();
                //last re-write
                else{
                    assert(mat_ps_read[csr_row].size() == mat_ps_read_size[csr_row]);
                    if(finish_rewrite_csr_row.find(csr_row) != finish_rewrite_csr_row.end()){
                        finish_mem_csr_row.emplace(csr_row);
                        finish_rewrite_csr_row.erase(csr_row);
                    }
                }
                assert((mat_ps_wr_cnt[{csr_row, stamp}] >= 0));
            }
        }
        else{
            if(mat_ps_all.find(csr_row) == mat_ps_all.end()){
                mat_ps_all.insert({csr_row, {}});
                mat_ps_all[csr_row].clear();
            }
            //assert(mat_ps_all[csr_row].size() == stamp || mat_ps_all[csr_row].size() == stamp+1);
            assert(mat_ps_all[csr_row].size() >= stamp);
            if(mat_ps_all[csr_row].size() == stamp){
                mat_ps_all[csr_row].push_back({});
            }
            mat_ps_all[csr_row][stamp].push_back(value);
            if(comp){
                mat_ps_wr_cnt[{csr_row, stamp}] -= 1;
                assert((mat_ps_wr_cnt[{csr_row, stamp}] >= 0));
                //has another re-write
                if(mat_ps_wr_cnt[{csr_row, stamp}] != 0)
                    mat_ps_all[csr_row][stamp].clear();
                //last re-write
                else{
                    if(finish_rewrite_csr_row.find(csr_row) != finish_rewrite_csr_row.end()){
                        finish_mem_csr_row.emplace(csr_row);
                        finish_rewrite_csr_row.erase(csr_row);
                    }
                }
            }
        }
        this->in_wr_comp->pop();
        this->in_wr_name->pop();
        this->in_wr_value->pop();
    }
    this->delete_mat();
    clear(*this->in_rd_pkg);
    clear(*this->in_wr_comp);
    clear(*this->in_wr_name);
    clear(*this->in_wr_value);
    return true;
}

// void mat_ab_memory::set_rd_input(cint clock, queue<mem_pkg> &rd_pkg){
//     clear(this->in_rd_pkg);
//     this->in_rd_pkg.swap(rd_pkg);
// }

// void mat_ab_memory::get_rd_output(cint clock, queue<pair<string, coo>> &data){
//     clear(data);
//     data.swap(this->out_data);
// }
    
// void mat_ab_memory::set_wr_input(cint clock, queue<bool> &comp, queue<pair<int, int>> &name, queue<coo> &value){
//     // if(value.size() > this->channel_num*2)
//     //     assert(false);
//     clear(this->in_wr_comp);
//     this->in_wr_comp.swap(comp);
//     clear(this->in_wr_name);
//     this->in_wr_name.swap(name);
//     clear(this->in_wr_value);
//     this->in_wr_value.swap(value);
// }

void mat_ab_memory::get_wr_output(cint clock, bool &wr_busy){
    wr_busy = false;
}

void mat_ab_memory::print_device_status(ofstream &fout){
    fout<<"Device name: "<<this->device_name<<endl;
    fout<<"Read count: "<<this->read_cnt<<endl;
    fout<<"Write count: "<<this->write_cnt<<endl;
}

#endif