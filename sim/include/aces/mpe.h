#ifndef SPMM_MPE
#define SPMM_MPE

#include<base.h>
#include<device_base.h>

class mpe : public device_base{
private:
    //-----------------Cahce side (B)-----------------//
    //input data
    // queue<coo> in_cache_rd_b_data;
    // queue<bool> in_cache_rd_b_valid;
    //queue<bool> in_cache_rd_b_busy;
    // coo in_cache_rd_b_data;
    // bool in_cache_rd_b_valid;
    bool in_cache_rd_b_busy;

    //output data(don't use param "num" in the pkg)
    // queue<mem_pkg> out_cache_rd_b_pkg;
    //mem_pkg out_cache_rd_b_pkg;
    // //-----------------Cahce side (A)-----------------//
    // //input data
    // coo in_cache_rd_a_data;
    // bool in_cache_rd_a_valid;
    // bool in_cache_rd_a_busy;

    // //output data(don't use param "num" in the pkg)
    // mem_pkg out_cache_rd_a_pkg;
    int read_a_delay;
    int a_cache_delay_time;
    //-------------------Adder side--------------------//
    //input (if it's true, send a whole ps to adder)
    int send_csr_row;
    bool send_mult_data;
    
    // coo out_add_data;
    // bool out_add_end;
    // bool out_add_valid;
    //-------------------Inner param-------------------//
    int pe_index;
    //for read a and b
    mult_task mt;
    bool p_a_rd_send, p_a_rd_recv;
    coo p_a;
    int b_rd_send_index, b_rd_recv_index;
    queue<coo> b_row;
    //output
    int output_csr_row;
    int out_buffer_buffer_size;
    //<result, end_flag>
    queue<pair<coo, bool>> out_buffer_buffer;
    //<csr_row, <result, end_flag>>
    map<int, queue<pair<coo, bool>>> out_buffer_sq;
    //queue<pair<coo, bool>> out_buffer;
    //output
    bool output_flag;
public:
    //input data
    queue<coo> *in_cache_rd_b_data;
    queue<bool> *in_cache_rd_b_valid;

    //output data(don't use param "num" in the pkg)
    queue<mem_pkg> *out_cache_rd_b_pkg;

    //output
    queue<coo> *out_add_data;
    queue<bool> *out_add_end;
    queue<bool> *out_add_valid;




    //-------------------Outer param-------------------//
    bool mult_task_end;
    unsigned long long idle_cnt;
    mpe(string name = "mpe", int ob_size = cfg_spmm_mpe_op_size);
    ~mpe();

    bool set_task(mult_task mt);
    void set_index(int index){this->pe_index = index;};
    void reset_task();
    
    void clock_update(cint clock);
    bool clock_apply(cint clock);

    void set_cache_a_input(coo data, bool valid, bool busy);
    void get_cache_a_output(mem_pkg &pkg);

    void set_cache_b_input(bool busy);
    void get_cache_b_output(queue<mem_pkg> &pkg);

    void set_adder_input(int send_csr_row, bool send_data);
    void get_adder_output(queue<coo> &data, queue<bool> &end, queue<bool> &valid);

    void print_device_status(ofstream &fout);
};

mpe::mpe(string name, int ob_size) : device_base(name){
    //-----------------Cahce side (B)-----------------//
    //input data
    // clear(this->in_cache_rd_b_data);
    // clear(this->in_cache_rd_b_valid);
    // this->in_cache_rd_b_valid = false;
    this->in_cache_rd_b_busy = true;
    //output data
    // clear(this->out_cache_rd_b_pkg);
    //-----------------Cahce side (A)-----------------//
    //input data
    // this->in_cache_rd_a_valid = false;
    // this->in_cache_rd_a_busy = true;
    this->read_a_delay = -1;
    //-------------------Adder side--------------------//
    //input (if it's true, send a whole ps to adder)
    this->send_csr_row = -1;
    this->send_mult_data = false;
    //output
    // clear(this->out_add_data);
    // clear(this->out_add_end);
    // clear(this->out_add_valid);
    // this->out_add_end = true;
    // this->out_add_valid = false;
    //-------------------Inner param-------------------//
    this->pe_index = -1;
    //for read a and b
    this->a_cache_delay_time = 2;
    this->p_a_rd_recv = true;
    this->p_a_rd_send = true;
    this->b_rd_send_index = -1;
    this->b_rd_recv_index = -1;
    clear(this->b_row);
    this->output_csr_row = -1;
    this->out_buffer_buffer_size = ob_size;
    clear(this->out_buffer_buffer);
    this->out_buffer_sq.clear();
    this->output_flag = false;
    //-------------------Outer param-------------------//
    this->mult_task_end = true;
    this->idle_cnt = 0;
}

