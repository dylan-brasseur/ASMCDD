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
#include "Category.h"

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
