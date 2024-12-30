#ifndef SPMM_CACHE_APE
#define SPMM_CACHE_APE

#include<base.h>
#include<device_base.h>

class cache_ape : public device_base{
private:
    // //--------------Mem side (PS write)----------------//
    // //input
    // bool in_mem_wr_ps_busy;
    // //output
    // queue<bool> out_mem_wr_ps_valid;
    // queue<bool> out_mem_wr_ps_comp;
    // queue<coo> out_mem_wr_ps_value;
    // queue<pair<int, int>> out_mem_wr_ps_name;
    // //---------------Mem side (PS read)----------------//
    // //input data
    // pair<int, int> in_mem_rd_ps_name;
    // coo in_mem_rd_ps_data;
    // bool in_mem_rd_ps_valid;
    // bool in_mem_rd_ps_busy;

    // //output data(don't use param "num" in the pkg)
    // mem_pkg out_mem_rd_ps_pkg;
    //-------------------Inner param-------------------//
    cint ps_read_delay;
    bool enable_cache_ape;
    int pe_index, task_index;
    int output_max_num;
    //add task <stamp, csr row>
    vector<pair<int, int>> add_ps;
    pair<int, int> add_res;
    bool end;
    // queue<pair<int, int>> add_res_record;
    //input and output fifo
    // vector<queue<coo>> ps;
    vector<int> ps_size;
    // vector<int> ps_send_index, ps_recv_index;
    // vector<bool> ps_end;
    // queue<pair<coo, bool>> out_buffer;
public:
    //-------------------Outer param-------------------//
    bool add_task_end, task_end_process;
    unsigned long long idle_cnt;

    cache_ape(string name = "cache_ape");
    ~cache_ape();

    void set_index(int index){this->pe_index = index;};
    void set_enable(bool enable){this->enable_cache_ape = enable;};
    bool get_enable(){return this->enable_cache_ape;};
    pair<int, int> get_add_task_target_info(){return this->add_res;};
    void reset_task();

    int get_new_task(cache_add_task add_task);
    
    void clock_update(cint clock);
    bool clock_apply(cint clock);

    // void set_mem_wr_input(bool wr_busy);
    // void get_mem_wr_output(queue<bool> &valid, queue<bool> &comp, queue<coo> &value, queue<pair<int, int>> &name);

    // void set_mem_rd_input(bool valid, bool busy, pair<int, int> name, coo data);
    // void get_mem_rd_output(mem_pkg &rd_pkg);

    void print_device_status(ofstream &fout);
};

cache_ape::cache_ape(string name) : device_base(name){
    //--------------Mem side (PS write)----------------//
    //input
    // this->in_mem_wr_ps_busy = true;
    // //output
    // clear(this->out_mem_wr_ps_valid);
    // clear(this->out_mem_wr_ps_comp);
    // clear(this->out_mem_wr_ps_value);
    // clear(this->out_mem_wr_ps_name);
    //---------------Mem side (PS read)----------------//
    //input data
    // this->in_mem_rd_ps_valid = false;
    // this->in_mem_rd_ps_busy = true;

    // //output data
    // this->out_mem_rd_ps_pkg = emp_pkg;
    //-------------------Inner param-------------------//
    this->ps_read_delay = -1;
    this->enable_cache_ape = false;
    this->output_max_num = __INT_MAX__;
    //add task
    this->add_ps.resize(2);
    this->add_res = {-1, -1};
    // clear(this->add_res_record);
    //input and output fifo
    // this->ps.resize(2);
    this->ps_size.resize(2);
    this->end = false;
    // this->ps_send_index.resize(2);
    // this->ps_recv_index.resize(2);
    // this->ps_end.resize(2);
    // clear(out_buffer);
    // fill(this->ps_size.begin(), this->ps_size.end(), 0);
    // fill(this->ps_send_index.begin(), this->ps_send_index.end(), 0);
    // fill(this->ps_recv_index.begin(), this->ps_recv_index.end(), 0);
    // fill(this->ps_end.begin(), this->ps_end.end(), true);
    // for(int i=0;i<this->ps.size();i++)
    //     clear(this->ps[i]);
    //-------------------Outer param-------------------//
    this->add_task_end = true;
    this->task_end_process = true;
    this->idle_cnt = 0;
}

