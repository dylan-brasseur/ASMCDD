//
// Created by "Dylan Brasseur" on 06/02/2020.
//

#ifndef DISKSPROJECT_CATEGORY_H
#define DISKSPROJECT_CATEGORY_H

#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include "utils.h"

/**
 * This class is a class in the algorithm and holds the disks
 */
class Category{
public:
    explicit Category(unsigned long _id, std::shared_ptr<std::vector<Category>> _categories, std::shared_ptr<ASMCDD_params> _params) : id(_id), categories(std::move(_categories)), params(std::move(_params)){initialized=false;};

    void setTargetDisks(std::vector<Disk> const & target);
    void addTargetDisk(Disk const & d);

    /**
     * Adds the dependency of this class of its parent
     * \param parent_id
     */
    void addDependency(unsigned long parent_id);
    void addChild(unsigned long child_id);

    /**
     * Computes the pcfs for the target disks
     */
    void computeTarget();

    std::vector<Target_pcf_type> getCurrentPCF(unsigned long parent);
    std::vector<Target_pcf_type> getTargetPCF(unsigned long parent);

    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getCurrentPCFs();
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getTargetPCFs();

    std::vector<Disk> getCurrentDisks();
    std::vector<Disk> getTargetDisks();

    /**
     * Get the compute status, which includes the disks, target rmax and the parents of the category
     * \return
     */
    Compute_status getComputeStatus();
    Compute_status getTargetComputeStatus();

    /**
     * Gets the final number of disks for a given domain length
     * \param domainLength Domain length
     * \return
     */
    unsigned long getFinalSize(float domainLength);


    void reset();

    /**
     * Initialization part of the algorithm
     * \param domainLength Domain length
     * \param e_delta Error delta added at each dart throw fail
     */
    void initialize(float domainLength, float e_delta);

    /**
     * Refinement part of the algorithm, not implemented
     * \param max_iter
     * \param threshold
     * \param isDistanceThreshold
     */
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

#endif //DISKSPROJECT_CATEGORY_H
