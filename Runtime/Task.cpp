#include "Task.hpp"
#include "Logger.hpp"

using namespace Runtime;

static void GetContext(ucontext_t *c){
    if (getcontext(c)<0){
        SYSERROR(); 
    }
}

static void SwapContext(ucontext_t *c1, ucontext_t *c2){
    if (swapcontext(c1,c2)<0){
        SYSERROR(); 
    }
}


//XXX: only support 64-bit intel.
static void MakeContext(ucontext_t *ctx, ContextFunc func, void *arg){
    makecontext(ctx, (void (*)())func, 2, (uintptr_t)arg & 0xFFFFFFFF, ((uintptr_t)arg >> 32)& 0xFFFFFFFF);
}


__thread CPU *CPU::current_core = NULL;
std::atomic<TaskID> Task::task_id(0);

//Delegating constructors only works in the constructor's initialization list.
Task::Task(TaskFunc f, void *arg, size_t ss)
    :Task(TaskFunctor(f), arg, ss)
{

}

Task::Task(TaskFunctor f, void *arg, size_t ss)
    : BindCPU(NULL), stack_ptr_(NULL), func_(f), arg_(arg)
{
    task_id_ = ++task_id;
    if(ss <= 0){
        ss = TASK_STACK_SIZE;
    }
    state_ = TaskState::NEW;
    stack_size = TASK_STACK_SIZE;
}

void Task::init_context(CPU *core){
    stack_ptr_ = new char[stack_size];
    GetContext(&uctx_);
    uctx_.uc_stack.ss_size = stack_size;
    uctx_.uc_stack.ss_sp = stack_ptr_;
    uctx_.uc_link = &core->sched_context;
    MakeContext(&uctx_, (ContextFunc)&Task::MainFunc, arg_);
    state_ = TaskState::READY;
    BindCPU = core;
}

Task::~Task() noexcept{
    if (stack_ptr_!=NULL){
        delete [] stack_ptr_;
    }
}
//64-bit intel
void Task::MainFunc(int p1, int p2) noexcept{
    void *arg = (void *)((uintptr_t)p2 << 32 | (uintptr_t)p1);
    Task * t = CPU::current_core->running_task;
    try{
        t->Run(arg);
    }catch(std::exception &e){
        printf("get exception:%s\n",e.what());
    }catch(...){
        printf("get unknown exception\n");
    }

    t->state_ = TaskState::DEAD;
}


void Task::Resume() noexcept{

    std::lock_guard<std::mutex> _(task_lock_);

    if (state_ == TaskState::NEW){
        init_context(CPU::current_core);
    }

    state_ = TaskState::RUNNING;
    SwapContext(&CPU::current_core->sched_context, &uctx_);

    switch (state_){
    case TaskState::SCHED:
        BindCPU->PutPrivateTask(this);
        break;
    case TaskState::READY:
    case TaskState::SUSPEND:
    case TaskState::WAITING:
    case TaskState::DEAD:
    default:
        break;
    }
}

void Task::SetReady(){
    std::lock_guard<std::mutex> _(task_lock_);
    if (state_ == TaskState::WAITING){
        state_ = TaskState::READY;
        BindCPU->PutPrivateTask(this); 
        return;
    }

    THROW(TaskError);
}

// Can only run in Task. Already have task_lock_.
void Task::Yield() noexcept{
    state_ = TaskState::WAITING;
    SwapContext(&uctx_, &(CPU::current_core->sched_context));
}

TaskQ::TaskQ(Scheduler &s)
    :sched_(s)
{

}

TaskQ::~TaskQ(){

}

void TaskQ::OnEmpty() {
    sched_.PushIdleCPU(CPU::current_core);
}





CPU::CPU(Scheduler &scheduler)
    : thread_(Loop, this), sched_(scheduler), private_queue_(sched_)
{

}

CPU::~CPU(){

}

void CPU::Loop(CPU *core){
    core->Run();
}

void CPU::PutPrivateTask(Task *t) noexcept{
    private_queue_.Push(t);
}

size_t CPU::PrivateTaskSize() const noexcept{
    return private_queue_.Size();
}

void CPU::Run(){
    current_core = this;

    Task * t = NULL;
    log_debug("core: %p, context: %p \n",this, &sched_context);

    while (true){
        t = private_queue_.PopFront();

        ++tick_;
        log_debug("pop [%d] %p, core:%p \n",tick_, t, this);

        current_core->running_task = t;

        t->Resume();

        // TODO: reuse this task.
        if(t->State()==TaskState::DEAD){
            log_debug("delete task:%p, private_queue_ size: %d \n", t, PrivateTaskSize());
            delete t;
        }
    }
}



Scheduler::Scheduler(size_t count)
    :core_id_(0)
{
    
    for(size_t i = 0 ; i < count ; ++i){
        CPU *core = new CPU(*this);
        cpus_.push_back(core);
    }
}

Scheduler::~Scheduler(){
    for(auto &core:cpus_){
        delete core;
    }
}

TaskID Scheduler::Spawn(TaskFunctor f, void *arg){
    Task* t= new Task(f,arg);
    PutTask(t);
    return t->Id();
}
/*
 * 如果Task已绑定cpu，则push到该cpu的调度队列，否则push给状态idle的cpu，
 * 如果没有idle cpu则采用Round-Robin选择cpu。
 */
inline void Scheduler::PutTask(Task *t){
    CPU *core = t->BindCPU;
    if (core==NULL){

        core = idle_queue_.PopFront(NULL);
        // not idle cpu
        if(core == NULL){
            core = NextCPU();
        }
    }
    return core->PutPrivateTask(t);
}





#ifdef __TASK_UNITTEST
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


Logger log;

class Test{
public:
    ~Test(){
        printf("test RAII\n");
    }
};
void test1(void *t){
    printf("test1 before yield \n");
    Test a;
    Yield();
    throw "XXX";
    printf("test1 after yield, exit test1 \n");
}
void test2(void *t){
    printf("test2 before yield \n");
    Yield();
    printf("test2 after yield, exit test2 \n");
}

int main(int argc, char **argv){

    int n = 1;
    if(argc>1){
        n = atoi(argv[1]);
    }
    Scheduler sched(n);
    
    for(int i = 0 ; i<3 ; ++i){
    //    printf("%d \n",sched.Spawn(&test1));
        printf("%d \n",sched.Spawn(&test2));
        sched.Spawn([](void *t){
                return;
                });
        sched.Spawn([i](void *t){
                uintptr_t a = (uintptr_t)t;
                printf("test%d before arg:%d yield\n", i, a);
                Yield();
                printf("test%d after yield, exit now\n",i);
                },(void *)222222);
    }

    while (true){
        sleep(1);
    }
}
#endif// __TASK_UNITTEST