cache_ape::~cache_ape(){
}

int cache_ape::get_new_task(cache_add_task add_task){
    if(!this->enable_cache_ape)
        return -1;
    //init param
    // clear(this->ps[0]);
    // clear(this->ps[1]);
    // clear(this->out_buffer);
    // fill(this->ps_send_index.begin(), this->ps_send_index.end(), 0);
    // fill(this->ps_recv_index.begin(), this->ps_recv_index.end(), 0);
    // fill(this->ps_end.begin(), this->ps_end.end(), false);
    this->add_task_end = false;
    this->task_end_process = false;

    this->add_ps[0] = {add_task.src1, add_task.csr_row};
    this->add_ps[1] = {add_task.src2, add_task.csr_row};
    this->add_res = {add_task.tar, add_task.csr_row};
    // this->add_res_record.push(this->add_res);
    this->ps_size[0] = mat_ps_all[this->add_ps[0].second][this->add_ps[0].first].size();
    this->ps_size[1] = mat_ps_all[this->add_ps[1].second][this->add_ps[1].first].size();

    this->ps_read_delay = this->ps_size[0] + this->ps_size[1];
    this->end = false;

    if(pe_debug_info_output){
        cout<<"                                                 ";
        cout<<"cache_ape "<<this->pe_index<<" | target row: "<<this->add_res.second<<" | target stamp: "<<this->add_res.first<<" get new task."<<endl;
    }
    // return this->task_index;

}

void cache_ape::reset_task(){
    //--------------Mem side (PS write)----------------//
    //input
    // this->in_mem_wr_ps_busy = true;
    // //output
    // clear(this->out_mem_wr_ps_valid);
    // clear(this->out_mem_wr_ps_comp);
    // clear(this->out_mem_wr_ps_value);
    // clear(this->out_mem_wr_ps_name);
    //---------------Mem side (PS read)----------------//
    //input data
    // this->in_mem_rd_ps_valid = false;
    // this->in_mem_rd_ps_busy = true;

    // //output data
    // this->out_mem_rd_ps_pkg = emp_pkg;
    //-------------------Inner param-------------------//
    this->enable_cache_ape = false;
    this->task_end_process = true;
    //add task
    this->add_res = {-1, -1};
    // clear(this->add_res_record);
    //input and output fifo
    // clear(this->out_buffer);
    fill(this->ps_size.begin(), this->ps_size.end(), 0);
    this->end = false;
    // fill(this->ps_send_index.begin(), this->ps_send_index.end(), 0);
    // fill(this->ps_recv_index.begin(), this->ps_recv_index.end(), 0);
    // fill(this->ps_end.begin(), this->ps_end.end(), true);
    // for(int i=0;i<this->ps.size();i++)
    //     clear(this->ps[i]);
    //-------------------Outer param-------------------//
    this->add_task_end = true;
    this->idle_cnt = 0;
}


