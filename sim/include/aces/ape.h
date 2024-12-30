#ifndef SPMM_APE
#define SPMM_APE

#include<base.h>
#include<device_base.h>

class ape : public device_base{
private:
    //-------------Cahce side (PS write)---------------//
    //input
    bool in_cache_wr_ps_busy;
    
    // bool out_cache_wr_ps_valid;
    // bool out_cache_wr_ps_comp;
    // coo out_cache_wr_ps_value;
    //--------------Cahce side (PS read)---------------//
    bool in_cache_rd_ps_busy;
    // coo in_cache_rd_ps_data;
    // bool in_cache_rd_ps_valid;
    // bool in_cache_rd_ps_busy;

    
    //-------------------Mult side---------------------//
    
    // coo in_mult_data;
    // bool in_mult_end;
    // bool in_mult_valid;
    //output (if it's true, reveice a whole ps from mult)
    int send_csr_row;
    bool send_mult_data;
    //-------------------Inner param-------------------//
    int pe_index, task_index;
    int output_max_num;
    //add task
    int cache_prefetch_index;
    //input and output fifo
    queue<coo> ps_cache, ps_mult;
    int out_buffer_size;
    queue<pair<coo, bool>> out_buffer;
    queue<int> out_task_index_buffer;
    bool ps_mult_read_send;
    int ps_cache_size, ps_cache_send_index, ps_cache_recv_index;
    bool ps_cache_end, ps_mult_end;
    bool erase_rd_occupy;
    //task end
    //csr row, end cyc
    map<int, int> csr_row_end_cnt;
public:
    //input
    queue<coo> *in_mult_data;
    queue<bool> *in_mult_end;
    queue<bool> *in_mult_valid;

    //input data
    queue<coo> *in_cache_rd_ps_data;
    queue<bool> *in_cache_rd_ps_valid;

    //output data(don't use param "num" in the pkg)
    queue<mem_pkg> *out_cache_rd_ps_pkg;

    //output
    queue<bool> *out_cache_wr_ps_valid;
    queue<bool> *out_cache_wr_ps_comp;
    queue<coo> *out_cache_wr_ps_value;






    //-------------------Outer param-------------------//
    int add_csr_row;
    bool add_task_end;
    unsigned long long idle_cnt, to_early_cnt;
    
    ape(string name = "ape", int ob_size = cfg_spmm_ape_op_size);
    ~ape();

    void set_index(int index){this->pe_index = index;};
    int get_new_task(cint clock);
    int get_working_csr_row(){return this->add_csr_row;};
    void reset_task();
    void set_cache_prefetch_index(int index);

    void clock_update(cint clock);
    bool clock_apply(cint clock);

    void set_cache_wr_input(bool wr_busy);
    //void get_cache_wr_output(bool &valid, bool &comp, coo &value);
    void get_cache_wr_output(queue<bool> &valid, queue<bool> &comp, queue<coo> &value);

    void set_cache_rd_input(bool busy);
    void get_cache_rd_output(queue<mem_pkg> &rd_pkg);

    void set_mult_input(queue<coo> &data, queue<bool> &end, queue<bool> &valid);
    void get_mult_output(int &send_csr_row, bool &send_data);

    void print_device_status(ofstream &fout);
};