mpe::~mpe(){
}
bool mpe::set_task(mult_task mt){
    assert(this->mult_task_end);
    assert(this->p_a_rd_send && this->p_a_rd_recv);
    assert(this->b_rd_send_index == this->b_rd_recv_index && this->b_rd_recv_index == this->mt.info_b.second);
    assert(this->b_row.empty());

    // clear(this->out_cache_rd_b_pkg);
    this->mt = mt;
    clear(this->b_row);
    this->p_a_rd_send = true;
    this->p_a_rd_recv = false;
    this->p_a = mat_a_read[this->mt.coor_a.first][this->mt.coor_a.second];
    this->b_rd_send_index = 0;
    this->b_rd_recv_index = 0;
    this->mult_task_end = false;
    this->read_a_delay = this->a_cache_delay_time;
    if(pe_debug_info_output)// && this->pe_index == 11)
        cout<<"mpe "<<this->pe_index<<" | task index: "<<this->mt.acc_seq_index<<" get new task."<<endl;
    
    #ifdef MPE_RECORD_SQ
    if(mpe_record_sq[this->pe_index].find(this->mt.coor_a.first) == mpe_record_sq[this->pe_index].end()){
        assert(this->out_buffer_sq.find(this->mt.coor_a.first) == this->out_buffer_sq.end());
        this->out_buffer_sq.insert({this->mt.coor_a.first, queue<pair<coo, bool>>()});
        mpe_record_sq[this->pe_index].insert({this->mt.coor_a.first, queue<int>()});
    }
    mpe_record_sq[this->pe_index][this->mt.coor_a.first].push(this->mt.acc_seq_index);
    #endif

    #ifdef MPE_RECORD_BUFFER
    mpe_record_buffer[this->pe_index].push({this->mt.coor_a.first, this->mt.acc_seq_index});
    #endif
}

void mpe::reset_task(){
    //-----------------Cahce side (B)-----------------//
    //input data
    // clear(this->in_cache_rd_b_data);
    // clear(this->in_cache_rd_b_valid);
    // this->in_cache_rd_b_valid = false;
    this->in_cache_rd_b_busy = true;
    //outout data
    // clear(this->out_cache_rd_b_pkg);
    //-----------------Cahce side (A)-----------------//
    //input data
    // this->in_cache_rd_a_valid = false;
    // this->in_cache_rd_a_busy = true;
    this->read_a_delay = -1;
    //-------------------Adder side--------------------//
    //input (if it's true, send a whole ps to adder)
    this->send_csr_row = -1;
    this->send_mult_data = false;
    //output
    // clear(this->out_add_data);
    // clear(this->out_add_end);
    // clear(this->out_add_valid);
    // this->out_add_end = true;
    // this->out_add_valid = false;
    //-------------------Inner param-------------------//
    this->mt = mult_task();
    //for read a and b
    this->p_a_rd_recv = true;
    this->p_a_rd_send = true;
    this->b_rd_send_index = -1;
    this->b_rd_recv_index = -1;
    clear(this->b_row);
    this->output_csr_row = -1;
    clear(this->out_buffer_buffer);
    this->out_buffer_sq.clear();
    this->output_flag = false;
    //-------------------Outer param-------------------//
    this->mult_task_end = true;
    this->idle_cnt = 0;
}