void cache_ape::clock_update(cint clock){
    if(!this->enable_cache_ape)
        return;
    // //send read cache signal
    // int send_index = -1;
    //     //0 is not finished and size is smaller than 1
    // if(!this->add_task_end && (this->ps[0].size() < this->ps[1].size() && this->ps_send_index[0] < this->ps_size[0]) ||
    //     //1 is finished 
    //     this->ps_send_index[1] >= this->ps_size[1])
    //     send_index = 0;
    // else
    //     send_index = 1;
    // if(!this->add_task_end && !this->in_mem_rd_ps_busy && this->ps_send_index[send_index] < this->ps_size[send_index]){
    //     //<name, csr row, csr col, stamp, valid>
    //     this->out_mem_rd_ps_pkg = mem_pkg{"ps", this->add_ps[send_index].second, this->ps_send_index[send_index], this->add_ps[send_index].first, true, clock};
    //     this->ps_send_index[send_index] += 1;
    // }
    // else
    //     this->out_mem_rd_ps_pkg.valid = false;

    // //write ps
    // while(!this->in_mem_wr_ps_busy && !this->out_buffer.empty() && this->out_mem_wr_ps_comp.size() < this->output_max_num){
    //     pair<coo, bool> &out_p = this->out_buffer.front();
    //     this->out_mem_wr_ps_value.push(out_p.first);
    //     this->out_mem_wr_ps_comp.push(out_p.second);
    //     this->out_mem_wr_ps_valid.push(true);
    //     this->out_mem_wr_ps_name.push(this->add_res_record.front());
    //     if(out_p.second)
    //         this->add_res_record.pop();
    //     this->out_buffer.pop();
    // }
    // if(!this->in_mem_wr_ps_busy && !this->out_buffer.empty()){
    //     pair<coo, bool> &out_p = this->out_buffer.front();
    //     this->out_mem_wr_ps_value = out_p.first;
    //     this->out_mem_wr_ps_comp = out_p.second;
    //     this->out_mem_wr_ps_valid = true;
    //     this->out_mem_wr_ps_name = this->add_res_record.front();
    //     if(this->out_mem_wr_ps_comp)
    //         this->add_res_record.pop();
    //     this->out_buffer.pop();
    // }
    // else
    //     this->out_mem_wr_ps_valid = false;
}