ape::ape(string name, int ob_size) : device_base(name){
    //-------------Cahce side (PS write)---------------//
    //input
    this->in_cache_wr_ps_busy = true;
    //output
    // clear(this->out_cache_wr_ps_valid);
    // clear(this->out_cache_wr_ps_comp);
    // clear(this->out_cache_wr_ps_value);
    // this->out_cache_wr_ps_valid = false;
    // this->out_cache_wr_ps_comp = false;
    //--------------Cahce side (PS read)---------------//
    //input data
    // clear(this->in_cache_rd_ps_valid);
    // clear(this->in_cache_rd_ps_data);
    // this->in_cache_rd_ps_valid = false;
    // this->in_cache_rd_ps_busy = true;

    //output data(don't use param "num" in the pkg)
    // clear(this->out_cache_rd_ps_pkg);
    // this->out_cache_rd_ps_pkg = emp_pkg;
    //-------------------Mult side---------------------//
    //input
    // clear(this->in_mult_data);
    // clear(this->in_mult_end);
    // clear(this->in_mult_valid);
    // this->in_mult_end = false;
    // this->in_mult_valid = false;
    //output (if it's true, reveice a whole ps from mult)
    this->send_mult_data = false;
    //-------------------Inner param-------------------//
    this->add_csr_row = -1;
    this->task_index = -1;
    this->cache_prefetch_index = -1;
    this->output_max_num = __INT_MAX__;
    clear(this->ps_cache);
    clear(this->ps_mult);
    this->out_buffer_size = ob_size;
    clear(this->out_buffer);
    clear(this->out_task_index_buffer);
    this->ps_mult_read_send = false;
    this->ps_cache_end = false;
    this->ps_mult_end = false;
    this->erase_rd_occupy = false;
    this->ps_cache_size = 0;
    this->ps_cache_send_index = 0;
    this->ps_cache_recv_index = 0;
    this->to_early_cnt = 0;
    this->csr_row_end_cnt.clear();
    //-------------------Outer param-------------------//
    this->add_task_end = true;
    this->idle_cnt = 0;
}

ape::~ape(){
}

int ape::get_new_task(cint clock){
    int next_task_row = -1, task_index = -1;
    #ifdef MPE_RECORD_SQ
    //no new task
    if(mpe_record_sq[this->pe_index].empty()){
        this->add_task_end = true;
        return -1;
    }
    //row occupy
    for(auto &mt : mpe_record_sq[this->pe_index]){
        if(ps_rd_occupy_row.find(mt.first) == ps_rd_occupy_row.end() && 
           ps_wr_occupy_row.find(mt.first) == ps_wr_occupy_row.end()){
            next_task_row = mt.first;
            task_index = mpe_record_sq[this->pe_index][next_task_row].front();
            break;
        }
    }
    #endif

    #ifdef MPE_RECORD_BUFFER
    //no new task
    if(mpe_record_buffer[this->pe_index].empty()){
        this->add_task_end = true;
        return -1;
    }
    //row occupy
    int mpe_row = mpe_record_buffer[this->pe_index].front().first;
    if(ps_rd_occupy_row.find(mpe_row) == ps_rd_occupy_row.end() && 
        ps_wr_occupy_row.find(mpe_row) == ps_wr_occupy_row.end()){
        next_task_row = mpe_row;
        task_index = mpe_record_buffer[this->pe_index].front().second;
    }
    #endif

    if(next_task_row == -1){
        this->add_task_end = true;
        return -2;
    }
    //occupy row
    assert(ps_rd_occupy_row.emplace(next_task_row).second);
    assert(ps_wr_occupy_row.emplace(next_task_row).second);

    //init param
    clear(this->ps_cache);
    clear(this->ps_mult);
    clear(this->out_buffer);
    clear(this->out_task_index_buffer);
    this->add_csr_row = next_task_row;
    this->ps_mult_read_send = false;
    this->ps_cache_end = false;
    this->ps_mult_end = false;
    this->erase_rd_occupy = false;
    this->ps_cache_size = mat_ps_read_size[next_task_row];
    this->ps_cache_send_index = 0;
    this->ps_cache_recv_index = 0;
    this->add_task_end = false;
    this->send_csr_row = next_task_row;
    this->task_index = task_index;
    if(pe_debug_info_output){//&& this->pe_index == 11){
        cout<<"                          ";
        cout<<"ape "<<this->pe_index<<" | task index: "<<this->task_index<<" get new task."<<endl;
    }
    return this->task_index;
}

void ape::set_cache_prefetch_index(int index){
    this->cache_prefetch_index = index;
    this->out_task_index_buffer.push(index);
}

