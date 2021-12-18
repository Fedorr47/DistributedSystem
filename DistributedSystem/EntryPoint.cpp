#include <iostream>
#include <thread>
#include <chrono>
#include <time.h>
#include <cstdlib>

const int kAgentCount = 10;
const int kBuildCount = 100;
const int kTestsCout = 20;

class Agent {

    class Queue {
    public:

        enum class JobStatus {
            JOB_IN_PROGRESS, JOB_DONE, JOB_READY_FOR_BUILD, JOB_FAILED
        };

        class Job {
        public:
            int count_;
            Agent* assigned_agent_;
            JobStatus status_;
        public:
            Job(int count) {
                count_ = count;
                status_ = JobStatus::JOB_READY_FOR_BUILD;
            }
        };

    public:
        
        Job* job_list_;
        int job_count_;

    public:
        Queue(int count) {
            job_count_ = count;
            job_list_ = static_cast<Job*>(malloc(sizeof(Job) * count));
            for (int i = 0; i < count; i++) {
                new(job_list_ + i) Job(1);
            }
        }

        Queue(int min, int max, int count) {
            std::srand(std::time(nullptr));
            int* jobs_sizes_ = static_cast<int*>(malloc(sizeof(int) * count));
            int tmp_count = count;
            job_count_ = 0;
            int rand_count;

            while (tmp_count > max) {
                rand_count = min + std::rand() % (max - min);
                jobs_sizes_[job_count_] = rand_count;
                tmp_count -= rand_count;
                job_count_++;
            }
            jobs_sizes_[job_count_] = tmp_count;
            job_count_++;

            job_list_ = static_cast<Job*>(malloc(sizeof(Job) * job_count_));
            for (int i = 0; i < job_count_; i++) {
                new(job_list_ + i) Job(jobs_sizes_[i]);
            }

            std::cout << "Jobs to do: \n";
            for (int i = 0; i < job_count_; i++) {
                std::cout << job_list_[i].count_ << " ";
            }
            std::cout << std::endl;

            free(jobs_sizes_);
        }

        bool AssignJob(Agent* InAgent) {
            Job* job;
            bool job_assigned = false;
            for (int i = 0; i < job_count_; i++) {
                if (job_list_[i].status_ == JobStatus::JOB_READY_FOR_BUILD || job_list_[i].status_ == JOB_FAILED) {
                    job = &job_list_[i];
                    job->assigned_agent_ = InAgent;
                    job->status_ = JobStatus::JOB_IN_PROGRESS;
                    job_assigned = true;
                    a->doJob(job);
                    break;
                }
            }
            return job_assigned;
        }

        bool JobsDone() {
            bool done = true;
            for (int i = 0; i < job_count_; i++) {
                if (job_list_[i].status_ != JOB_DONE) {
                    done = false;
                }
            }
            return done;
        }

    };

public:
    int state_;
    int identity_;
    enum States {
        STATE_AVAILABLE, STATE_BUSY, STATE_OFFLINE
    };
public:
    Agent(int id) {
        state_ = STATE_AVAILABLE;
        identity_ = id;
    }

    void doJob(Queue::Job* job) {
        auto workflow = [](Queue::Job* j, Agent* a) {
            a->state_ = STATE_BUSY;
            printf("[%d] Job started (%d)\n", a->identity_, j->count_);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000000 * j->count_));
            printf("[%d] Job finished\n", a->identity_);
            j->status_ = Queue::JOB_DONE;
            a->state_ = STATE_AVAILABLE;
        };
        std::thread thread(workflow, job, this);
        thread.detach();
    }

    Agent* getAllAgents() {
        return generateAgents(kAgentCount);
    }

    double buildContent(int count) {
        Agent* a_list = getAllAgents();
        Queue* q = new Queue(1, 10, count);
        time_t start, end;
        time(&start);
        while (!q->JobsDone()) {
            for (int i = 0; i < kAgentCount; i++) {
                if (a_list[i].state_ == STATE_AVAILABLE) {
                    q->AssignJob(&a_list[i]);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000000));
        }
        time(&end);
        double build_time = difftime(end, start);
        std::cout << "Build time: " << build_time << "s" << std::endl;
        return build_time;

    }
private:
    Agent* generateAgents(int count) {
        Agent* agents;
        agents = static_cast<Agent*>(malloc(sizeof(Agent) * count));
        for (int i = 0; i < count; i++) {
            new(agents + i) Agent(i);
        }
        return agents;
    }
};

int main(int argc, const char* argv[]) {
    Agent* current_agent = new Agent(999);

    double time[kTestsCout];
    double summ = 0;
    for (int i = 0; i < kTestsCout; i++) {
        time[i] = current_agent->buildContent(kBuildCount);
        summ += time[i];
    }
    double avg = summ / kTestsCout;

    std::cout << "Agent count: " << kAgentCount << std::endl;
    std::cout << "Build count: " << kBuildCount << std::endl;
    std::cout << "Average build time: " << avg << std::endl;
    std::cout << "Time for each build: " << std::endl;
    for (int i = 0; i < kTestsCout; i++) {
        std::cout << time[i] << " ";
    }

    std::thread::hardware_concurrency();


    return 0;
}