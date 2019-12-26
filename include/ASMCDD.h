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
#include "utils.h"

float perimeter_weight(float x, float y, float r);

struct ClassInteraction{
    ClassInteraction(unsigned int a, unsigned int b) : class_a(a), class_b(b){};
    unsigned int class_a=0, class_b=0;
    std::vector<float> meanPCF, maxPCF, minPCF, radii;
    void computePCF(std::vector<Disk> const & disks_a, std::vector<Disk> const & disks_b, float rmax, float step, float sigma, float limit, bool same_class);
    static float disk_distance(Disk const & a, Disk const & b, float rmax);
    void discardPCFs();
};

struct Target_pcf_type{
    Target_pcf_type(float _mean, float _min, float _max): mean(_mean), min(_min), max(_max){};
    float mean;
    float min;
    float max;
    float radius=0;
};


struct ASMCDD_params{
    float step = 0.1;
    float sigma = 0.25;
    float limit = 5;
    bool changed = true;
};

class Category{
public:
    explicit Category(unsigned long _id, std::shared_ptr<std::vector<Category>> _categories, std::shared_ptr<ASMCDD_params> _params) : id(_id), categories(std::move(_categories)), params(std::move(_params)){initialized=false;};
    void setTargetDisks(std::vector<Disk> const & target);
    void addDependency(unsigned long parent_id);
    void addChild(unsigned long child_id);
    void computeTarget();
    std::vector<Target_pcf_type> getCurrentPCF(unsigned long parent);
    std::vector<Target_pcf_type> getTargetPCF(unsigned long parent);
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getCurrentPCFs();
    std::vector<Disk> getCurrentDisks();


    void reset();
    void initialize(float domainLength, float e_delta);
    void refine();
private:

    unsigned long id;
    std::vector<unsigned long> parents_id;
    std::vector<unsigned long> children_id;

    std::map<unsigned long, std::vector<Target_pcf_type>> pcf;
    std::map<unsigned long, std::vector<std::vector<float>>> distances;

    std::map<unsigned long, std::vector<Target_pcf_type>> target_pcf;
    std::map<unsigned long, float> target_rmax;
    std::map<unsigned long, std::vector<float>> target_areas;

    std::vector<Disk> disks;
    std::vector<Disk> target_disks;

    std::shared_ptr<std::vector<Category>> categories;
    std::shared_ptr<ASMCDD_params> params;

    bool initialized;

};

class ASMCDD{
public:
    explicit ASMCDD()=default;
    explicit ASMCDD(std::string const & filename);
    unsigned int addClass(std::vector<Disk> const & disks);
    unsigned int addClassDependency(unsigned int a, unsigned int b);
    void computePCF();
    std::vector<ClassInteraction> & getSavedInteractions();
    std::vector<std::vector<Disk>> & getSavedDisks();

    void setTargetPCFs(unsigned long nClasses, std::vector<ClassInteraction> const & _target);
    void initialisation(float domainLength, float e_delta);

    void save();

private:
    std::vector<std::vector<Disk>> disks;
    std::vector<std::vector<Disk>> disks_saved;

    std::vector<ClassInteraction> interaction_graph;
    std::vector<ClassInteraction> interaction_saved;

    std::vector<ClassInteraction> target_graph;
    std::vector<std::vector<Disk>> target_disks;
    std::vector<unsigned int> topological_order;
    bool topological_order_computed=false;
};

class ASMCDD_new{
public:
    explicit ASMCDD_new(){categories=std::make_shared<std::vector<Category>>(); params=std::make_shared<ASMCDD_params>();};
    explicit ASMCDD_new(std::string const & filename);
    unsigned long addTargetClass(std::vector<Disk> const & target);
    Category & getClass(unsigned long id);
    void setParams(ASMCDD_params const & _params);
    void computeTarget();


    void addDependency(unsigned long parent, unsigned long child);
    void initialize(float domainLength, float e_delta);
    std::vector<Target_pcf_type> getTargetPCF(unsigned long self);
    std::vector<Target_pcf_type> getTargetPCF(unsigned long parent, unsigned long child);
    std::vector<Target_pcf_type> getCurrentPCF(unsigned long self);
    std::vector<Target_pcf_type> getCurrentPCF(unsigned long parent, unsigned long child);

    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getTargetPCFplot();
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getCurrentPCFplot();

    std::vector<Disk> getCurrentDisks(unsigned long id);

private:
    std::shared_ptr<std::vector<Category>> categories;
    std::shared_ptr<ASMCDD_params> params;
};

#endif //DISKSPROJECT_ASMCDD_H