void ape::reset_task(){
    //-------------Cahce side (PS write)---------------//
    //input
    this->in_cache_wr_ps_busy = true;
    //output
    // clear(this->out_cache_wr_ps_valid);
    // clear(this->out_cache_wr_ps_comp);
    // clear(this->out_cache_wr_ps_value);
    // this->out_cache_wr_ps_valid = false;
    // this->out_cache_wr_ps_comp = false;
    //--------------Cahce side (PS read)---------------//
    // clear(this->in_cache_rd_ps_valid);
    // clear(this->in_cache_rd_ps_data);
    // this->in_cache_rd_ps_valid = false;
    // this->in_cache_rd_ps_busy = true;

    //output data(don't use param "num" in the pkg)
    // clear(this->out_cache_rd_ps_pkg);
    // this->out_cache_rd_ps_pkg = emp_pkg;
    //-------------------Mult side---------------------//
    //input
    // clear(this->in_mult_data);
    // clear(this->in_mult_end);
    // clear(this->in_mult_valid);
    // this->in_mult_end = false;
    // this->in_mult_valid = false;
    //output (if it's true, reveice a whole ps from mult)
    this->send_mult_data = false;
    //-------------------Inner param-------------------//
    this->add_csr_row = -1;
    this->task_index = -1;
    this->cache_prefetch_index = -1;
    clear(this->ps_cache);
    clear(this->ps_mult);
    clear(this->out_buffer);
    this->ps_mult_read_send = false;
    this->ps_cache_end = false;
    this->ps_mult_end = false;
    this->erase_rd_occupy = false;
    this->ps_cache_size = 0;
    this->ps_cache_send_index = 0;
    this->ps_cache_recv_index = 0;
    this->to_early_cnt = 0;
    this->csr_row_end_cnt.clear();
    //-------------------Outer param-------------------//
    this->add_task_end = true;
    this->idle_cnt = 0;
}

void ape::clock_update(cint clock){
    //send read mult signal
    if(!this->add_task_end && !ps_mult_read_send){
        this->send_mult_data = true;
        this->ps_mult_read_send = true;
    }
    else
        this->send_mult_data = false;
    
    
    // if(!this->add_task_end && !this->in_cache_rd_ps_busy && this->ps_cache_send_index < this->ps_cache_size){
    //     this->out_cache_rd_ps_pkg = mem_pkg{"ps", this->add_csr_row, this->ps_cache_send_index, 1, true, clock};
    //     this->ps_cache_send_index += 1;
    // }
    // else
    //     this->out_cache_rd_ps_pkg.valid = false;

    if(this->cache_prefetch_index > b_cache_ps_prefetch_index || (!this->out_task_index_buffer.empty() && this->out_task_index_buffer.front() > b_cache_ps_prefetch_index))
        this->to_early_cnt += 1;

    //write ps
    while(!this->in_cache_wr_ps_busy && !this->out_buffer.empty() && this->out_cache_wr_ps_comp->size() < this->output_max_num && this->out_task_index_buffer.front() <= b_cache_ps_prefetch_index){
        pair<coo, bool> &out_p = this->out_buffer.front();
        this->out_cache_wr_ps_value->push(out_p.first);
        this->out_cache_wr_ps_comp->push(out_p.second);
        this->out_cache_wr_ps_valid->push(true);
        if(out_p.second)
            this->out_task_index_buffer.pop();
        this->out_buffer.pop();
        int erase = -1;
        for(auto it=this->csr_row_end_cnt.begin();it!=this->csr_row_end_cnt.end();it++){
            it->second -= 1;
            if(it->second == 0){
                assert(erase == -1);
                finish_add_csr_row.emplace(it->first);
                erase = it->first;
            }
        }
        if(erase != -1){
            assert(this->out_cache_wr_ps_comp->back());
            this->csr_row_end_cnt.erase(erase);
        }
    }
    

    //send read cache signal
    if(this->cache_prefetch_index > b_cache_ps_prefetch_index){
        return;
    }
    while(!this->add_task_end && !this->in_cache_rd_ps_busy && this->ps_cache_send_index < this->ps_cache_size){
        this->out_cache_rd_ps_pkg->push(mem_pkg{"ps", this->add_csr_row, this->ps_cache_send_index, 1, true, clock});
        this->ps_cache_send_index += 1;
    }

    // //write ps
    // if(!this->in_cache_wr_ps_busy && !this->out_buffer.empty()){
    //     pair<coo, bool> &out_p = this->out_buffer.front();
    //     this->out_cache_wr_ps_value = out_p.first;
    //     this->out_cache_wr_ps_comp = out_p.second;
    //     this->out_cache_wr_ps_valid = true;
    //     this->out_buffer.pop();
    // }
    // else
    //     this->out_cache_wr_ps_valid = false;
}

