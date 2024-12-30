#ifndef DEVICE_BASE
#define DEVICE_BASE

#include<base.h>

class device_base
{
protected:
    //vector<double> in_port_data;
    //vector<bool> in_port_data_ready;
    //vector<cdata> out_port_data;
    //vector<cdata> out_stagging_data;

    //index in param of the clock_apply function
    //map<string, int> in_port2index;
    //map<string, int> out_port2index;

    string device_name;
    //bool busy_status = false;
    //cint busy_end_clock = -1;
public:
    vector<string> in_port_name;
    vector<string> out_port_name;

    device_base(string name = "no name");
    ~device_base();

    string get_device_name();
    //virtual bool set_in_data(string port, double data);
    //virtual bool get_out_data(string port, cint clock, cdata& data);
    //virtual void set_ready_false();

    virtual void print_device_status(ofstream &fout) = 0;

    //virtual bool is_busy();
    //virtual void set_busy(cint end_clock);
    virtual void clock_update(cint clock) = 0;
    virtual bool clock_apply(cint clock) = 0;
};

device_base::device_base(string name){
    this->device_name = name;

    this->in_port_name.clear();
    this->out_port_name.clear();
}

device_base::~device_base(){
    cout<<this->device_name<<" base des"<<endl;
}

string device_base::get_device_name(){
    return this->device_name;
}

/*
bool device_base::data_vector_init(){
    this->in_port_data.resize(this->in_port_name.size());
    this->out_port_data.resize(this->out_port_name.size());
    this->out_stagging_data.resize(this->out_port_name.size());
    this->in_port_data_ready.resize(this->in_port_name.size());
    this->set_ready_false();
}

bool device_base::set_in_data(string port, double data){
    if(this->is_busy())
        return false;
    if(this->in_port2index.find(port) == this->in_port2index.end())
        return false;
    this->in_port_data[this->in_port2index[port]] = data;
    this->in_port_data_ready[this->in_port2index[port]] = true;
    return true;
}

bool device_base::get_out_data(string port, cint clock, cdata& data){
    //wrong output port
    if(this->out_port2index.find(port) == this->out_port2index.end()){
        data = {__DBL_MIN__, -2};
        return false;
    }

    cdata tmp = this->out_port_data[this->out_port2index[port]];
    // too early
    if(tmp.second > clock){
        data = {__DBL_MIN__, -1};
        return false;
    }
    else{
        data = tmp;
        return true;
    }
}

void device_base::set_ready_false(){
    fill(this->in_port_data_ready.begin(), this->in_port_data_ready.end(), false);
}

void device_base::print_device_status(int indent){
    cout<<setfill(' ');
    cout<<setw(indent+13)<<"device name: "<<this->get_device_name()<<" | status: "<<this->is_busy()<<" | busy end: "<<this->busy_end_clock<<endl;
    cout<<setw(indent+6)<<"input:"<<endl;
    for(string s : this->in_port_name)
        cout<<setw(indent+10)<<s<<setw(8)<<this->in_port_data[this->in_port2index[s]]<<setw(8)<<this->in_port_data_ready[this->in_port2index[s]]<<endl;
    cout<<setw(indent+9)<<"stagging:"<<endl;
    for(string s : this->out_port_name)
        cout<<setw(indent+10)<<s<<setw(8)<<this->out_stagging_data[this->out_port2index[s]]<<endl;
    cout<<setw(indent+7)<<"output:"<<endl;
    for(string s : this->out_port_name)
        cout<<setw(indent+10)<<s<<setw(8)<<this->out_port_data[this->out_port2index[s]]<<endl;
}

bool device_base::is_busy(){
    return this->busy_status;
}

void device_base::set_busy(cint end_clock){
    this->busy_status = true;
    this->busy_end_clock = end_clock;
}

void device_base::clock_update(cint clock){
    if(this->busy_status && clock >= this->busy_end_clock){
        this->busy_status = false;
        this->out_port_data.swap(this->out_stagging_data);
        fill(this->out_stagging_data.begin(), this->out_stagging_data.end(), make_pair(__DBL_MIN__, -1));
    }
}
*/
#endif