void mpe::clock_update(cint clock){
    // //read a
    // if(!this->mult_task_end && !this->in_cache_rd_a_busy && !this->p_a_rd_send){
    //     this->out_cache_rd_a_pkg = mem_pkg{"a", this->mt.coor_a.first, this->mt.coor_a.second, 1, true, clock};
    //     this->p_a_rd_send = true;
    // }
    // else
    //     this->out_cache_rd_a_pkg.valid = false;

    //read b
    while(!this->mult_task_end && !this->in_cache_rd_b_busy && this->b_rd_send_index < this->mt.info_b.second){
        this->out_cache_rd_b_pkg->push(mem_pkg{"b", this->mt.info_b.first, this->b_rd_send_index, 1, true, clock});
        this->b_rd_send_index += 1;
        // if(this->out_cache_rd_b_pkg.size() >= 1)
        //     break;
    }
    // if(!this->mult_task_end && !this->in_cache_rd_b_busy && this->b_rd_send_index < this->mt.info_b.second){
    //     this->out_cache_rd_b_pkg = mem_pkg{"b", this->mt.info_b.first, this->b_rd_send_index, 1, true, clock};
    //     this->b_rd_send_index += 1;
    // }
    // else
    //     this->out_cache_rd_b_pkg.valid = false;
    
    #ifdef MPE_RECORD_SQ
    //write ps
    if(this->output_flag && !this->out_buffer_sq[this->output_csr_row].empty()){
        pair<coo, bool> out_p = this->out_buffer_sq[this->output_csr_row].front();
        this->out_buffer_sq[this->output_csr_row].pop();
        this->out_add_data = out_p.first;
        this->out_add_end = out_p.second;
        this->out_add_valid = true;
        if(out_p.second){
            mpe_record_sq[this->pe_index][this->output_csr_row].pop();
            if(this->out_buffer_sq[this->output_csr_row].empty()){
                assert(mpe_record_sq[this->pe_index][this->output_csr_row].empty() || 
                      (mpe_record_sq[this->pe_index][this->output_csr_row].size() == 1 && 
                       mpe_record_sq[this->pe_index][this->output_csr_row].front() == this->mt.acc_seq_index));
                if(mpe_record_sq[this->pe_index][this->output_csr_row].empty()){
                    mpe_record_sq[this->pe_index].erase(this->output_csr_row);
                    this->out_buffer_sq.erase(this->output_csr_row);
                }
            }
            this->output_flag = false;
            this->output_csr_row = -1;
        }
    }
    else
        this->out_add_valid = false;
    #endif
    #ifdef MPE_RECORD_BUFFER
    //write ps
    while(this->output_flag && !this->out_buffer_buffer.empty()){
        pair<coo, bool> out_p = this->out_buffer_buffer.front();
        this->out_buffer_buffer.pop();
        this->out_add_data->push(out_p.first);
        this->out_add_end->push(out_p.second);
        this->out_add_valid->push(true);
        if(out_p.second){
            mpe_record_buffer[this->pe_index].pop();
            this->output_flag = false;
        }
    }
    // if(this->output_flag && !this->out_buffer_buffer.empty()){
    //     pair<coo, bool> out_p = this->out_buffer_buffer.front();
    //     this->out_buffer_buffer.pop();
    //     this->out_add_data = out_p.first;
    //     this->out_add_end = out_p.second;
    //     this->out_add_valid = true;
    //     if(out_p.second){
    //         mpe_record_buffer[this->pe_index].pop();
    //         this->output_flag = false;
    //     }
    // }
    // else
    //     this->out_add_valid = false;
    #endif
}