bool ape::clock_apply(cint clock){

    if(add_task_end){
        this->idle_cnt += 1;
        return false;
    }

    //read from mult
    while(this->ps_mult_read_send && !this->in_mult_valid->empty()){
        this->ps_mult.push(this->in_mult_data->front());
        this->ps_mult_end = this->in_mult_end->front();
        this->in_mult_data->pop();
        this->in_mult_end->pop();
        this->in_mult_valid->pop();
    }
    // if(this->ps_mult_read_send && this->in_mult_valid){
    //     this->ps_mult.push(this->in_mult_data);
    //     this->ps_mult_end = this->in_mult_end;
    // }
    //read from cache
    while(!this->in_cache_rd_ps_valid->empty() && this->ps_cache_recv_index < this->ps_cache_size){
        this->ps_cache.push(this->in_cache_rd_ps_data->front());
        this->ps_cache_recv_index += 1;
        this->ps_cache_end = (this->ps_cache_recv_index == this->ps_cache_size);
        this->in_cache_rd_ps_valid->pop();
        this->in_cache_rd_ps_data->pop();
    }
    if(this->ps_cache_size == 0 && this->cache_prefetch_index <= b_cache_ps_prefetch_index){
        this->ps_cache_end = true;
    }

    // if(this->in_cache_rd_ps_valid && this->ps_cache_recv_index < this->ps_cache_size){
    //     this->ps_cache.push(this->in_cache_rd_ps_data);
    //     this->ps_cache_recv_index += 1;
    //     this->ps_cache_end = (this->ps_cache_recv_index == this->ps_cache_size);
    // }
    // else if(this->ps_cache_size == 0){
    //     this->ps_cache_end = true;
    // }
    if(this->ps_cache_end && !this->erase_rd_occupy){
        ps_rd_occupy_row.erase(this->add_csr_row);
        this->erase_rd_occupy = true;
    }
    //get new ps
    if((!this->ps_mult.empty() || !this->ps_cache.empty()) && this->out_buffer.size() < this->out_buffer_size){
        bool end_flag;
        //mult and cache is both not empty
        while(!this->ps_mult.empty() && !this->ps_cache.empty()){
            if(this->ps_mult.front().c < this->ps_cache.front().c){
                assert(this->ps_mult.front().r == this->ps_cache.front().r);
                this->out_buffer.push({this->ps_mult.front(), false});
                this->ps_mult.pop();
                continue;
            }
            if(this->ps_cache.front().c < this->ps_mult.front().c){
                assert(this->ps_mult.front().r == this->ps_cache.front().r);
                this->out_buffer.push({this->ps_cache.front(), false});
                this->ps_cache.pop();
                continue;
            }
            if(this->ps_mult.front().c == this->ps_cache.front().c){
                assert(this->ps_mult.front().r == this->ps_cache.front().r);
                coo out_p = this->ps_mult.front();
                out_p.v += this->ps_cache.front().v;
                end_flag = (this->ps_mult_end && this->ps_cache_end) && 
                            (this->ps_mult.size() == 1 && this->ps_cache.size() == 1);
                this->out_buffer.push({out_p, end_flag});
                this->ps_mult.pop();
                this->ps_cache.pop();
                continue;
            }
        }
        //cache end
        while(!this->ps_mult.empty() && this->ps_cache.empty() && this->ps_cache_end){
            end_flag = (this->ps_mult_end && this->ps_cache_end && this->ps_mult.size() == 1);
            this->out_buffer.push({this->ps_mult.front(), end_flag});
            this->ps_mult.pop();
        }
        //mult end
        while(!this->ps_cache.empty() && this->ps_mult.empty() && this->ps_mult_end){
            end_flag = (this->ps_mult_end && this->ps_cache_end && this->ps_cache.size() == 1);
            this->out_buffer.push({this->ps_cache.front(), end_flag});
            this->ps_cache.pop();
        }
    }

    //task end
    if(this->ps_mult_end && this->ps_cache_end && 
        this->ps_mult.empty() && this->ps_cache.empty() && this->out_buffer.back().second){
        this->add_task_end = true;
        add_comp_task_num += 1;
        assert(all_csr_row.find(this->add_csr_row) != all_csr_row.end());
        all_csr_row[this->add_csr_row] -= 1;
        if(all_csr_row[this->add_csr_row] == 0){
            this->csr_row_end_cnt.insert({this->add_csr_row, this->out_buffer.size()});
        }

        if(pe_debug_info_output){// && this->pe_index == 11){
            cout<<"                          ";
            cout<<"ape "<<this->pe_index<<" | task index: "<<this->task_index<<" task end."<<endl;
        }
    }

    clear(*this->in_mult_data);
    clear(*this->in_mult_valid);
    clear(*this->in_mult_end);

    clear(*this->in_cache_rd_ps_valid);
    clear(*this->in_cache_rd_ps_data);

    // clear(*this->out_cache_rd_ps_pkg);

    // clear(*this->out_cache_wr_ps_valid);
    // clear(*this->out_cache_wr_ps_comp);
    // clear(*this->out_cache_wr_ps_value);
    return true;
}

