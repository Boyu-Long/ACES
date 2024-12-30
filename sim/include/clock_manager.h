#ifndef CLOCK_MANAGER
#define CLOCK_MANAGER

#include<base.h>

class clock_manager{
private:
    cint _clock;
public:
    clock_manager();
    ~clock_manager();
    void clock_step();
    bool clock_skip(int skip_num);
    cint get_clock();
};

clock_manager::clock_manager(){
    this->_clock = 0;
}

clock_manager::~clock_manager(){
    cout<<"clock manager deconstruction"<<endl;
}

void clock_manager::clock_step(){
    _clock++;
}

bool clock_manager::clock_skip(int skip_num){
    if(skip_num < 0)
        return false;

    _clock += skip_num;
    return true;
}

cint clock_manager::get_clock(){
    return _clock;
}

#endif