bool mpe::clock_apply(cint clock){
    //output p_s
    if(!this->output_flag && this->send_mult_data){
        this->output_flag = true;
        this->output_csr_row = this->send_csr_row;
    }
    
    // if(mult_percentage_output <= 100 && mult_task_end)
    //     this->idle_cnt += 1;

    if(mult_task_end){
        this->idle_cnt += 1;
        return false;
    }

    //read A row
    if(!this->p_a_rd_recv){
        if(this->read_a_delay <= 0)
            this->p_a_rd_recv = true;
        else
            this->read_a_delay -= 1;
    }
    

    //read B column
    while(this->b_rd_recv_index < this->mt.info_b.second && !this->in_cache_rd_b_valid->empty()){
        this->b_row.push(this->in_cache_rd_b_data->front());
        this->b_rd_recv_index += 1;
        this->in_cache_rd_b_data->pop();
        this->in_cache_rd_b_valid->pop();
    }
    // if(this->b_rd_recv_index < this->mt.info_b.second && this->in_cache_rd_b_valid){
    //     this->b_row.push(this->in_cache_rd_b_data);
    //     this->b_rd_recv_index += 1;
    // }

    #ifdef MPE_RECORD_SQ
    //get p_s
    if(this->p_a_rd_recv && !this->b_row.empty() && this->out_buffer_sq.size() < this->out_buffer_buffer_size){
        assert(this->p_a.c == this->b_row.front().r);
        coo psum_p = coo{this->p_a.r, this->b_row.front().c, this->p_a.v*this->b_row.front().v};
        bool end_flag = false;
        if(this->b_rd_recv_index == this->mt.info_b.second && this->b_row.size() == 1){
            end_flag = true;
            this->mult_task_end = true;
            if(pe_debug_info_output)
                cout<<"mpe "<<this->pe_index<<" | task index: "<<this->mt.acc_seq_index<<" task end."<<endl;
            
        }
        this->out_buffer_sq[this->mt.coor_a.first].push({psum_p, end_flag});
        this->b_row.pop();
    }
    #endif

    #ifdef MPE_RECORD_BUFFER
    //get p_s
    if(this->p_a_rd_recv && !this->b_row.empty() && this->out_buffer_buffer.size() < this->out_buffer_buffer_size){
        assert(this->p_a.c == this->b_row.front().r);
        coo psum_p = coo{this->p_a.r, this->b_row.front().c, this->p_a.v*this->b_row.front().v};
        bool end_flag = false;
        if(this->b_rd_recv_index == this->mt.info_b.second && this->b_row.size() == 1){
            end_flag = true;
            this->mult_task_end = true;
            if(pe_debug_info_output)// && this->pe_index == 11)
                cout<<"mpe "<<this->pe_index<<" | task index: "<<this->mt.acc_seq_index<<" task end."<<endl;
            
        }
        this->out_buffer_buffer.push({psum_p, end_flag});
        this->b_row.pop();
    }
    clear(*this->in_cache_rd_b_data);
    clear(*this->in_cache_rd_b_valid);
    // clear(*this->out_cache_rd_b_pkg);

    // clear(*this->out_add_data);
    // clear(*this->out_add_valid);
    // clear(*this->out_add_end);
    return true;
    #endif
}

// void mpe::set_cache_a_input(coo data, bool valid, bool busy){
//     this->in_cache_rd_a_data = data;
//     this->in_cache_rd_a_valid = valid;
//     this->in_cache_rd_a_busy = busy;
// }

// void mpe::get_cache_a_output(mem_pkg &pkg){
//     pkg = this->out_cache_rd_a_pkg;
// }

void mpe::set_cache_b_input(bool busy){
    // clear(this->in_cache_rd_b_data);
    // clear(this->in_cache_rd_b_valid);
    // this->in_cache_rd_b_data.swap(data);
    // this->in_cache_rd_b_valid.swap(valid);
    // this->in_cache_rd_b_data = data;
    // this->in_cache_rd_b_valid = valid;
    this->in_cache_rd_b_busy = busy;
}

// void mpe::get_cache_b_output(queue<mem_pkg> &pkg){
//     clear(pkg);
//     pkg.swap(this->out_cache_rd_b_pkg);
// }

void mpe::set_adder_input(int send_csr_row, bool send_data){
    this->send_csr_row = send_csr_row;
    this->send_mult_data = send_data;
}

// void mpe::get_adder_output(queue<coo> &data, queue<bool> &end, queue<bool> &valid){
//     clear(data);
//     clear(end);
//     clear(valid);
//     data.swap(this->out_add_data);
//     end.swap(this->out_add_end);
//     valid.swap(this->out_add_valid);
// }

void mpe::print_device_status(ofstream &fout){

}
#endif