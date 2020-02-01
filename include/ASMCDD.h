//
// Created by "Dylan Brasseur" on 28/11/2019.
//

#ifndef DISKSPROJECT_ASMCDD_H
#define DISKSPROJECT_ASMCDD_H

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <random>
#include <mutex>
#include "utils.h"

struct Target_pcf_type{
    Target_pcf_type(float _mean, float _min, float _max): mean(_mean), min(_min), max(_max){};
    float mean;
    float min;
    float max;
    float radius=0;
};

struct Compute_status{
    float rmax;
    std::vector<Disk> disks;
    std::vector<unsigned long> parents;
};


struct ASMCDD_params{
    float step = 0.1;
    float sigma = 0.25;
    float limit = 5;
    float domainLength = 1;
    unsigned long max_iter = 2000;
    float threshold = 0.001;
    float error_delta=0.0001;
    bool distanceThreshold = true;
    std::string example_filename;
};

class Category{
public:
    explicit Category(unsigned long _id, std::shared_ptr<std::vector<Category>> _categories, std::shared_ptr<ASMCDD_params> _params) : id(_id), categories(std::move(_categories)), params(std::move(_params)){initialized=false;};
    void setTargetDisks(std::vector<Disk> const & target);
    void addTargetDisk(Disk const & d);
    void addDependency(unsigned long parent_id);
    void addChild(unsigned long child_id);
    void computeTarget();
    std::vector<Target_pcf_type> getCurrentPCF(unsigned long parent);
    std::vector<Target_pcf_type> getTargetPCF(unsigned long parent);
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getCurrentPCFs();
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getTargetPCFs();
    std::vector<Disk> getCurrentDisks();
    std::vector<Disk> getTargetDisks();

    Compute_status getComputeStatus();
    Compute_status getTargetComputeStatus();

    unsigned long getFinalSize(float domainLength);


    void reset();
    void initialize(float domainLength, float e_delta);
    void refine(unsigned long max_iter, float threshold, bool isDistanceThreshold);
    void normalize(float domainLength);
private:

    unsigned long id;
    std::vector<unsigned long> parents_id;
    std::vector<unsigned long> children_id;

    std::map<unsigned long, std::vector<Target_pcf_type>> pcf;

    std::map<unsigned long, std::vector<Target_pcf_type>> target_pcf;
    std::map<unsigned long, float> target_rmax;
    std::map<unsigned long, std::vector<float>> target_areas;
    std::map<unsigned long, std::vector<float>> target_radii;

    std::vector<Disk> disks;
    std::vector<Disk> target_disks;

    std::shared_ptr<std::vector<Category>> categories;
    std::shared_ptr<ASMCDD_params> params;

    bool initialized;
    unsigned long finalSize=0;
    static std::mutex disks_access;

};

class ASMCDD{
public:
    explicit ASMCDD(){ categories=std::make_shared<std::vector<Category>>(); params=std::make_shared<ASMCDD_params>();};
    explicit ASMCDD(std::string const & filename);
    void loadFile(std::string const & filename);
    unsigned long addTargetClass(std::vector<Disk> const & target);
    Category & getClass(unsigned long id);
    void setParams(ASMCDD_params const & _params);
    void computeTarget();


    void addDependency(unsigned long parent, unsigned long child);
    void initialize(float domainLength, float e_delta);
    void refine(unsigned long max_iter, float threshold, bool isDistanceThreshold);
    void normalize(float domainLength);
    std::vector<Target_pcf_type> getTargetPCF(unsigned long self);
    std::vector<Target_pcf_type> getTargetPCF(unsigned long parent, unsigned long child);
    std::vector<Target_pcf_type> getCurrentPCF(unsigned long self);
    std::vector<Target_pcf_type> getCurrentPCF(unsigned long parent, unsigned long child);

    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getTargetPCFplot();
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getCurrentPCFplot();

    std::vector<Disk> getCurrentDisks(unsigned long id);
    std::vector<Disk> getTargetDisks(unsigned long id);

    std::pair<std::vector<std::vector<Disk>>, std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>>> getPrettyPCFplot(float domainLength, std::vector<unsigned long> const & currentSizes);
    std::pair<std::vector<std::vector<Disk>>, std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>>> getPrettyTargetPCFplot(float domainLength);

    std::vector<unsigned long> getFinalSizes(float domainLength);

private:
    std::shared_ptr<std::vector<Category>> categories;
    std::shared_ptr<ASMCDD_params> params;
};

#endif //DISKSPROJECT_ASMCDD_H