bool cache_ape::clock_apply(cint clock){
    if(this->enable_cache_ape && this->add_task_end)
        this->idle_cnt += 1;
    
    if(this->add_task_end || !this->enable_cache_ape)
        return false;
    // //read from mem
    // if(this->in_mem_rd_ps_valid){
    //     int recv_index = -1;
    //     if(this->in_mem_rd_ps_name == this->add_ps[0])
    //         recv_index = 0;
    //     else if(this->in_mem_rd_ps_name == this->add_ps[1])
    //         recv_index = 1;
    //     else 
    //         assert(false);

    //     this->ps[recv_index].push(this->in_mem_rd_ps_data);
    //     this->ps_recv_index[recv_index] += 1;
    //     this->ps_end[recv_index] = (this->ps_recv_index[recv_index] == this->ps_size[recv_index]);
    // }

    // //get new ps
    // if(!this->ps[0].empty() || !this->ps[1].empty()){
    //     bool end_flag;
    //     //mult and cache is both not empty
    //     while(!this->ps[0].empty() && !this->ps[1].empty()){
    //         if(coo_coor_ls(this->ps[0].front(), this->ps[1].front())){
    //             this->out_buffer.push({this->ps[0].front(), false});
    //             this->ps[0].pop();
    //             continue;
    //         }
    //         if(coo_coor_ls(this->ps[1].front(), this->ps[0].front())){
    //             this->out_buffer.push({this->ps[1].front(), false});
    //             this->ps[1].pop();
    //             continue;
    //         }
    //         if(coo_coor_eq(this->ps[0].front(), this->ps[1].front())){
    //             coo out_p = this->ps[0].front();
    //             out_p.v += this->ps[1].front().v;
    //             end_flag = (this->ps_end[0] && this->ps_end[1]) && 
    //                        (this->ps[0].size() == 1 && this->ps[1].size() == 1);
    //             this->out_buffer.push({out_p, end_flag});
    //             this->ps[0].pop();
    //             this->ps[1].pop();
    //             continue;
    //         }
    //     }
    //     //cache end
    //     while(!this->ps[0].empty() && this->ps[1].empty() && this->ps_end[1]){
    //         end_flag = (this->ps_end[0] && this->ps_end[1] && this->ps[0].size() == 1);
    //         this->out_buffer.push({this->ps[0].front(), end_flag});
    //         this->ps[0].pop();
    //     }
    //     //mult end
    //     while(!this->ps[1].empty() && this->ps[0].empty() && this->ps_end[0]){
    //         end_flag = (this->ps_end[0] && this->ps_end[1] && this->ps[1].size() == 1);
    //         this->out_buffer.push({this->ps[1].front(), end_flag});
    //         this->ps[1].pop();
    //     }
    // }


    if(this->ps_read_delay != 0)
        this->ps_read_delay -= 1;
    else{
        // this->ps_end[0] = true;
        // this->ps_end[1] = true;
        this->end = true;
        vector<coo> tmp = {};
        // mat_ps_all[row][stamp][column]
        tmp.insert(tmp.begin(), mat_ps_all[this->add_ps[0].second][this->add_ps[0].first].begin(), mat_ps_all[this->add_ps[0].second][this->add_ps[0].first].end());
        tmp.insert(tmp.begin(), mat_ps_all[this->add_ps[1].second][this->add_ps[1].first].begin(), mat_ps_all[this->add_ps[1].second][this->add_ps[1].first].end());
        sort(tmp.begin(), tmp.end(), coo_coor_ls);
        coo p = tmp.front();
        vector<coo> ans = {};
        for(unsigned long i=1;i<tmp.size();i++){
            if(coo_coor_eq(p, tmp[i])){
                p.v += tmp[i].v;
            }
            else{
                ans.push_back(p);
                p = tmp[i];
            }
        }
        ans.push_back(p);
        int csr_row = this->add_res.second;
        int stamp = this->add_res.first;
        if(mat_ps_all.find(csr_row) == mat_ps_all.end()){
            mat_ps_all.insert({csr_row, {}});
            mat_ps_all[csr_row].clear();
        }
        while(mat_ps_all[csr_row].size() <= stamp){
            mat_ps_all[csr_row].push_back({});
        }
        mat_ps_all[csr_row][stamp].assign(ans.begin(), ans.end());
        if(finish_pure_rewrite_info.find(make_pair(csr_row, stamp)) != finish_pure_rewrite_info.end()){
            finish_pure_rewrite_info.erase(make_pair(csr_row, stamp));
        }
    }

    //task end
    // if(this->ps_end[0] && this->ps_end[1] && 
    //     this->ps[0].empty() && this->ps[1].empty() && this->out_buffer.back().second){
    if(this->end){
        this->add_task_end = true;
        finish_cache_ape_work += 1;
        if(pe_debug_info_output){
            cout<<"                                                 ";
            cout<<"cache_ape "<<this->pe_index<<" | target row: "<<this->add_res.second<<" | target stamp: "<<this->add_res.first<<" task end."<<endl;
        }
    }
    return true;
}

// void cache_ape::set_mem_wr_input(bool wr_busy){
//     this->in_mem_wr_ps_busy = wr_busy;
// }

// void cache_ape::get_mem_wr_output(queue<bool> &valid, queue<bool> &comp, queue<coo> &value, queue<pair<int, int>> &name){
//     clear(valid);
//     clear(comp);
//     clear(value);
//     clear(name);
//     valid.swap(this->out_mem_wr_ps_valid);
//     comp.swap(this->out_mem_wr_ps_comp);
//     value.swap(this->out_mem_wr_ps_value);
//     name.swap(this->out_mem_wr_ps_name);
//     // valid = this->out_mem_wr_ps_valid;
//     // comp = this->out_mem_wr_ps_comp;
//     // value = this->out_mem_wr_ps_value;
//     // name = this->out_mem_wr_ps_name;
// }

// void cache_ape::set_mem_rd_input(bool valid, bool busy, pair<int, int> name, coo data){
//     this->in_mem_rd_ps_valid = valid;
//     this->in_mem_rd_ps_busy = busy;
//     this->in_mem_rd_ps_data = data;
//     this->in_mem_rd_ps_name = name;
// }

// void cache_ape::get_mem_rd_output(mem_pkg &rd_pkg){
//     rd_pkg = this->out_mem_rd_ps_pkg;
// }

void cache_ape::print_device_status(ofstream &fout){

}
#endif