void ape::set_cache_wr_input(bool wr_busy){
    this->in_cache_wr_ps_busy = wr_busy;
}

// void ape::get_cache_wr_output(queue<bool> &valid, queue<bool> &comp, queue<coo> &value){
//     clear(valid);
//     clear(comp);
//     clear(value);
//     valid.swap(this->out_cache_wr_ps_valid);
//     comp.swap(this->out_cache_wr_ps_comp);
//     value.swap(this->out_cache_wr_ps_value);
//     // valid = this->out_cache_wr_ps_valid;
//     // comp = this->out_cache_wr_ps_comp;
//     // value = this->out_cache_wr_ps_value;
// }

void ape::set_cache_rd_input(bool busy){
    // clear(this->in_cache_rd_ps_valid);
    // clear(this->in_cache_rd_ps_data);
    // this->in_cache_rd_ps_valid.swap(valid);
    this->in_cache_rd_ps_busy = busy;
    // this->in_cache_rd_ps_data.swap(data);
}

// void ape::get_cache_rd_output(queue<mem_pkg> &rd_pkg){
//     clear(rd_pkg);
//     rd_pkg.swap(this->out_cache_rd_ps_pkg);
// }

// void ape::set_mult_input(queue<coo> &data, queue<bool> &end, queue<bool> &valid){
//     clear(this->in_mult_valid);
//     clear(this->in_mult_end);
//     clear(this->in_mult_data);
//     this->in_mult_valid.swap(valid);
//     this->in_mult_end.swap(end);
//     this->in_mult_data.swap(data);
// }

void ape::get_mult_output(int &send_csr_row, bool &send_data){
    send_csr_row = this->send_csr_row;
    send_data = this->send_mult_data;
}

void ape::print_device_status(ofstream &fout){

}
#endif